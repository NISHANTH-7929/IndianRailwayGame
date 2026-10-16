#include "RailOperations.h"
#include "RailScenarioContent.h"
#include "RailTambaramDriving.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace Rail {
namespace {
constexpr double Epsilon=1e-8;
bool Touch(double a,double b,double c,double d){return a<=d+Epsilon&&c<=b+Epsilon;}
bool Other(const std::set<TrainId>& occupants,TrainId train){for(auto t:occupants)if(t!=train)return true;return false;}
Result Station(const PathHistory& history,TrackPosition p,double& out){
    bool found=false;
    for(const auto& o:history.occurrences)if(o.directed.edge==p.edge&&o.directed.direction==p.direction){
        if(found)return Result::Fail("Ambiguous repeated route boundary");
        out=o.beginMetres+(p.direction==1?p.distanceMetres:o.endMetres-o.beginMetres-p.distanceMetres);found=true;
    }
    return found?Result{}:Result::Fail("Boundary absent from committed history");
}
std::string Quote(const std::string& s){std::string r="\"";for(char c:s){if(c=='\"'||c=='\\')r+='\\';if(c=='\n'){r+="\\n";continue;}r+=c;}return r+'\"';}
}
OperationsDefinitions MakeOperationsDefinitions(){
    OperationsDefinitions d;
    auto block=[&](uint64_t id,std::vector<TrackRange> coverage,uint64_t train=0){
        BlockId b(id);d.blocks.emplace(b,BlockDefinition{b,std::move(coverage),{},TrainId(train)});
    };
    block(1,{{EdgeId(1),0,220}},1);block(2,{{EdgeId(1),220,240}});
    block(3,{{EdgeId(2),0,130}},2);block(4,{{EdgeId(2),130,160}});
    block(5,{{EdgeId(3),0,20*Pi},{EdgeId(4),0,20*Pi},{EdgeId(5),0,20*Pi},{EdgeId(6),0,20*Pi}});
    block(6,{{EdgeId(7),0,10}});block(7,{{EdgeId(7),10,220}},1);
    d.blocks.at(BlockId(4)).conflicts.insert(BlockId(5));d.blocks.at(BlockId(5)).conflicts.insert(BlockId(4));
    d.points.emplace(PointId(1),PointDefinition{PointId(1),{{0,{TransitionId(1)}},{1,{TransitionId(2)}}},{BlockId(2),BlockId(5)}});
    d.points.emplace(PointId(2),PointDefinition{PointId(2),{{0,{TransitionId(7)}},{1,{TransitionId(6)}}},{BlockId(4),BlockId(5),BlockId(6)}});
    d.routes.emplace(RouteId(1),RouteDefinition{RouteId(1),TrainId(1),{EdgeId(1),220,1},{EdgeId(7),219,1},
        {{EdgeId(1),1},{EdgeId(3),1},{EdgeId(4),1},{EdgeId(5),1},{EdgeId(6),1},{EdgeId(7),1}},
        {BlockId(2),BlockId(5),BlockId(6)},{{PointId(1),1},{PointId(2),1}},BlockId(1),BlockId(7),SignalId(1)});
    d.routes.emplace(RouteId(2),RouteDefinition{RouteId(2),TrainId(2),{EdgeId(2),130,1},{EdgeId(7),5,1},
        {{EdgeId(2),1},{EdgeId(7),1}},{BlockId(4),BlockId(6)},{{PointId(2),0}},BlockId(3),{},SignalId(2)});
    d.routes.emplace(RouteId(3),RouteDefinition{RouteId(3),TrainId(2),{EdgeId(2),100,1},{EdgeId(2),120,1},
        {{EdgeId(2),1}},{BlockId(3)},{},BlockId(3),{},SignalId(3)});
    for(const auto& pair:d.routes){const auto& r=pair.second;d.signals.emplace(r.signal,SignalDefinition{r.signal,r.origin,r.id});}
    return d;
}
Result OperationsDefinitions::Validate(const Graph& graph) const {
    auto fail=[](const char* s){return Result::Fail(s);};
    if(!graph.Validate().ok||blocks.empty()||routes.empty()||signals.empty())return fail("Missing safety definitions");
    std::map<EdgeId,std::vector<std::pair<double,double>>> coverage;
    for(const auto& pair:blocks){const auto& b=pair.second;
        if(!b.id.value||pair.first!=b.id||b.coverage.empty())return fail("Invalid block identity/coverage");
        for(size_t i=0;i<b.coverage.size();++i){const auto& c=b.coverage[i];const auto* edge=graph.Find(c.edge);
            if(!edge||!std::isfinite(c.from)||!std::isfinite(c.to)||c.from<0||c.to>edge->lengthMetres||c.to<=c.from)return fail("Invalid block extent");
            if(i){const auto& previous=b.coverage[i-1];const auto* p=graph.Find(previous.edge);
                if(previous.to!=p->lengthMetres||c.from!=0||!graph.Connection({previous.edge,1},{c.edge,1}))return fail("Disconnected ordered block coverage");}
            coverage[c.edge].push_back({c.from,c.to});
        }
        for(auto c:b.conflicts){auto f=blocks.find(c);if(c==b.id||f==blocks.end()||!f->second.conflicts.count(b.id))return fail("Invalid/asymmetric block conflict");}
    }
    for(const auto& edge:graph.edges){auto spans=coverage[edge.id];std::sort(spans.begin(),spans.end());double end=0;
        for(auto span:spans){if(std::abs(span.first-end)>Epsilon)return fail("Missing/overlapping safety coverage");end=span.second;}
        if(std::abs(end-edge.lengthMetres)>Epsilon)return fail("Unprotected track extent");
    }
    for(const auto& pair:points){const auto& p=pair.second;
        if(!p.id.value||pair.first!=p.id||p.positions.empty()||p.protectedBlocks.empty())return fail("Invalid point definition");
        for(auto b:p.protectedBlocks)if(!blocks.count(b))return fail("Missing point protection");
        for(const auto& pos:p.positions){if(pos.second.empty())return fail("Empty point position");for(auto t:pos.second)if(!graph.Find(t))return fail("Unknown point transition");}
    }
    // A continuation is not a switch. Every genuine facing/trailing branch,
    // however, needs one governing point with mutually exclusive positions.
    std::map<DirectedEdge,std::vector<const Transition*>> outgoing;
    for(const auto& t:graph.transitions)outgoing[t.from].push_back(&t);
    for(const auto& branch:outgoing)if(branch.second.size()>1){
        bool protectedBranch=false;
        for(const auto& pair:points){
            std::set<int> positions;bool complete=true;
            for(const auto* t:branch.second){
                const auto* reverse=graph.Connection(t->to.Reversed(),t->from.Reversed());
                unsigned matches=0;int selected=0;
                for(const auto& position:pair.second.positions){
                    const auto& ids=position.second;
                    if(std::find(ids.begin(),ids.end(),t->id)!=ids.end()||
                       (reverse&&std::find(ids.begin(),ids.end(),reverse->id)!=ids.end())){++matches;selected=position.first;}
                }
                if(matches!=1||!positions.insert(selected).second){complete=false;break;}
            }
            if(complete){protectedBranch=true;break;}
        }
        if(!protectedBranch)return fail("Branch lacks mutually exclusive governing point positions");
    }
    for(const auto& pair:routes){const auto& r=pair.second;Sample sample;
        if(!r.id.value||pair.first!=r.id||!r.train.value||r.path.empty()||r.blocks.empty()||!signals.count(r.signal)||
           !graph.SamplePosition(r.origin,sample).ok||!graph.SamplePosition(r.endpoint,sample).ok||
           !(r.path.front()==DirectedEdge{r.origin.edge,r.origin.direction})||!(r.path.back()==DirectedEdge{r.endpoint.edge,r.endpoint.direction}))return fail("Invalid route boundaries/identity");
        std::set<DirectedEdge> seen;
        for(size_t i=0;i<r.path.size();++i){if(!seen.insert(r.path[i]).second||(r.path[i].direction!=1&&r.path[i].direction!=-1))return fail("Routes require unique valid directed occurrences");
            if(i&&!graph.Connection(r.path[i-1],r.path[i]))return fail("Disconnected route");}
        for(auto b:r.blocks){auto f=blocks.find(b);if(f==blocks.end()||(f->second.exclusiveTo.value&&f->second.exclusiveTo!=r.train&&!f->second.transitAllowed))return fail("Invalid route resource");}
        auto home=blocks.find(r.originHolding);if(home==blocks.end()||home->second.exclusiveTo!=r.train)return fail("Missing train-assigned origin");
        if(r.destinationHolding.value){auto dest=blocks.find(r.destinationHolding);if(dest==blocks.end()||dest->second.exclusiveTo!=r.train)return fail("Invalid destination holding area");}
        for(auto p:r.points){auto f=points.find(p.first);if(f==points.end()||!f->second.positions.count(p.second))return fail("Unknown required point position");
            bool used=false;for(auto transition:f->second.positions.at(p.second)){const auto* t=graph.Find(transition);
                for(size_t i=1;i<r.path.size();++i)if(r.path[i-1]==t->from&&r.path[i]==t->to)used=true;}
            if(!used)return fail("Point position does not serve route");}
        for(const auto& p:points)for(const auto& position:p.second.positions)for(auto transition:position.second){const auto* t=graph.Find(transition);
            for(size_t i=1;i<r.path.size();++i)if(r.path[i-1]==t->from&&r.path[i]==t->to){auto required=r.points.find(p.first);
                if(required==r.points.end()||required->second!=position.first)return fail("Route omits governing point lock");}}
    }
    for(const auto& pair:signals){const auto& s=pair.second;auto route=routes.find(s.route);
        if(!s.id.value||s.id!=pair.first||route==routes.end()||route->second.signal!=s.id||s.boundary.edge!=route->second.origin.edge||
           s.boundary.distanceMetres!=route->second.origin.distanceMetres||s.boundary.direction!=route->second.origin.direction)return fail("Invalid signal association");}
    return {};
}
Result FootprintBlocks(const OperationsDefinitions& definitions,const Graph& graph,const PathHistory& history,
                       double rear,double head,std::set<BlockId>& out){
    out.clear();if(!std::isfinite(rear)||!std::isfinite(head)||rear>head||history.occurrences.empty()||!history.Validate(graph).ok||
        rear<history.occurrences.front().beginMetres-Epsilon||head>history.occurrences.back().endMetres+Epsilon)return Result::Fail("Invalid/uncovered formation footprint");
    for(const auto& occurrence:history.occurrences){if(!Touch(rear,head,occurrence.beginMetres,occurrence.endMetres))continue;
        double a=std::max(rear,occurrence.beginMetres)-occurrence.beginMetres,b=std::min(head,occurrence.endMetres)-occurrence.beginMetres;
        if(occurrence.directed.direction<0){const double length=occurrence.endMetres-occurrence.beginMetres;const double old=a;a=length-b;b=length-old;}
        double covered=a;std::vector<std::pair<double,double>> spans;
        for(const auto& pair:definitions.blocks)for(const auto& c:pair.second.coverage)if(c.edge==occurrence.directed.edge&&Touch(a,b,c.from,c.to)){
            out.insert(pair.first);spans.push_back({std::max(a,c.from),std::min(b,c.to)});}
        std::sort(spans.begin(),spans.end());if(spans.empty())return Result::Fail("Missing logical safety coverage");
        for(auto span:spans){if(span.first>covered+Epsilon)return Result::Fail("Gap in footprint safety coverage");covered=std::max(covered,span.second);}
        if(covered<b-Epsilon)return Result::Fail("Incomplete footprint safety coverage");
    }
    return {};
}
const Simulation* OperationsWorld::Spatial(TrainId train) const {auto f=state_.trains.find(train);return f==state_.trains.end()?nullptr:&f->second.spatial;}
const LongitudinalState* OperationsWorld::Handling(TrainId train) const {auto f=state_.trains.find(train);return f==state_.trains.end()?nullptr:&f->second.handling.Current();}
void OperationsWorld::Event(const std::string& event){auto& q=state_.snapshot.events;q.push_back(std::to_string(state_.snapshot.tick+1)+": "+event);if(q.size()>24)q.pop_front();}
Result OperationsWorld::InitializeFixture(){
    return InitializeContent(FormationContent::LegacyId);
}
Result OperationsWorld::InitializeContent(const std::string& contentId){
    Fixture fixture;Formation shortTrain,longTrain;auto r=ScenarioContent::Resolve(contentId,fixture,shortTrain,longTrain);if(!r.ok)return r;
    const auto selectedContent=contentId;
    *this=OperationsWorld{};contentId_=selectedContent;definitions_=ScenarioContent::OperationsFor(contentId);r=definitions_.Validate(fixture.graph);if(!r.ok){error_=r.error;return r;}
    for(uint64_t id=1;id<=ScenarioContent::TrainCount(contentId);++id){TrainRuntime t;auto formation=id==2?shortTrain:longTrain;
        if(id>1)for(auto& member:formation.members)member.id=VehicleId(member.id.value+100*(id-1));
        const bool Opposing=ScenarioContent::IsTambaram(contentId)&&id==3;
        const EdgeId Initial=Opposing?TambaramRoute::EdgeFor(0,TambaramRoute::SegmentCount-1):ScenarioContent::IsTambaram(contentId)&&id==1?TambaramRoute::EdgeFor(1,4):EdgeId(id==1?1:2);
        r=t.spatial.Initialize(fixture.graph,fixture.definitions,formation,{Initial,Opposing?-1:1},ScenarioContent::Start(contentId,id),ScenarioContent::IsS8Route(contentId)?Choices{}:fixture.SelectUpper(id==1),TrainId(id));
        auto handling=MakeHandlingDefinitions();
        if(ScenarioContent::UsesLhb(contentId_)){auto coach=handling.at(DefinitionId(2));coach.id=DefinitionId(FormationContent::LhbDefinitionId);handling.emplace(coach.id,coach);auto loco=handling.at(DefinitionId(1));loco.id=DefinitionId(FormationContent::Wap7DefinitionId);handling.emplace(loco.id,loco);}
        if(ScenarioContent::IsTambaram(contentId_))handling=TambaramDriving::Definitions();
        if(r.ok)r=t.handling.Initialize(t.spatial,fixture.definitions,formation,handling);if(!r.ok){error_=r.error;return r;}
        state_.trains.emplace(TrainId(id),std::move(t));
    }
    for(const auto& b:definitions_.blocks)state_.snapshot.blocks.emplace(b.first,BlockState{});
    for(const auto& p:definitions_.points)state_.snapshot.points.emplace(p.first,PointState{});
    for(const auto& route:definitions_.routes)state_.snapshot.routes.emplace(route.first,RouteState{});
    for(const auto& s:definitions_.signals)state_.snapshot.signals.emplace(s.first,SignalState{});
    r=RecomputeOccupancy();if(r.ok)r=DeriveAuthorities();state_.snapshot.valid=r.ok;DeriveSignals();if(r.ok)r=ValidateInvariants();
    paused_=!r.ok;error_=r.error;return r;
}
Result OperationsWorld::RecomputeOccupancy(){
    if(state_.snapshot.blocks.size()!=definitions_.blocks.size())return Result::Fail("Missing block runtime state");
    for(auto& b:state_.snapshot.blocks)b.second.occupants.clear();
    for(const auto& pair:state_.trains){const auto& s=pair.second.spatial;std::set<BlockId> occupied;
        auto r=FootprintBlocks(definitions_,s.Network(),s.History(),s.Current().headStationMetres-s.Current().properties.lengthMetres,s.Current().headStationMetres,occupied);if(!r.ok)return r;
        for(auto b:occupied){auto f=state_.snapshot.blocks.find(b);if(f==state_.snapshot.blocks.end())return Result::Fail("Missing occupied block");f->second.occupants.insert(pair.first);}
    }return {};
}
Result OperationsWorld::DeriveAuthorities(){
    state_.snapshot.authorities.clear();
    for(const auto& pair:state_.trains){MovementAuthority authority;authority.train=pair.first;
        const auto& s=pair.second.spatial;const auto& snap=s.Current();
        for(const auto& route:state_.snapshot.routes)if(route.second.owner.Active()&&route.second.owner.train==pair.first){
            if(authority.valid)return Result::Fail("Multiple route authorities for train");auto def=definitions_.routes.find(route.first);
            if(def==definitions_.routes.end())return Result::Fail("Unknown active route");const auto& d=def->second;
            const auto& home=definitions_.blocks.at(d.originHolding).coverage.front();
            const int direction=d.origin.direction;
            auto r=Station(s.History(),{home.edge,direction==1?home.from:home.to,direction},authority.minimumRear);if(!r.ok)return r;
            r=Station(s.History(),d.endpoint,authority.maximumHead);if(!r.ok)return r;
            authority.valid=true;authority.routeGranted=true;authority.owner=route.second.owner;authority.endpoint=d.endpoint;authority.restriction="Route corridor; S3 braking approaches endpoint";
        }
        if(!authority.valid)for(const auto& b:definitions_.blocks)if(b.second.exclusiveTo==pair.first&&b.second.coverage.size()==1){
            const auto& c=b.second.coverage.front();double begin=0,end=0;
            int direction=1;for(const auto& occurrence:s.History().occurrences)if(occurrence.directed.edge==c.edge)direction=occurrence.directed.direction;
            if(Station(s.History(),{c.edge,direction==1?c.from:c.to,direction},begin).ok&&Station(s.History(),{c.edge,direction==1?c.to:c.from,direction},end).ok&&
                snap.headStationMetres-snap.properties.lengthMetres>=begin-Epsilon&&snap.headStationMetres<=end+Epsilon){
                authority.valid=true;authority.minimumRear=begin;authority.maximumHead=end;authority.endpoint={c.edge,direction==1?c.to:c.from,direction};
                authority.restriction="Restricted train-assigned holding area; no route granted";break;
            }
        }
        if(!authority.valid)authority.restriction="Denied: no complete valid corridor";
        state_.snapshot.authorities.emplace(pair.first,authority);
    }return {};
}
Result OperationsWorld::Submit(const OperationsRequest& request){
    if(state_.aiBindings.count(request.train))return Result::Fail("Train route intent owned by AI");
    return SubmitRequest(request);
}
Result OperationsWorld::SubmitRequest(const OperationsRequest& request){
    if(paused_||suspended_||!state_.snapshot.valid)return Result::Fail("Operations unavailable");
    if(!state_.trains.count(request.train)||!request.sequence||request.sequence<=state_.sequences[request.train]||
        request.applyTick<=state_.snapshot.tick||request.applyTick-state_.snapshot.tick>18000||state_.requests.size()>=256)return Result::Fail("Invalid train, stale sequence/tick or full request queue");
    if(request.kind!=OperationsRequestKind::Route&&request.kind!=OperationsRequestKind::Cancel&&request.kind!=OperationsRequestKind::Point)return Result::Fail("Unknown operations request");
    state_.sequences[request.train]=request.sequence;state_.requests.push_back(request);return {};
}
Result OperationsWorld::RequestRoute(TrainId t,RouteId r){return Submit({OperationsRequestKind::Route,t,r,{},0,state_.sequences[t]+1,state_.snapshot.tick+1,0});}
Result OperationsWorld::RequestPoint(TrainId t,PointId p,int position){return Submit({OperationsRequestKind::Point,t,{},p,position,state_.sequences[t]+1,state_.snapshot.tick+1,0});}
Result OperationsWorld::CancelRoute(TrainId t,RouteId r,uint64_t generation){return Submit({OperationsRequestKind::Cancel,t,r,{},0,state_.sequences[t]+1,state_.snapshot.tick+1,generation});}
Result OperationsWorld::Command(TrainId train,CommandKind kind,double value){
    if(state_.aiBindings.count(train))return Result::Fail("Train controls owned by AI");
    return SubmitControl(train,kind,value);
}
Result OperationsWorld::SubmitControl(TrainId train,CommandKind kind,double value){
    auto t=state_.trains.find(train);if(paused_||suspended_||!state_.snapshot.valid||t==state_.trains.end())return Result::Fail("Train control unavailable");
    return t->second.handling.Submit(t->second.spatial,{train,t->second.handling.Source(),++t->second.commandSequence,state_.snapshot.tick+1,kind,value});
}
Result OperationsWorld::Grant(const OperationsRequest& request){
    auto found=definitions_.routes.find(request.route);if(found==definitions_.routes.end()||found->second.train!=request.train)return Result::Fail("Unknown route or wrong requesting train");
    const auto& d=found->second;auto& s=state_.trains.at(request.train).spatial;
    for(const auto& r:state_.snapshot.routes)if(r.second.owner.Active()&&r.second.owner.train==request.train)return Result::Fail("Train already owns a route");
    std::set<BlockId> footprint;auto check=FootprintBlocks(definitions_,s.Network(),s.History(),s.Current().headStationMetres-s.Current().properties.lengthMetres,s.Current().headStationMetres,footprint);
    if(!check.ok)return check;
    if(footprint!=std::set<BlockId>{d.originHolding})return Result::Fail("Grant requires entire formation inside assigned origin");
    double origin=0;if(!Station(s.History(),d.origin,origin).ok||s.Current().headStationMetres>origin+Epsilon)return Result::Fail("Train already beyond route origin");
    for(auto b:d.blocks){auto state=state_.snapshot.blocks.find(b);if(state==state_.snapshot.blocks.end())return Result::Fail("Missing required block state");
        if(state->second.reservation.Active()||Other(state->second.occupants,request.train))return Result::Fail("Required block occupied or reserved");
        for(auto conflict:definitions_.blocks.at(b).conflicts){auto f=state_.snapshot.blocks.find(conflict);
            if(f==state_.snapshot.blocks.end()||f->second.reservation.Active()||!f->second.occupants.empty())return Result::Fail("Conflicting block occupied or reserved");}
    }
    for(auto required:d.points){auto p=state_.snapshot.points.find(required.first);if(p==state_.snapshot.points.end()||p->second.lock.Active())return Result::Fail("Required point missing or route-locked");
        for(auto b:definitions_.points.at(required.first).protectedBlocks){auto f=state_.snapshot.blocks.find(b);
            if(f==state_.snapshot.blocks.end()||!f->second.occupants.empty()||f->second.reservation.Active())return Result::Fail("Point protected area occupied or reserved");}
    }
    for(size_t i=1;i<d.path.size();++i){const auto* t=s.Network().Connection(d.path[i-1],d.path[i]);if(!t)return Result::Fail("Route path disconnected");check=s.SetChoice(t->from,t->id);if(!check.ok)return check;}
    const double originOffset=d.origin.direction==1?d.origin.distanceMetres:s.Network().Find(d.origin.edge)->lengthMetres-d.origin.distanceMetres;
    double endpoint=origin-originOffset;
    for(size_t i=0;i+1<d.path.size();++i)endpoint+=s.Network().Find(d.path[i].edge)->lengthMetres;
    endpoint+=d.endpoint.direction==1?d.endpoint.distanceMetres:s.Network().Find(d.endpoint.edge)->lengthMetres-d.endpoint.distanceMetres;
    check=s.CommitHistoryExtent(s.Current().headStationMetres-s.Current().properties.lengthMetres,endpoint);if(!check.ok)return check;
    double actual=0;if(!Station(s.History(),d.endpoint,actual).ok||std::abs(actual-endpoint)>Epsilon)return Result::Fail("Committed history disagrees with requested route");
    std::vector<DirectedEdge> actualPath;for(const auto& occurrence:s.History().occurrences)if(occurrence.endMetres>origin-Epsilon&&occurrence.beginMetres<endpoint-Epsilon)actualPath.push_back(occurrence.directed);
    if(actualPath!=d.path)return Result::Fail("Cannot rewrite committed branch history");
    std::set<BlockId> corridor;check=FootprintBlocks(definitions_,s.Network(),s.History(),origin,endpoint,corridor);if(!check.ok)return check;
    for(auto b:corridor){const auto& block=definitions_.blocks.at(b);
        if(block.exclusiveTo.value){if(block.exclusiveTo!=request.train&&(!block.transitAllowed||!d.blocks.count(b)))return Result::Fail("Route enters another train holding area without reserved transit");}
        else if(!d.blocks.count(b))return Result::Fail("Route omits traversed protected resource");}
    OwnerToken token{d.id,request.train,state_.nextGeneration++};
    for(auto b:d.blocks)state_.snapshot.blocks.at(b).reservation=token;
    for(auto p:d.points){auto& point=state_.snapshot.points.at(p.first);point.position=p.second;point.requested=p.second;point.lock=token;}
    state_.snapshot.routes.at(d.id)={token,false,{}};Event("granted R"+std::to_string(d.id.value)+" to T"+std::to_string(request.train.value));return {};
}
void OperationsWorld::Release(RouteId route,const std::string& reason){
    const auto token=state_.snapshot.routes.at(route).owner;
    for(auto& b:state_.snapshot.blocks)if(b.second.reservation==token)b.second.reservation={};
    for(auto& p:state_.snapshot.points)if(p.second.lock==token)p.second.lock={};
    state_.snapshot.routes.at(route)={{},false,reason};Event("released R"+std::to_string(route.value)+": "+reason);
}
Result OperationsWorld::Evaluate(const OperationsRequest& request){
    if(request.kind==OperationsRequestKind::Route){
        // Include path history, points, reservations and generation in the same transaction.
        auto candidate=*this;auto r=candidate.Grant(request);if(r.ok)state_=std::move(candidate.state_);return r;
    }
    if(request.kind==OperationsRequestKind::Point){auto f=state_.snapshot.points.find(request.point);auto d=definitions_.points.find(request.point);
        if(f==state_.snapshot.points.end()||d==definitions_.points.end()||!d->second.positions.count(request.position))return Result::Fail("Unknown point/position");
        f->second.requested=request.position;if(f->second.lock.Active())return Result::Fail("Point route-locked");
        for(auto b:d->second.protectedBlocks){auto block=state_.snapshot.blocks.find(b);
            if(block==state_.snapshot.blocks.end()||!block->second.occupants.empty()||block->second.reservation.Active())return Result::Fail("Point protected by occupancy/reservation");}
        f->second.position=request.position;return {};
    }
    auto route=state_.snapshot.routes.find(request.route);if(route==state_.snapshot.routes.end()||!route->second.owner.Active()||route->second.owner.train!=request.train||
        route->second.owner.generation!=request.expectedGeneration)return Result::Fail("Stale/wrong route release token");
    const auto& t=state_.trains.at(request.train);const auto& d=definitions_.routes.at(request.route);std::set<BlockId> occupied;
    auto r=FootprintBlocks(definitions_,t.spatial.Network(),t.spatial.History(),t.spatial.Current().headStationMetres-t.spatial.Current().properties.lengthMetres,t.spatial.Current().headStationMetres,occupied);
    if(!r.ok)return r;
    if(route->second.entered||std::abs(t.handling.Current().speedMps)>.02||occupied!=std::set<BlockId>{d.originHolding})return Result::Fail("Cancellation unsafe: require stopped pre-entry formation in origin");
    Release(request.route,"safe pre-entry cancellation");return {};
}
Result OperationsWorld::ValidateInvariants() const {
    auto fail=[](const char* s){return Result::Fail(s);};
    if(!state_.snapshot.valid||state_.trains.empty())return fail("Missing valid operations state");
    if(state_.snapshot.blocks.size()!=definitions_.blocks.size()||state_.snapshot.points.size()!=definitions_.points.size()||
       state_.snapshot.routes.size()!=definitions_.routes.size()||state_.snapshot.signals.size()!=definitions_.signals.size()||
       state_.snapshot.authorities.size()!=state_.trains.size())return fail("Incomplete safety runtime state");
    Occupancy expected;for(const auto& b:definitions_.blocks)expected.emplace(b.first,std::set<TrainId>{});
    for(const auto& t:state_.trains){const auto& s=t.second.spatial;std::set<BlockId> blocks;
        if(s.Current().train!=t.first||s.Current().tick!=state_.snapshot.tick)return fail("Train identity/tick mismatch");
        auto r=FootprintBlocks(definitions_,s.Network(),s.History(),s.Current().headStationMetres-s.Current().properties.lengthMetres,s.Current().headStationMetres,blocks);if(!r.ok)return r;
        for(auto b:blocks)expected.at(b).insert(t.first);
        auto a=state_.snapshot.authorities.find(t.first);if(a==state_.snapshot.authorities.end()||!a->second.valid||a->second.train!=t.first||
            !std::isfinite(a->second.minimumRear)||!std::isfinite(a->second.maximumHead)||
            s.Current().headStationMetres>a->second.maximumHead+Epsilon||s.Current().headStationMetres-s.Current().properties.lengthMetres<a->second.minimumRear-Epsilon)return fail("Invalid authority/formation outside corridor");
    }
    for(const auto& pair:definitions_.blocks){auto f=state_.snapshot.blocks.find(pair.first);if(f==state_.snapshot.blocks.end()||f->second.occupants!=expected.at(pair.first))return fail("Occupancy differs from complete formation");
        const auto& b=f->second;if(b.occupants.size()>1)return fail("Multiple trains physically occupy one block");
        if(pair.second.exclusiveTo.value&&Other(b.occupants,pair.second.exclusiveTo)&&
           (!pair.second.transitAllowed||!b.reservation.Active()||Other(b.occupants,b.reservation.train)))return fail("Wrong train in exclusive holding area");
        if(b.reservation.Active()){auto r=state_.snapshot.routes.find(b.reservation.route);
            if(r==state_.snapshot.routes.end()||r->second.owner!=b.reservation||!definitions_.routes.at(r->first).blocks.count(pair.first)||Other(b.occupants,b.reservation.train))return fail("Orphan/conflicting block owner");}
        for(auto c:pair.second.conflicts){auto f2=state_.snapshot.blocks.find(c);if(f2==state_.snapshot.blocks.end())return fail("Missing conflict state");
            if(b.reservation.Active()&&f2->second.reservation.Active()&&b.reservation!=f2->second.reservation)return fail("Conflicting reservations");
            if(!b.occupants.empty()&&Other(f2->second.occupants,*b.occupants.begin()))return fail("Conflicting physical footprints");
            if(b.reservation.Active()&&Other(f2->second.occupants,b.reservation.train))return fail("Conflicting occupancy/ownership");}
    }
    for(const auto& pair:state_.snapshot.points){auto d=definitions_.points.find(pair.first);if(d==definitions_.points.end()||!d->second.positions.count(pair.second.position))return fail("Invalid point runtime");
        if(pair.second.lock.Active()){auto route=state_.snapshot.routes.find(pair.second.lock.route);
            if(route==state_.snapshot.routes.end()||route->second.owner!=pair.second.lock)return fail("Orphan point lock");const auto& required=definitions_.routes.at(route->first).points;
            auto p=required.find(pair.first);if(p==required.end()||p->second!=pair.second.position)return fail("Locked point position mismatch");}
    }
    for(const auto& pair:state_.snapshot.routes)if(pair.second.owner.Active()){auto d=definitions_.routes.find(pair.first);const auto& token=pair.second.owner;
        if(d==definitions_.routes.end()||token.route!=pair.first||token.train!=d->second.train||!token.generation)return fail("Invalid route owner");
        for(auto b:d->second.blocks)if(state_.snapshot.blocks.at(b).reservation!=token)return fail("Partial route block grant");
        for(auto p:d->second.points)if(state_.snapshot.points.at(p.first).lock!=token||state_.snapshot.points.at(p.first).position!=p.second)return fail("Partial route point grant");
        auto a=state_.snapshot.authorities.find(token.train);if(a==state_.snapshot.authorities.end()||!a->second.routeGranted||a->second.owner!=token)return fail("Route/authority owner mismatch");
    }
    auto derived=*this;auto r=derived.DeriveAuthorities();if(!r.ok)return r;
    for(const auto& a:state_.snapshot.authorities){const auto& correct=derived.state_.snapshot.authorities.at(a.first);
        if(a.second.valid!=correct.valid||a.second.routeGranted!=correct.routeGranted||a.second.owner!=correct.owner||
            a.second.minimumRear!=correct.minimumRear||a.second.maximumHead!=correct.maximumHead)return fail("Stale/forged authority");}
    return {};
}
Result OperationsWorld::Step(){
    auto valid=ValidateInvariants();if(!valid.ok)return valid;
    std::stable_sort(state_.requests.begin(),state_.requests.end(),[](const OperationsRequest& a,const OperationsRequest& b){
        if(a.applyTick!=b.applyTick)return a.applyTick<b.applyTick;if(a.train!=b.train)return a.train<b.train;return a.sequence<b.sequence;});
    while(!state_.requests.empty()&&state_.requests.front().applyTick<=state_.snapshot.tick+1){const auto request=state_.requests.front();state_.requests.pop_front();
        const auto result=Evaluate(request);state_.snapshot.lastRequest[request.train]={request,result.ok,result.error,state_.snapshot.tick+1};if(!result.ok)Event("denied T"+std::to_string(request.train.value)+": "+result.error);}
    valid=DeriveAuthorities();if(!valid.ok)return valid;
    DeriveSignals();valid=UpdateAI();if(!valid.ok)return valid;
    const auto oldBlocks=state_.snapshot.blocks;
    for(auto& pair:state_.trains){auto& t=pair.second;const auto& a=state_.snapshot.authorities.at(pair.first);if(!a.valid)return Result::Fail("Movement denied: missing authority");
        const auto before=t.spatial.Current();const double speed=t.handling.Current().speedMps;
        const int direction=std::abs(speed)>1e-8?(speed>0?1:-1):t.handling.Current().command.direction;
        const double distance=direction>=0?a.maximumHead-before.headStationMetres:before.headStationMetres-before.properties.lengthMetres-a.minimumRear;
        // Provisional fixture envelope uses S3 service brakes, never assigns position or velocity.
        const double StopEnvelope=ScenarioContent::IsTambaram(contentId_)?TambaramDriving::AuthorityStoppingDistance(speed):speed*speed/(2*.55)+std::abs(speed)*2+2;
        if(distance<=StopEnvelope){
            auto r=SubmitControl(pair.first,CommandKind::Throttle,0);if(r.ok)r=SubmitControl(pair.first,CommandKind::Brake,1);if(!r.ok)return r;
        }
        auto r=t.spatial.AdvanceLogicalStep([&](Simulation& s){return t.handling.AdvanceFixedStep(s,1.0/30.0);});if(!r.ok)return r;
        const double after=t.spatial.Current().headStationMetres;
        const double rear=std::min(before.headStationMetres,after)-before.properties.lengthMetres,head=std::max(before.headStationMetres,after);
        if(rear<a.minimumRear-Epsilon||head>a.maximumHead+Epsilon)return Result::Fail("Authority boundary guard: candidate rejected; physical state retained");
        std::set<BlockId> swept;r=FootprintBlocks(definitions_,t.spatial.Network(),t.spatial.History(),rear,head,swept);if(!r.ok)return r;
        for(auto b:swept){const auto& def=definitions_.blocks.at(b);const auto& block=oldBlocks.at(b);
            if(Other(block.occupants,pair.first)||(def.exclusiveTo.value&&def.exclusiveTo!=pair.first&&!def.transitAllowed))return Result::Fail("Swept movement meets occupied/exclusive block");
            if((!def.exclusiveTo.value||def.exclusiveTo!=pair.first)&&(!a.routeGranted||block.reservation!=a.owner))return Result::Fail("Swept movement lacks route resource ownership");
            for(auto c:def.conflicts)if(Other(oldBlocks.at(c).occupants,pair.first)||(oldBlocks.at(c).reservation.Active()&&oldBlocks.at(c).reservation!=a.owner))return Result::Fail("Swept conflicting movement rejected");
        }
    }
    valid=RecomputeOccupancy();if(!valid.ok)return valid;
    for(auto& pair:state_.snapshot.routes)if(pair.second.owner.Active()){
        const auto& d=definitions_.routes.at(pair.first);bool occupied=false;
        for(auto b:d.blocks)if(state_.snapshot.blocks.at(b).occupants.count(pair.second.owner.train))occupied=true;
        if(d.blocks!=std::set<BlockId>{d.originHolding}){if(occupied)pair.second.entered=true;
            if(pair.second.entered&&!occupied)Release(pair.first,"whole formation rear cleared all protected resources");}
    }
    ++state_.snapshot.tick;valid=DeriveAuthorities();if(!valid.ok)return valid;DeriveSignals();return ValidateInvariants();
}
Result OperationsWorld::Update(double dt){
    metrics_={};
    if(!std::isfinite(dt)||dt<0)return Result::Fail("Invalid operations delta");
    if(suspended_)return {};if(skipResumeDelta_){skipResumeDelta_=false;dt=0;}
    auto r=AdvanceFixedClock(dt,1.0/30.0,4,accumulator_,paused_,error_,[&]{
        auto candidate=*this;candidate.metrics_={};const auto start=std::chrono::steady_clock::now();auto result=candidate.Step();
        metrics_.stepMilliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();++metrics_.steps;
        metrics_.aiMilliseconds+=candidate.metrics_.aiMilliseconds;metrics_.decisions+=candidate.metrics_.decisions;
        if(result.ok)state_=std::move(candidate.state_);return result;
    });
    if(!r.ok){state_.snapshot.valid=false;for(auto& a:state_.snapshot.authorities){a.second.valid=false;a.second.restriction=r.error;}DeriveSignals();}
    return r;
}
void OperationsWorld::CancelInput(){for(auto& pair:state_.trains)pair.second.handling.CancelInput();}
void OperationsWorld::SetSuspended(bool value){if(value&&!suspended_){CancelInput();state_.requests.clear();}if(!value&&suspended_)skipResumeDelta_=true;suspended_=value;}
std::string OperationsWorld::Json() const {
    const auto& s=state_.snapshot;std::ostringstream o;o<<std::setprecision(17)<<"{\"tick\":"<<s.tick<<",\"valid\":"<<(s.valid?"true":"false")<<",\"faulted\":"<<(paused_?"true":"false")
      <<",\"suspended\":"<<(suspended_?"true":"false")<<",\"error\":"<<Quote(error_)<<",\"trains\":[";
    bool comma=false;for(const auto& pair:state_.trains){if(comma)o<<',';comma=true;o<<"{\"id\":"<<pair.first.value<<",\"handling\":"<<pair.second.handling.Json(pair.second.spatial.Current());
        auto a=s.authorities.find(pair.first);
        if(a!=s.authorities.end()){
            o<<",\"authorityValid\":"<<(a->second.valid?"true":"false")<<",\"routeGranted\":"<<(a->second.routeGranted?"true":"false")
             <<",\"route\":"<<a->second.owner.route.value<<",\"generation\":"<<a->second.owner.generation<<",\"minimumRear\":"<<a->second.minimumRear<<",\"maximumHead\":"<<a->second.maximumHead<<",\"restriction\":"<<Quote(a->second.restriction);
        }
        o<<'}';
    }
    o<<"],\"ai\":[";comma=false;for(const auto& ai:state_.drivers){if(comma)o<<',';comma=true;o<<ai.second.Json();}
    o<<"],\"aiTraceHash\":"<<state_.traceHash<<",\"aiTrace\":[";comma=false;for(const auto& entry:state_.aiTrace){if(comma)o<<',';comma=true;o<<entry;}
    o<<"],\"blocks\":[";comma=false;for(const auto& b:s.blocks){if(comma)o<<',';comma=true;o<<"{\"id\":"<<b.first.value<<",\"route\":"<<b.second.reservation.route.value<<",\"owner\":"<<b.second.reservation.train.value<<",\"occupants\":[";bool c=false;for(auto t:b.second.occupants){if(c)o<<',';c=true;o<<t.value;}o<<"]}";}
    o<<"],\"points\":[";comma=false;for(const auto& p:s.points){if(comma)o<<',';comma=true;o<<"{\"id\":"<<p.first.value<<",\"position\":"<<p.second.position<<",\"requested\":"<<p.second.requested<<",\"route\":"<<p.second.lock.route.value<<",\"owner\":"<<p.second.lock.train.value<<'}';}
    o<<"],\"routes\":[";comma=false;for(const auto& route:s.routes){if(comma)o<<',';comma=true;o<<"{\"id\":"<<route.first.value<<",\"owner\":"<<route.second.owner.train.value<<",\"generation\":"<<route.second.owner.generation<<",\"entered\":"<<(route.second.entered?"true":"false")<<",\"releaseReason\":"<<Quote(route.second.releaseReason)<<'}';}
    o<<"],\"signals\":[";comma=false;for(const auto& sig:s.signals){if(comma)o<<',';comma=true;o<<"{\"id\":"<<sig.first.value<<",\"proceed\":"<<(sig.second.aspect==SignalAspect::Proceed?"true":"false")<<",\"route\":"<<sig.second.route.value<<",\"train\":"<<sig.second.train.value<<'}';}
    o<<"],\"requests\":[";comma=false;for(const auto& req:s.lastRequest){if(comma)o<<',';comma=true;o<<"{\"train\":"<<req.first.value<<",\"route\":"<<req.second.request.route.value<<",\"granted\":"<<(req.second.granted?"true":"false")<<",\"reason\":"<<Quote(req.second.reason)<<'}';}
    o<<"],\"events\":[";comma=false;for(const auto& e:s.events){if(comma)o<<',';comma=true;o<<Quote(e);}o<<"]}";return o.str();
}
void OperationsWorld::DeriveSignals(){
    for(auto& pair:state_.snapshot.signals){SignalState next;next.revision=state_.snapshot.tick;auto d=definitions_.signals.find(pair.first);
        if(d!=definitions_.signals.end()){next.route=d->second.route;auto r=state_.snapshot.routes.find(next.route);
            if(state_.snapshot.valid&&r!=state_.snapshot.routes.end()&&r->second.owner.Active()){
                next.train=r->second.owner.train;auto a=state_.snapshot.authorities.find(next.train);const auto* s=Spatial(next.train);double boundary=0;
                if(a!=state_.snapshot.authorities.end()&&a->second.valid&&a->second.routeGranted&&a->second.owner==r->second.owner&&s&&
                    Station(s->History(),d->second.boundary,boundary).ok&&s->Current().headStationMetres<=boundary+Epsilon)next.aspect=SignalAspect::Proceed;
            }
        }pair.second=next;
    }
}
}


