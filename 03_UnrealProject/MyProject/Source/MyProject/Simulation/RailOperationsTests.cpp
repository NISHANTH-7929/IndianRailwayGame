#include "RailOperations.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Rail {
TestReport RunOperationsTests(){
    TestReport report;
    auto add=[&](const char* name,bool pass,const std::string& detail=""){report.cases.push_back({name,pass,0,0,detail});};
    const TrainId a(1),b(2);const RouteId ra(1),rb(2);const double dt=1.0/30;
    auto fresh=[](){OperationsWorld w;w.InitializeFixture();return w;};
    auto step=[&](OperationsWorld& w,int count=1){for(int i=0;i<count;++i)if(!w.Update(dt).ok)return false;return true;};
    auto grant=[&](OperationsWorld& w){return w.RequestRoute(a,ra).ok&&step(w)&&w.Current().routes.at(ra).owner.Active();};
    {
        auto w=fresh();w.state_.trains.clear();bool okay=w.RecomputeOccupancy().ok;
        for(const auto& block:w.Current().blocks)okay=okay&&block.second.occupants.empty();add("S4-T01.EmptyInfrastructure",okay);
    }
    {
        auto fixture=MakeFixture();Simulation s;auto defs=MakeOperationsDefinitions();std::set<BlockId> occupied;
        bool okay=s.Initialize(fixture.graph,fixture.definitions,fixture.Long(),{EdgeId(1),1},200,fixture.SelectUpper()).ok;
        okay=okay&&s.AdvanceDistance(330).ok&&FootprintBlocks(defs,s.Network(),s.History(),349,530,occupied).ok;
        add("S4-T02.WholeFormationAcrossEdges",okay&&occupied==std::set<BlockId>{BlockId(5),BlockId(6),BlockId(7)});
        add("S4-T03.FrontPassedRearStillOccupies",occupied.count(BlockId(5))&&occupied.count(BlockId(6)));
        okay=s.AdvanceDistance(160).ok&&FootprintBlocks(defs,s.Network(),s.History(),509,690,occupied).ok;
        add("S4-T04.RearClearanceOccupancy",okay&&occupied==std::set<BlockId>{BlockId(7)});
    }
    auto granted=fresh();bool good=grant(granted);
    add("S4-T05.ClearRouteGrant",good&&granted.ValidateInvariants().ok);
    {
        auto w=granted;good=w.RequestRoute(b,rb).ok&&step(w);
        add("S4-T06.ConflictDenied",good&&!w.Current().lastRequest.at(b).granted&&!w.Current().routes.at(rb).owner.Active());
        const auto token=w.Current().routes.at(ra).owner;
        add("S4-T08.AtomicLocks",token.Active()&&w.Current().blocks.at(BlockId(2)).reservation==token&&w.Current().blocks.at(BlockId(5)).reservation==token&&
            w.Current().blocks.at(BlockId(6)).reservation==token&&w.Current().points.at(PointId(1)).lock==token&&w.Current().points.at(PointId(2)).lock==token);
        add("S4-T09.NoPartialFailedGrant",!w.Current().blocks.at(BlockId(4)).reservation.Active()&&w.Current().points.at(PointId(2)).lock==token);
        good=w.RequestPoint(b,PointId(2),0).ok&&step(w);
        add("S4-T10.PointRouteLocked",good&&!w.Current().lastRequest.at(b).granted&&w.Current().points.at(PointId(2)).position==1);
    }
    {
        auto w=granted;good=w.RequestRoute(b,RouteId(3)).ok&&step(w);
        add("S4-T07.NonconflictingCoexist",good&&w.Current().routes.at(ra).owner.Active()&&w.Current().routes.at(RouteId(3)).owner.Active()&&w.ValidateInvariants().ok);
    }
    {
        auto w=fresh();auto& s=w.state_.trains.at(b).spatial;good=s.AdvanceDistance(40).ok&&w.RecomputeOccupancy().ok;
        const int before=w.Current().points.at(PointId(2)).position;
        auto r=w.Evaluate({OperationsRequestKind::Point,b,{},PointId(2),1,1,1,0});
        add("S4-T11.PointOccupancyIndependentOfLock",good&&!r.ok&&w.Current().points.at(PointId(2)).position==before);
    }
    add("S4-T12.TrainBoundAuthority",granted.Current().authorities.at(a).train==a&&granted.Current().authorities.at(a).owner.train==a&&
        granted.Current().authorities.at(a).routeGranted&&!granted.Current().authorities.at(b).routeGranted);
    {
        auto w=fresh();w.Command(a,CommandKind::Throttle,1);good=step(w,1800);
        add("S4-T13.RestrictiveBoundaryServiceBraking",good&&w.Spatial(a)->Current().headStationMetres<220&&w.Handling(a)->speedMps==0&&!w.Faulted());
        // Adversarial fast train immediately before the boundary: candidate must roll back, not set velocity to zero.
        auto q=fresh();q.Command(a,CommandKind::Throttle,1);step(q,120);
        auto& s=q.state_.trains.at(a).spatial;s.AdvanceDistance(219.999-s.Current().headStationMetres);q.RecomputeOccupancy();q.DeriveAuthorities();
        const auto old=s.Current();const double speed=q.Handling(a)->speedMps;auto r=q.Update(dt);
        add("S4.AuthorityHardGuardRetainsPhysicalState",!r.ok&&q.Faulted()&&q.Spatial(a)->Current().tick==old.tick&&q.Spatial(a)->Current().headStationMetres==old.headStationMetres&&q.Handling(a)->speedMps==speed&&speed>0,q.Error());
    }
    add("S4-T14.DerivedSignal",granted.Current().signals.at(SignalId(1)).aspect==SignalAspect::Proceed&&granted.Current().signals.at(SignalId(2)).aspect==SignalAspect::Stop);
    {
        auto w=fresh();w.state_.snapshot.signals.at(SignalId(1)).aspect=SignalAspect::Proceed;w.Command(a,CommandKind::Throttle,1);good=step(w,1800);
        add("S4-T15.StaleSignalCannotAuthorize",good&&!w.Current().authorities.at(a).routeGranted&&w.Spatial(a)->Current().headStationMetres<220&&w.Current().signals.at(SignalId(1)).aspect==SignalAspect::Stop);
    }
    {
        bool all=true;for(int damaged=0;damaged<5;++damaged){auto w=granted;
            if(damaged==0)w.state_.snapshot.blocks.erase(BlockId(6));
            if(damaged==1)w.state_.snapshot.points.erase(PointId(2));
            if(damaged==2)w.state_.snapshot.authorities.erase(a);
            if(damaged==3)w.state_.snapshot.routes.erase(ra);
            if(damaged==4)w.state_.snapshot.signals.erase(SignalId(1));
            const auto tick=w.Current().tick;all=all&&!w.Update(dt).ok&&w.Faulted()&&w.Current().tick==tick;
        }add("S4-T16.MissingStateFailsSafe",all);
    }
    {
        auto w=granted;const auto token=w.Current().routes.at(ra).owner;
        const bool stale=!w.Submit({OperationsRequestKind::Route,a,ra,{},0,1,w.Current().tick+1,0}).ok;
        w.CancelRoute(a,ra,token.generation+1);good=step(w);
        add("S4-T17.StaleRequestsCannotSteal",stale&&good&&w.Current().routes.at(ra).owner==token&&!w.Current().lastRequest.at(a).granted);
        w.CancelRoute(a,ra,token.generation);good=step(w);w.RequestRoute(a,ra);good=good&&step(w);
        const auto newer=w.Current().routes.at(ra).owner;w.CancelRoute(a,ra,token.generation);good=good&&step(w);
        add("S4.StaleGenerationCannotReleaseNewOwner",good&&newer.generation>token.generation&&w.Current().routes.at(ra).owner==newer);
    }
    {
        auto w=granted;w.Command(a,CommandKind::Throttle,.5);bool heldAfterFront=false,released=false,all=true;double releaseHead=0;
        for(int i=0;i<9000;++i){if(i==4500){w.Command(a,CommandKind::Brake,0);w.Command(a,CommandKind::Throttle,1);}if(!step(w)){all=false;break;}const double head=w.Spatial(a)->Current().headStationMetres;
            if(head>501.3275&&head<682.3274)heldAfterFront=heldAfterFront||w.Current().routes.at(ra).owner.Active();
            if(!w.Current().routes.at(ra).owner.Active()){released=true;releaseHead=head;break;}}
        add("S4-T18.WholeTrainRouteRelease",all&&heldAfterFront&&released&&releaseHead>682.327412287, w.Error()+" release head="+std::to_string(releaseHead)+" final head="+std::to_string(w.Spatial(a)->Current().headStationMetres));
        good=w.RequestRoute(b,rb).ok&&step(w);add("S4.CompetingRouteGrantedAfterRearClear",good&&w.Current().routes.at(rb).owner.Active()&&w.Current().points.at(PointId(2)).position==0);
        add("S4-T19.ForwardRetainsCommittedHistory",all&&w.Spatial(a)->History().occurrences.size()==6&&w.Spatial(a)->History().occurrences[1].directed.edge==EdgeId(3));
    }
    {
        auto w=granted;w.Command(a,CommandKind::Throttle,.5);good=step(w,1800);w.Command(a,CommandKind::Throttle,0);w.Command(a,CommandKind::Brake,1);good=good&&step(w,600);
        const auto tape=w.Spatial(a)->History().occurrences;const double before=w.Spatial(a)->Current().headStationMetres;
        w.Command(a,CommandKind::Direction,-1);step(w);w.Command(a,CommandKind::Brake,0);w.Command(a,CommandKind::Throttle,.25);good=good&&step(w,600);
        bool same=tape.size()==w.Spatial(a)->History().occurrences.size();for(size_t i=0;same&&i<tape.size();++i)same=tape[i].directed==w.Spatial(a)->History().occurrences[i].directed;
        add("S4-T20.ReverseRetainsHistory",good&&same&&w.Spatial(a)->Current().headStationMetres<before,w.Error());
    }
    {
        auto w=fresh();auto& s=w.state_.trains.at(a).spatial;s.SetChoice({EdgeId(1),1},TransitionId(1));s.CommitHistoryExtent(19,300);
        const auto before=s.History().occurrences;w.RequestRoute(a,ra);good=step(w);
        add("S4-T21.CannotRewriteCommittedBranch",good&&!w.Current().lastRequest.at(a).granted&&!w.Current().routes.at(ra).owner.Active()&&w.Spatial(a)->History().occurrences.size()==before.size()&&w.Spatial(a)->History().occurrences[1].directed.edge==EdgeId(2));
    }
    {
        auto w=granted;w.Command(a,CommandKind::Throttle,1);good=step(w,300);const double speed=w.Handling(a)->speedMps;
        w.Command(a,CommandKind::Throttle,0);w.Command(a,CommandKind::Brake,1);good=good&&step(w,300);
        add("S4-T22.S3HandlingUnderAuthority",good&&speed>1&&w.Handling(a)->speedMps<speed);
    }
    {
        std::vector<std::string> results;bool all=true;
        for(int hz:{30,60,144}){auto w=fresh();w.RequestRoute(b,rb);w.RequestRoute(a,ra);w.Command(a,CommandKind::Throttle,.5);
            for(int frame=0;frame<hz*30;++frame)all=all&&w.Update(1.0/hz).ok;results.push_back(w.Json());}
        add("S4-T23.Render30_60_144",all&&results[0]==results[1]&&results[1]==results[2]);
    }
    {
        auto w=granted;w.Command(a,CommandKind::Throttle,1);step(w,60);w.SetSuspended(true);const auto before=w.Json();good=w.Update(1000).ok&&before==w.Json();
        const auto tick=w.Current().tick;w.SetSuspended(false);good=good&&w.Update(1000).ok&&w.Current().tick==tick&&step(w);
        add("S4-T24.SuspendNoSafetyTransitions",good&&w.Handling(a)->command.throttle==0&&w.Handling(a)->command.brake==1);
    }
    {
        auto w=granted;auto view=w.Current();const auto before=w.Json();view={};
        add("S4-T25.PurePresentationDiscard",before==w.Json(),"Actual Unreal Actor destruction requires separate runtime evidence");
        view=w.Current();add("S4-T26.PurePresentationReconstruction",view.signals.at(SignalId(1)).aspect==SignalAspect::Proceed&&view.points.at(PointId(2)).lock==w.Current().points.at(PointId(2)).lock,"Actual Unreal Actor recreation requires separate runtime evidence");
    }
    const auto kernel=RunKernelTests(),handling=RunHandlingTests();
    add("S4-T27.FullS2Regression39",kernel.Passed()&&kernel.cases.size()==39);
    add("S4-T28.FullS3Regression28",handling.Passed()&&handling.cases.size()==28);
    {
        bool all=true;for(int order=0;order<2;++order){auto w=fresh();if(order){w.RequestRoute(a,ra);w.RequestRoute(b,rb);}else{w.RequestRoute(b,rb);w.RequestRoute(a,ra);}
            all=all&&step(w)&&w.Current().routes.at(ra).owner.Active()&&!w.Current().routes.at(rb).owner.Active()&&w.ValidateInvariants().ok;}
        add("S4.SimultaneousRequestOrderStable",all,"Same apply tick ordered by TrainId then sequence");
    }
    {
        auto w=granted;const auto tick=w.Current().tick;add("S4.ClockOverloadRetainsDebt",!w.Update(1).ok&&w.Faulted()&&w.Current().tick==tick&&w.accumulator_==1);
        auto invalid=MakeOperationsDefinitions();invalid.blocks.erase(BlockId(6));add("S4.DefinitionCoverageFailsSafe",!invalid.Validate(MakeFixture().graph).ok);
        auto forged=granted;forged.state_.snapshot.authorities.at(a).maximumHead+=20;add("S4.ForgedAuthorityRejected",!forged.Update(dt).ok&&forged.Faulted());
        auto partial=granted;partial.state_.snapshot.blocks.at(BlockId(6)).reservation={};add("S4.PartialOwnerRejected",!partial.Update(dt).ok&&partial.Faulted());
    }
    {
        auto fixture=MakeFixture();auto definitions=MakeOperationsDefinitions();PathHistory tape;
        bool okay=tape.Initialize(fixture.graph,{EdgeId(1),1}).ok&&tape.Cover(fixture.graph,0,600,fixture.SelectUpper()).ok;
        std::set<BlockId> swept;okay=okay&&FootprintBlocks(definitions,fixture.graph,tape,210,520,swept).ok;
        add("S4.SweptIntermediateResources",okay&&swept.count(BlockId(2))&&swept.count(BlockId(5))&&swept.count(BlockId(6)),"Sweep retains a block crossed completely between endpoints");
        auto w=granted;w.state_.snapshot.blocks.at(BlockId(6)).occupants.insert(b);
        add("S4.OccupancyCorruptionRejected",!w.Update(dt).ok&&w.Faulted());
        auto reverse=fresh();reverse.Command(a,CommandKind::Direction,-1);reverse.Command(a,CommandKind::Throttle,1);okay=step(reverse,1500);
        add("S4.ReverseRearBoundary",okay&&reverse.Spatial(a)->Current().headStationMetres>=181&&reverse.Handling(a)->speedMps==0);
    }
    return report;
}
}


