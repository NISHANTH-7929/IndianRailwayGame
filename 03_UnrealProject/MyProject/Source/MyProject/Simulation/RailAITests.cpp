#include "RailOperations.h"
#include <cmath>
#include <limits>
#include <type_traits>

namespace Rail {
TestReport RunAITests(){
    TestReport report;auto add=[&](const char* name,bool pass,const std::string& detail=""){report.cases.push_back({name,pass,0,0,detail});};
    const TrainId a(1),b(2);const RouteId ra(1),rb(2);
    auto fresh=[](){OperationsWorld w;w.InitializeAIFixture();return w;};
    auto step=[](OperationsWorld& w,int count){for(int i=0;i<count;++i)if(!w.Update(1.0/30).ok)return false;return true;};
    auto w=fresh();auto obs=w.ObserveAI(b);AIDriver driver(b,rb,1);const auto before=w.Json();auto intent=driver.Decide(obs);
    add("S5-T01.AuthoritativeObservation",obs.valid&&obs.train==b&&obs.head==100&&obs.length==43&&obs.authority.valid);
    add("S5-T02.SemanticControls",intent.controls.size()==2&&intent.controls[0].kind==CommandKind::Throttle&&intent.controls[0].value==0&&intent.controls[1].kind==CommandKind::Brake&&intent.controls[1].value==1);
    add("S5-T03.ValueObservationCannotMoveTrain",w.Json()==before&&std::is_same<decltype(w.Spatial(b)),const Simulation*>::value);
    bool okay=step(w,2);
    add("S5-T04.RequestThroughS4",okay&&w.Current().lastRequest.count(b)&&w.Current().lastRequest.at(b).request.route==rb);
    add("S5-T05.DenialObserved",!w.Current().lastRequest.at(b).granted&&!w.ObserveAI(b).lastRequest.reason.empty());
    okay=step(w,87);
    add("S5-T06.SafeWait",okay&&w.Handling(b)->speedMps==0&&w.Spatial(b)->Current().headStationMetres==100&&w.AI(b)->Phase()==AIPhase::WaitingForRoute);
    add("S5-T07.ThrottledRetry",w.state_.sequences.at(b)==1);
    okay=step(w,4);add("S5.RetryAtFixedTick",okay&&w.state_.sequences.at(b)==2);
    auto freeWorld=[](){OperationsWorld q;q.InitializeFixture();q.EnableAI(TrainId(2),RouteId(2));return q;};
    auto free=freeWorld();okay=step(free,4);
    add("S5-T08.GrantReceived",okay&&free.Current().authorities.at(b).routeGranted&&free.AI(b)->Phase()==AIPhase::RouteGranted);
    auto permitted=free.ObserveAI(b);
    add("S5-T09.MatchingPermission",permitted.resourcesOwned&&permitted.authority.owner.train==b&&permitted.signal.aspect==SignalAspect::Proceed);
    auto restrictive=permitted;restrictive.signal.aspect=SignalAspect::Stop;AIDriver restrictedDriver(b,rb,1);auto stop=restrictedDriver.Decide(restrictive);
    add("S5-T10.RestrictiveSignal",restrictedDriver.Phase()==AIPhase::RestrictiveApproach&&stop.controls[1].value==1);
    okay=step(free,1800);
    add("S5-T11.EndpointStop",okay&&free.Handling(b)->speedMps==0&&free.Spatial(b)->Current().headStationMetres<165&&free.Spatial(b)->Current().headStationMetres>130,free.Error());
    auto absent=permitted;absent.authority.valid=false;AIDriver noAuthority(b,rb,1);stop=noAuthority.Decide(absent);
    add("S5-T12.NoPermissionBrakes",noAuthority.Phase()==AIPhase::FailSafe&&stop.controls[1].value==1);
    add("S5-T13.PlayerBlocksAI",w.Current().routes.at(ra).owner.train==a&&!w.Current().routes.at(rb).owner.Active()&&w.Handling(b)->speedMps==0);
    add("S5-T14.NoConflictingPermission",w.ValidateInvariants().ok&&free.ValidateInvariants().ok);
    auto moving=freeWorld();okay=step(moving,150);
    add("S5-T15.S3ForcesDriveAI",okay&&moving.Handling(b)->lastAppliedSequence>0&&moving.Handling(b)->speedMps>0&&moving.Handling(b)->massKg==120000);
    add("S5-T16.S2Traversal",free.Spatial(b)->Current().tick==free.Current().tick&&free.Spatial(b)->History().occurrences.size()>=2&&free.Spatial(b)->Current().headStationMetres>100);
    std::set<BlockId> occupied;const auto* spatial=free.Spatial(b);
    okay=FootprintBlocks(free.DefinitionsView(),spatial->Network(),spatial->History(),spatial->Current().headStationMetres-43,spatial->Current().headStationMetres,occupied).ok;
    for(auto id:occupied)okay=okay&&free.Current().blocks.at(id).occupants.count(b);
    add("S5-T17.WholeFormationOccupancy",okay&&occupied.count(BlockId(3))&&occupied.count(BlockId(4)));
    // The short terminal cannot clear B's rear: the driver must never release its locks.
    add("S5.BRetainsUnclearedRoute",free.Current().routes.at(rb).owner.Active()&&free.Current().routes.at(rb).entered);
    // Player operates the same commands. A's rear, not AI intent, causes the handoff.
    auto interaction=fresh();interaction.Command(a,CommandKind::Brake,0);interaction.Command(a,CommandKind::Throttle,.5);
    okay=step(interaction,4500);const bool held=interaction.Current().routes.at(ra).owner.Active()&&interaction.Handling(b)->speedMps==0;
    interaction.Command(a,CommandKind::Brake,0);interaction.Command(a,CommandKind::Throttle,1);
    okay=okay&&step(interaction,1800);
    add("S5-T18.RearControlsRelease",okay&&held&&!interaction.Current().routes.at(ra).owner.Active()&&interaction.Current().routes.at(ra).releaseReason=="whole formation rear cleared all protected resources",interaction.Error());
    add("S5-T19.AutonomousResumeAfterPlayer",okay&&interaction.Current().routes.at(rb).owner.train==b&&interaction.Spatial(b)->Current().headStationMetres>130);
    auto repeated=freeWorld();step(repeated,1804);
    add("S5-T20.RepeatedRunDeterminism",repeated.Json()==free.Json());
    auto slow=freeWorld(),fast=freeWorld();for(int i=0;i<300;++i)slow.Update(1.0/15);for(int i=0;i<1200;++i)fast.Update(1.0/60);
    add("S5-T21.RenderRateIndependence",slow.Current().tick==600&&slow.Json()==fast.Json());
    // Actual Actor recreation is verified in Unreal/Android, not claimed by this value-copy check.
    auto copied=interaction;step(copied,60);step(interaction,60);add("S5.LogicalCopyReconstruction",copied.Json()==interaction.Json());
    auto missing=moving;missing.state_.drivers.erase(b);okay=step(missing,300);
    add("S5-T23.MissingDriverBrakes",okay&&missing.Handling(b)->speedMps==0&&missing.Handling(b)->command.brake==1);
    auto stale=fresh();auto decision=stale.state_.drivers.at(b).Decide(stale.ObserveAI(b));okay=stale.DispatchAI(decision).ok;auto unchanged=stale.Json();
    add("S5-T24.StaleIntentRejected",okay&&!stale.DispatchAI(decision).ok&&stale.Json()==unchanged);
    auto corrupt=permitted;corrupt.head=std::numeric_limits<double>::quiet_NaN();AIDriver invalid(b,rb,1);stop=invalid.Decide(corrupt);
    add("S5.CorruptObservationLatched",invalid.Phase()==AIPhase::FailSafe&&stop.controls[1].value==1&&invalid.Decide(permitted).controls[1].value==1);
    add("S5.PlayerCannotOverrideAI",!moving.Command(b,CommandKind::Throttle,1).ok&&!moving.RequestRoute(b,rb).ok);
    OperationsWorld upper;upper.InitializeFixture();okay=upper.EnableAI(a,ra).ok&&step(upper,7500);
    add("S5.AITraversesCurvesAndCompletesAfterRearClear",okay&&upper.Spatial(a)->Current().head.edge==EdgeId(7)&&
        upper.Spatial(a)->Current().headStationMetres>682.327412&&!upper.Current().routes.at(ra).owner.Active()&&upper.AI(a)->Phase()==AIPhase::Completed&&upper.Handling(a)->speedMps==0,upper.Error());
    auto bindingGuard=freeWorld();add("S5.DuplicateBindingRejected",!bindingGuard.EnableAI(b,rb).ok);
    OperationsWorld pending;pending.InitializeFixture();pending.RequestRoute(b,rb);add("S5.PendingPlayerIntentPreventsBinding",!pending.EnableAI(b,rb).ok);
    auto rollback=moving;const auto prior=rollback.Json();const auto aiBefore=rollback.AI(b)->Json();
    rollback.state_.snapshot.blocks.at(BlockId(4)).reservation={RouteId(1),a,999};auto fail=rollback.Update(1.0/30);
    add("S5.InvalidInfrastructureRetainsAIAndPosition",!fail.ok&&rollback.AI(b)->Json()==aiBefore&&rollback.Spatial(b)->Current().headStationMetres==moving.Spatial(b)->Current().headStationMetres);
    const auto frozen=moving.Current().tick;moving.SetSuspended(true);okay=step(moving,100);moving.SetSuspended(false);okay=okay&&step(moving,1);
    add("S5.PauseFreezesDecisionClock",okay&&moving.Current().tick==frozen);
    add("S5-T25.S2Regression",RunKernelTests().Passed());add("S5-T26.S3Regression",RunHandlingTests().Passed());add("S5-T27.S4Regression",RunOperationsTests().Passed());
    return report;
}
}
