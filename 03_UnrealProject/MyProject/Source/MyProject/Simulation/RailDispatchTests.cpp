#include "RailDispatchDiagnostics.h"

namespace Rail {
TestReport RunDispatchTests(){
    TestReport report;
    auto RecordCheck=[&](const std::string& name,bool pass,const std::string& detail=""){report.cases.push_back({name,pass,0,0,detail});};
    auto step=[](OperationsWorld& w,unsigned n){for(unsigned i=0;i<n;++i)if(!w.Update(1.0/30).ok)return false;return true;};
    OperationsWorld baseline;bool good=baseline.InitializeAIFixture().ok&&step(baseline,5);
    const auto before=baseline.Json();auto holds=InspectDispatch(baseline.DefinitionsView(),baseline.Current());
    RecordCheck("S9.Dispatch.RealDenialIdentifiesResourceAndOwner",good&&holds.size()==1&&!holds[0].blockers.empty()&&holds[0].Summary().find("T1")!=std::string::npos);
    RecordCheck("S9.Dispatch.ReadOnlyNoSaveStateMutation",baseline.Json()==before);
    RecordCheck("S9.Dispatch.LegitimateWaitNotCycle",holds.size()==1&&!holds[0].cyclic);
    DispatchWatchdog watch(90);watch.Observe(baseline);good=step(baseline,100);holds=watch.Observe(baseline);
    RecordCheck("S9.Dispatch.ProlongedHoldRequiresAttentionNotRelease",good&&holds.size()==1&&holds[0].prolonged&&!holds[0].cyclic&&baseline.Current().routes.at(RouteId(1)).owner.Active());
    baseline.SetSuspended(true);const auto tick=baseline.Current().tick;good=step(baseline,200);auto paused=watch.Observe(baseline);
    RecordCheck("S9.Dispatch.PauseDoesNotAgeWatchdog",good&&baseline.Current().tick==tick&&paused[0].unchangedTicks==holds[0].unchangedTicks);
    OperationsWorld reset;reset.InitializeAIFixture();step(reset,5);holds=watch.Observe(reset);
    RecordCheck("S9.Dispatch.RecoveryClockResetDoesNotFalseAge",holds.size()==1&&!holds[0].prolonged&&holds[0].unchangedTicks==0);
    DispatchWatchdog progress(900);progress.Observe(reset);
    good=reset.Command(TrainId(1),CommandKind::Brake,0).ok&&reset.Command(TrainId(1),CommandKind::Throttle,.5).ok&&step(reset,600);
    holds=progress.Observe(reset);
    RecordCheck("S9.Dispatch.MovingBlockerResetsWaitAge",good&&holds.size()==1&&holds[0].unchangedTicks==0&&!holds[0].prolonged);
    // Deliberately constructed diagnostic-only cycle. Not a permitted railway
    // snapshot and never advanced or used as interaction-coverage evidence.
    auto defs=baseline.DefinitionsView();auto snap=baseline.Current();
    defs.routes.at(RouteId(1)).blocks={BlockId(3)};defs.routes.at(RouteId(1)).points.clear();
    defs.routes.at(RouteId(2)).blocks={BlockId(1)};defs.routes.at(RouteId(2)).points.clear();
    snap.lastRequest[TrainId(1)]={{OperationsRequestKind::Route,TrainId(1),RouteId(1)},false,"fixture occupied",5};
    snap.authorities.at(TrainId(1)).routeGranted=false;
    holds=InspectDispatch(defs,snap);RecordCheck("S9.Dispatch.SyntheticCycleDistinguished",holds.size()==2&&holds[0].cyclic&&holds[1].cyclic);
    // Four actual formations on one authoritative track. Three AI follow each
    // other ahead of the player; long overlapping reservations enforce serial
    // clearance. This logical stress fixture is not a shipped corridor/mission.
    auto create=[](){
        OperationsWorld w;auto stock=MakeFixture();Graph g;
        Geometry geometry{{0,0,0},0,0,0,3000};
        g.nodes={{NodeId(1),{0,0,0}},{NodeId(2),{3000,0,0}}};
        g.edges={{EdgeId(1),NodeId(1),NodeId(2),3000,geometry,true,true}};
        const double bounds[]={0,200,400,600,800,1000,1200,1400,1800,2000,2200,2400,3000};
        for(unsigned i=0;i<12;++i){const unsigned owner=i==0?1:i==2?2:i==4?3:i==6?4:i>=8?i-7:0;
            w.definitions_.blocks.emplace(BlockId(i+1),BlockDefinition{BlockId(i+1),{{EdgeId(1),bounds[i],bounds[i+1]}},{},TrainId(owner),owner!=0});}
        for(unsigned id=1;id<=4;++id){
            const unsigned origin=(id-1)*2+1,dest=id+8;
            std::set<BlockId> blocks;for(unsigned b=origin+1;b<dest;++b)blocks.insert(BlockId(b));
            const RouteId route(10+id);const SignalId signal(10+id);
            w.definitions_.routes.emplace(route,RouteDefinition{route,TrainId(id),{EdgeId(1),bounds[origin],1},{EdgeId(1),bounds[dest]-10,1},{{EdgeId(1),1}},blocks,{},BlockId(origin),BlockId(dest),signal});
            w.definitions_.signals.emplace(signal,SignalDefinition{signal,{EdgeId(1),bounds[origin],1},route});
            OperationsWorld::TrainRuntime t;auto formation=stock.Short();for(auto& v:formation.members)v.id=VehicleId(v.id.value+100*id);
            auto r=t.spatial.Initialize(g,stock.definitions,formation,{EdgeId(1),1},100+400*(id-1),{},TrainId(id));
            if(r.ok)r=t.handling.Initialize(t.spatial,stock.definitions,formation,MakeHandlingDefinitions());
            if(!r.ok){w.error_=r.error;return w;}w.state_.trains.emplace(TrainId(id),std::move(t));
        }
        auto r=w.definitions_.Validate(g);
        for(const auto& b:w.definitions_.blocks)w.state_.snapshot.blocks.emplace(b.first,BlockState{});
        for(const auto& p:w.definitions_.routes)w.state_.snapshot.routes.emplace(p.first,RouteState{});
        for(const auto& s:w.definitions_.signals)w.state_.snapshot.signals.emplace(s.first,SignalState{});
        if(r.ok)r=w.RecomputeOccupancy();if(r.ok)r=w.DeriveAuthorities();w.state_.snapshot.valid=r.ok;w.DeriveSignals();
        if(r.ok)r=w.ValidateInvariants();w.paused_=!r.ok;w.error_=r.error;return w;
    };
    auto fixture=create();RecordCheck("S9.Stress.FourTrainDefinitionsAndWholeTrainOccupancy",!fixture.Faulted()&&fixture.ValidateInvariants().ok,fixture.Error());
    // Declared denominator: six deterministic pacing/departure combinations.
    // An interaction is an actual denied route caused by another train, observed
    // in lastRequest, followed by feasible AI completion. No injected events.
    unsigned interactions=0;
    for(unsigned seed=0;seed<6;++seed){
        auto w=create();bool ok=!w.Faulted();unsigned denied=0;bool emergencySafe=true,pauseSafe=true;
        ok=ok&&w.EnableAI(TrainId(2),RouteId(12)).ok&&w.EnableAI(TrainId(3),RouteId(13)).ok;
        unsigned delayed=30*(seed+1);bool fourth=false;
        // Serial 3 m/s traversals total about 4 km plus acceleration/clearance;
        // 2,000 simulation seconds bounds completion without a speed multiplier.
        for(unsigned i=0;i<60000&&ok;++i){
            if(i==delayed){ok=w.EnableAI(TrainId(4),RouteId(14)).ok;fourth=true;}
            if(i%90==0)ok=ok&&w.RequestRoute(TrainId(1),RouteId(11)).ok;
            if(i==300+seed*30){ok=ok&&w.Command(TrainId(1),CommandKind::Emergency,1).ok&&step(w,2);emergencySafe=emergencySafe&&w.Handling(TrainId(1))->command.emergency&&!w.Handling(TrainId(4))->command.emergency;}
            if(i==360+seed*30)ok=ok&&w.Command(TrainId(1),CommandKind::Emergency,0).ok;
            if(i==600||i==1200){w.SetSuspended(true);const auto frozen=w.Current().tick;ok=ok&&step(w,5);pauseSafe=pauseSafe&&w.Current().tick==frozen;w.SetSuspended(false);}
            // The player uses normal controls and chooses a bounded low speed.
            if(i%3==0&&w.Current().authorities.at(TrainId(1)).routeGranted){
                const auto* h=w.Handling(TrainId(1));const double head=w.Spatial(TrainId(1))->Current().headStationMetres;
                ok=ok&&w.Command(TrainId(1),CommandKind::Brake,head>1860?1:0).ok;
                ok=ok&&w.Command(TrainId(1),CommandKind::Throttle,head<1860&&h->speedMps<(seed%2?2.5:3.0)?1:0).ok;
            }
            ok=ok&&step(w,1);
            if(seed==0&&i==900){auto copy=w,again=w;const bool equal=step(copy,90)&&step(again,90)&&copy.Json()==again.Json();RecordCheck("S9.Stress.DeterministicContinuationFromIdenticalState",equal);}
            for(const auto& h:InspectDispatch(w.DefinitionsView(),w.Current()))if(!h.blockers.empty())++denied;
        }
        bool complete=fourth;for(unsigned id=2;id<=4;++id)complete=complete&&w.AI(TrainId(id))&&w.AI(TrainId(id))->Phase()==AIPhase::Completed&&w.Handling(TrainId(id))->speedMps==0;
        if(ok&&denied>0&&complete)++interactions;
        std::string detail=w.Error()+" denied observations="+std::to_string(denied);
        for(unsigned id=1;id<=4;++id)detail+=" T"+std::to_string(id)+" head="+std::to_string(w.Spatial(TrainId(id))->Current().headStationMetres)+(w.AI(TrainId(id))?" "+std::string(AIPhaseName(w.AI(TrainId(id))->Phase())):"");
        RecordCheck("S9.Stress.Seed"+std::to_string(seed)+".ThreeAICompleteAfterRealFollowingHolds",ok&&denied>0&&complete,detail);
        RecordCheck("S9.Stress.Seed"+std::to_string(seed)+".EmergencyPauseAndSafety",ok&&emergencySafe&&pauseSafe&&w.ValidateInvariants().ok);
    }
    RecordCheck("S9.Stress.DeclaredSixCaseInteractionCoverage",interactions==6,std::to_string(interactions)+"/6 logical following stress cases; not eight missions or rendered coverage");
    return report;
}
}

