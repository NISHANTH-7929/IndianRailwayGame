#include "RailWorldSnapshot.h"
#include "RailScenarioContent.h"
#include "RailSnapshotArchive.h"
#include <cmath>
#include <cstring>
#include <type_traits>

namespace Rail {
void WorldSnapshotCodec::Transfer(SnapshotArchive& a,OperationsWorld& w){
    a.Fields(w.accumulator_,w.paused_,w.suspended_,w.skipResumeDelta_,w.error_);
    auto& state=w.state_;auto& snapshot=state.snapshot;
    a.Fields(snapshot.tick,snapshot.valid,snapshot.blocks,snapshot.points,snapshot.routes,
        snapshot.authorities,snapshot.lastRequest,snapshot.events,state.requests,state.sequences,
        state.nextGeneration,state.aiTrace,state.traceHash);
    // The content ID defines the roster (Tambaram v6 has three trains).
    // Changed rosters require new content IDs, never transform-based migration.
    for(auto& pair:state.trains){auto& train=pair.second;auto& s=train.spatial;auto& h=train.handling;
        a.Fields(s.current_.train,s.current_.tick,s.current_.headStationMetres,s.current_.commandedSpeedMps,
            s.current_.head,s.current_.leadingEnd,s.formation_,s.choices_,s.history_.occurrences,
            s.previousHead_,s.accumulator_,s.paused_,s.error_,s.FixedStepSeconds,s.MaxStepsPerUpdate,
            h.state_,h.train_,h.source_,h.lastSubmittedSequence_,h.queue_,h.initialized_,h.suspended_,h.skipResumeDelta_,h.cancelPending_,train.commandSequence);
        bool bound=state.aiBindings.count(pair.first)!=0;a.Value(bound);
        if(bound){auto& binding=state.aiBindings[pair.first];a.Fields(binding.route,binding.generation,binding.acceptedSequence);}
        bool driver=state.drivers.count(pair.first)!=0;a.Value(driver);
        if(driver){auto& d=state.drivers[pair.first];a.Fields(d.train_,d.route_,d.binding_,d.sequence_,d.nextRetry_,d.lastTick_,d.phase_,d.observed_,d.hadRoute_);
            // Only the newly versioned roster adds the endpoint latch bytes.
            // Other content retains its existing checkpoint layout.
            if(ScenarioContent::IsTambaram(w.contentId_))a.Value(d.stopping_);
            a.Value(d.reason_);}
    }
}
Result WorldSnapshotCodec::ValidateAndRebuild(OperationsWorld& w){
    auto fail=[](const char* reason){return Result::Fail(reason);};
    if(w.paused_||!w.error_.empty()||!w.state_.snapshot.valid)return fail("Faulted world cannot become a recovery checkpoint");
    if(w.accumulator_<0||w.accumulator_>=1.0/30.0+1e-9)return fail("Invalid world clock remainder");
    if(w.state_.snapshot.tick>UINT64_MAX-18001||!w.state_.nextGeneration)return fail("Exhausted logical counter");
    Fixture fixture;Formation shortTrain,longTrain;auto contentResult=ScenarioContent::Resolve(w.ContentId(),fixture,shortTrain,longTrain);if(!contentResult.ok)return contentResult;std::set<VehicleId> vehicles;
    auto sameKeys=[](const auto& values,const auto& definitions){if(values.size()!=definitions.size())return false;for(const auto& pair:values)if(!definitions.count(pair.first))return false;return true;};
    if(!sameKeys(w.state_.snapshot.blocks,w.definitions_.blocks)||!sameKeys(w.state_.snapshot.points,w.definitions_.points)||
        !sameKeys(w.state_.snapshot.routes,w.definitions_.routes)||!sameKeys(w.state_.snapshot.authorities,w.state_.trains))return fail("Unknown/missing saved resource identifiers");
    auto validToken=[&](const OwnerToken& token){return token.Active()?(w.definitions_.routes.count(token.route)&&w.state_.trains.count(token.train)&&token.generation>0):(!token.train.value&&!token.generation);};
    for(const auto& pair:w.state_.snapshot.blocks)if(!validToken(pair.second.reservation))return fail("Invalid block ownership token");
    for(const auto& pair:w.state_.snapshot.points)if(!validToken(pair.second.lock)||!w.definitions_.points.at(pair.first).positions.count(pair.second.requested))return fail("Invalid point ownership/request");
    for(const auto& pair:w.state_.snapshot.routes)if(!validToken(pair.second.owner))return fail("Invalid route ownership token");
    for(const auto& pair:w.state_.snapshot.authorities)if(!validToken(pair.second.owner))return fail("Invalid authority ownership token");
    for(auto& pair:w.state_.trains){auto& s=pair.second.spatial;auto& h=pair.second.handling;
        if(s.current_.train!=pair.first||h.train_!=pair.first||s.current_.tick!=w.state_.snapshot.tick||s.paused_||!s.error_.empty()||
            s.FixedStepSeconds!=1.0/30.0||s.MaxStepsPerUpdate!=4||s.accumulator_!=0||!h.initialized_||h.source_!=1)return fail("Invalid train identity/clock/configuration");
        auto expected=pair.first.value==2?shortTrain:longTrain;
        if(pair.first.value>1)for(auto& member:expected.members)member.id=VehicleId(member.id.value+100*(pair.first.value-1));
        if(s.formation_.gapMetres!=expected.gapMetres||s.formation_.members.size()!=expected.members.size())return fail("Formation content mismatch");
        for(size_t i=0;i<expected.members.size();++i){const auto& v=s.formation_.members[i];const auto& e=expected.members[i];
            if(v.id!=e.id||v.definition!=e.definition||v.orientation!=e.orientation||!vehicles.insert(v.id).second)return fail("Formation identity/content mismatch");}
        h.formation_=s.formation_;
        auto result=s.history_.Validate(s.graph_);if(!result.ok)return result;
        for(const auto& choice:s.choices_){const auto* t=s.graph_.Find(choice.second);if(!t||!(t->from==choice.first))return fail("Invalid saved path choice");}
        const auto savedHead=s.current_.head,savedLeading=s.current_.leadingEnd;
        result=Place(s.graph_,s.definitions_,s.formation_,s.history_,s.current_.headStationMetres,s.current_);if(!result.ok)return result;
        auto same=[](const TrackPosition& x,const TrackPosition& y){return x.edge==y.edge&&x.distanceMetres==y.distanceMetres&&x.direction==y.direction;};
        if(!same(savedHead,s.current_.head)||!same(savedLeading,s.current_.leadingEnd))return fail("Saved topology position disagrees with path history");
        Snapshot previous=s.current_;result=Place(s.graph_,s.definitions_,s.formation_,s.history_,s.previousHead_,previous);if(!result.ok)return result;
        const auto& state=h.state_;const auto& command=state.command;
        const double minimumThrottle=ScenarioContent::IsTambaram(w.ContentId())?-1:0; // regenerative braking
        if(command.throttle<minimumThrottle||command.throttle>1||command.brake<0||command.brake>1||command.direction< -1||command.direction>1||
            state.appliedThrottle<minimumThrottle||state.appliedThrottle>1||state.appliedBrake<0||state.appliedBrake>1||state.massKg!=s.current_.properties.massKg||
            state.speedMps!=s.current_.commandedSpeedMps||state.lastAppliedSequence>h.lastSubmittedSequence_||h.lastSubmittedSequence_>pair.second.commandSequence||h.queue_.size()>256)return fail("Invalid saved handling state");
        uint64_t lastSequence=state.lastAppliedSequence,lastTick=w.state_.snapshot.tick;
        for(const auto& commandItem:h.queue_){
            if(commandItem.train!=pair.first||commandItem.source!=h.source_||commandItem.sequence<=lastSequence||commandItem.sequence>h.lastSubmittedSequence_||
                commandItem.applyTick<=w.state_.snapshot.tick||commandItem.applyTick<lastTick||commandItem.applyTick-w.state_.snapshot.tick>18000)return fail("Invalid queued train command");
            if(commandItem.kind<CommandKind::Throttle||commandItem.kind>CommandKind::Emergency)return fail("Unknown queued command kind");
            const auto value=commandItem.value;
            if(commandItem.kind==CommandKind::Direction?(value!= -1&&value!=0&&value!=1):(value<(commandItem.kind==CommandKind::Throttle?minimumThrottle:0)||value>1))return fail("Invalid queued command value");
            lastSequence=commandItem.sequence;lastTick=commandItem.applyTick;
        }
    }
    if(w.state_.requests.size()>256||w.state_.aiTrace.size()>32||w.state_.snapshot.events.size()>24)return fail("Saved queue exceeds runtime bound");
    std::map<TrainId,uint64_t> requestSequences;
    for(const auto& request:w.state_.requests){const auto sequence=w.state_.sequences.find(request.train);
        if(!w.state_.trains.count(request.train)||sequence==w.state_.sequences.end()||!request.sequence||request.sequence>sequence->second||
            request.applyTick<=w.state_.snapshot.tick||request.applyTick-w.state_.snapshot.tick>18000||request.kind<OperationsRequestKind::Route||request.kind>OperationsRequestKind::Point||request.sequence<=requestSequences[request.train])return fail("Invalid pending operations request");
        requestSequences[request.train]=request.sequence;
    }
    if(w.state_.aiBindings.size()!=w.state_.drivers.size())return fail("AI binding/driver mismatch");
    for(const auto& pair:w.state_.drivers){const auto& d=pair.second;auto b=w.state_.aiBindings.find(pair.first);
        if(b==w.state_.aiBindings.end()||d.train_!=pair.first||d.route_!=b->second.route||d.binding_!=b->second.generation||!d.binding_||
            d.sequence_!=b->second.acceptedSequence||d.lastTick_>w.state_.snapshot.tick||d.nextRetry_>w.state_.snapshot.tick+90||d.phase_<AIPhase::Holding||d.phase_>AIPhase::FailSafe||
            !w.definitions_.routes.count(d.route_)||w.definitions_.routes.at(d.route_).train!=pair.first)return fail("Invalid AI continuity state");
    }
    // Validate saved occupancy and ownership before replacing derived values.
    // This rejects forged complete/checksummed records instead of laundering them.
    auto result=w.ValidateInvariants();if(!result.ok)return result;
    for(const auto& pair:w.state_.snapshot.routes)if(pair.second.owner.Active()&&pair.second.owner.generation>=w.state_.nextGeneration)return fail("Stale ownership generation counter");
    for(const auto& pair:w.state_.snapshot.routes)if(pair.second.owner.Active()){
        const auto& route=w.definitions_.routes.at(pair.first);const auto& spatial=w.state_.trains.at(pair.second.owner.train).spatial;
        double origin=0,endpoint=0;bool foundOrigin=false,foundEndpoint=false;
        for(const auto& occurrence:spatial.history_.occurrences){
            const auto Station=[&](TrackPosition P){return occurrence.beginMetres+(P.direction==1?P.distanceMetres:occurrence.endMetres-occurrence.beginMetres-P.distanceMetres);};
            if(occurrence.directed==DirectedEdge{route.origin.edge,route.origin.direction}){origin=Station(route.origin);foundOrigin=true;}
            if(occurrence.directed==DirectedEdge{route.endpoint.edge,route.endpoint.direction}){endpoint=Station(route.endpoint);foundEndpoint=true;}
        }
        if(!foundOrigin||!foundEndpoint||endpoint<origin)return fail("Missing route topology in saved history");
        std::vector<DirectedEdge> path;for(const auto& occurrence:spatial.history_.occurrences)if(occurrence.endMetres>origin-1e-6&&occurrence.beginMetres<endpoint-1e-6)path.push_back(occurrence.directed);
        if(path!=route.path)return fail("Saved route owner disagrees with committed branch history");
    }
    result=w.RecomputeOccupancy();if(!result.ok)return result;
    const auto savedAuthorities=w.state_.snapshot.authorities;
    result=w.DeriveAuthorities();if(!result.ok)return result;
    for(const auto& pair:savedAuthorities){const auto& expected=w.state_.snapshot.authorities.at(pair.first).endpoint;
        if(pair.second.endpoint.edge!=expected.edge||pair.second.endpoint.direction!=expected.direction||pair.second.endpoint.distanceMetres!=expected.distanceMetres)return fail("Stale saved authority endpoint");}
    w.DeriveSignals();return w.ValidateInvariants();
}
Result WorldSnapshotCodec::Capture(const OperationsWorld& world,uint64_t generation,SnapshotRecord& out){
    auto candidate=world;auto result=ValidateAndRebuild(candidate);if(!result.ok)return result;
    std::vector<uint8_t> payload;SnapshotArchive archive(payload,false);Transfer(archive,candidate);
    result=archive.Finish();if(!result.ok)return result;
    return SnapshotRecord::Create(generation,payload,world.ContentId(),out);
}
Result WorldSnapshotCodec::Restore(const SnapshotRecord& record,OperationsWorld& out){
    SnapshotRecord verified;auto result=SnapshotRecord::Read(record.Bytes(),verified);if(!result.ok)return result;
    OperationsWorld candidate;result=candidate.InitializeContent(verified.ContentId());if(!result.ok)return result;
    auto payload=verified.Payload();SnapshotArchive archive(payload,true);Transfer(archive,candidate);
    result=archive.Finish();if(!result.ok)return result;
    result=ValidateAndRebuild(candidate);if(!result.ok)return result;
    out=std::move(candidate);return {};
}
TestReport RunWorldSnapshotTests(){
    TestReport report;auto recordCheck=[&](const char* name,bool passed,const std::string& detail=""){report.cases.push_back({name,passed,0,0,detail});};
    OperationsWorld world;auto result=world.InitializeAIFixture();
    for(unsigned i=0;i<30&&result.ok;++i)result=world.Update(1.0/30.0);
    SnapshotRecord saved;result=result.ok?WorldSnapshotCodec::Capture(world,1,saved):result;
    recordCheck("S6-world-capture-AI-wait",result.ok,result.error);
    OperationsWorld restored;if(result.ok)result=WorldSnapshotCodec::Restore(saved,restored);
    recordCheck("S6-world-restore-exact-JSON",result.ok&&world.Json()==restored.Json(),result.error);
    if(!result.ok)return report;
    world.Command(TrainId(1),CommandKind::Throttle,1);
    for(unsigned i=0;i<300&&result.ok;++i)result=world.Update(1.0/30.0);
    world.Command(TrainId(1),CommandKind::Brake,.5);
    result=WorldSnapshotCodec::Capture(world,2,saved);
    if(result.ok)result=WorldSnapshotCodec::Restore(saved,restored);
    recordCheck("S6-world-queued-brake-roundtrip",result.ok&&world.Json()==restored.Json(),result.error);
    bool identical=result.ok;
    for(unsigned i=0;i<300&&identical;++i){identical=world.Update(1.0/30.0).ok&&restored.Update(1.0/30.0).ok;
        SnapshotRecord a,b;identical=identical&&WorldSnapshotCodec::Capture(world,3,a).ok&&WorldSnapshotCodec::Capture(restored,3,b).ok&&a.Bytes()==b.Bytes();}
    recordCheck("S6-world-deterministic-continuation",identical);
    RecoverySession session;result=session.Restore(saved);const auto before=session.World().Json();session.Update(60);
    recordCheck("S6-world-restore-paused",result.ok&&session.Paused()&&session.World().Json()==before);
    session.Resume();session.Update(60);recordCheck("S6-world-resume-discards-inactive-time",session.World().Json()==before&&!session.Paused());
    SnapshotRecord invalid;SnapshotRecord::Create(3,{1,2,3},invalid);const auto stable=restored.Json();
    recordCheck("S6-world-invalid-payload-transactional",!WorldSnapshotCodec::Restore(invalid,restored).ok&&restored.Json()==stable);
    RecoverySelection selection;
    result=SelectSnapshot({saved.Bytes(),invalid.Bytes()},[](const SnapshotRecord& record){OperationsWorld w;return WorldSnapshotCodec::Restore(record,w);},selection);
    recordCheck("S6-world-semantic-fallback",result.ok&&selection.slot==0,result.error);
    const auto negative=WorldSnapshotCodec::RejectionTests();report.cases.insert(report.cases.end(),negative.cases.begin(),negative.cases.end());
    return report;
}
TestReport WorldSnapshotCodec::RejectionTests(){
    TestReport report;OperationsWorld baseline;baseline.InitializeAIFixture();for(int i=0;i<30;++i)baseline.Update(1.0/30.0);
    auto rejected=[&](const char* name,OperationsWorld candidate){
        std::vector<uint8_t> payload;SnapshotArchive writer(payload,false);Transfer(writer,candidate);
        SnapshotRecord record;SnapshotRecord::Create(10,payload,record);auto target=baseline;const auto before=target.Json();
        const auto result=Restore(record,target);report.cases.push_back({name,!result.ok&&target.Json()==before,0,0,result.error});
    };
    auto candidate=baseline;candidate.state_.snapshot.blocks.at(BlockId(1)).occupants.clear();rejected("S6-negative-forged-occupancy",candidate);
    candidate=baseline;candidate.state_.snapshot.points.at(PointId(1)).lock={};rejected("S6-negative-partial-point-lock",candidate);
    candidate=baseline;candidate.state_.snapshot.authorities.at(TrainId(1)).maximumHead+=10;rejected("S6-negative-expanded-authority",candidate);
    candidate=baseline;candidate.state_.snapshot.authorities.at(TrainId(1)).endpoint.edge=EdgeId(99);rejected("S6-negative-stale-authority-edge",candidate);
    candidate=baseline;const auto route=candidate.state_.snapshot.routes.at(RouteId(1));candidate.state_.snapshot.routes.erase(RouteId(1));candidate.state_.snapshot.routes.emplace(RouteId(99),route);rejected("S6-negative-unknown-route-key",candidate);
    candidate=baseline;candidate.state_.drivers.at(TrainId(2)).binding_+=1;rejected("S6-negative-AI-binding",candidate);
    candidate=baseline;candidate.state_.trains.at(TrainId(1)).handling.state_.appliedBrake=2;rejected("S6-negative-invalid-actuator",candidate);
    candidate=baseline;candidate.state_.trains.at(TrainId(1)).spatial.formation_.members[0].id=VehicleId(999);rejected("S6-negative-vehicle-identity",candidate);
    candidate=baseline;candidate.state_.nextGeneration=1;rejected("S6-negative-owner-generation",candidate);
    candidate=baseline;candidate.state_.snapshot.signals.at(SignalId(1)).aspect=SignalAspect::Stop;
    std::vector<uint8_t> payload;SnapshotArchive writer(payload,false);Transfer(writer,candidate);SnapshotRecord record;SnapshotRecord::Create(10,payload,record);
    OperationsWorld rebuilt;const auto result=Restore(record,rebuilt);
    report.cases.push_back({"S6-world-signals-derived-not-trusted",result.ok&&rebuilt.Json()==baseline.Json(),0,0,result.error});
    OperationsWorld moving;moving.InitializeAIFixture();moving.Command(TrainId(1),CommandKind::Throttle,1);bool spanning=false,valid=true;
    for(unsigned i=0;i<1200&&valid;++i){valid=moving.Update(1.0/30.0).ok;
        if(i%90==0){SnapshotRecord checkpoint;OperationsWorld copy;valid=valid&&Capture(moving,20,checkpoint).ok&&Restore(checkpoint,copy).ok&&copy.Json()==moving.Json();}
        const auto& train=moving.Spatial(TrainId(1))->Current();spanning|=train.headStationMetres>240&&train.headStationMetres-train.properties.lengthMetres<240;
    }
    report.cases.push_back({"S6-world-boundary-snapshot-continuity",valid&&spanning,0,0,"Complete long formation snapshots across logical E1-to-upper-route boundary"});
    return report;
}
}
