#include "RailHandling.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Rail {
namespace {
Fixture StraightFixture(double grade=0){
    auto fixture=MakeFixture();Geometry g{{-1000,0,0},0,0,grade,5000};
    fixture.graph.nodes={{NodeId(1),g.At(0).position},{NodeId(2),g.At(5000).position}};
    fixture.graph.edges={{EdgeId(1),NodeId(1),NodeId(2),5000,g,true,true}};fixture.graph.transitions.clear();return fixture;
}
struct Rig {
    Simulation spatial;HandlingModel handling;uint64_t sequence=0;
    bool Initialize(const Fixture& f,Formation formation,HandlingDefinitions definitions=MakeHandlingDefinitions(),double head=1200){
        return spatial.Initialize(f.graph,f.definitions,formation,{EdgeId(1),1},head,
            f.graph.transitions.empty()?Choices{}:f.SelectUpper()).ok&&handling.Initialize(spatial,f.definitions,formation,std::move(definitions)).ok;
    }
    bool Command(CommandKind kind,double value,uint64_t tick=0){
        return handling.Submit(spatial,{TrainId(1),1,++sequence,tick?tick:spatial.Current().tick+1,kind,value}).ok;
    }
    bool Steps(int count){for(int i=0;i<count;++i)if(!handling.Update(spatial,spatial.FixedStepSeconds).ok)return false;return true;}
};
HandlingDefinitions AnalyticDefinitions(){
    auto d=MakeHandlingDefinitions();for(auto& entry:d){auto& p=entry.second;p.rollingNPerKg=0;p.aeroNPerMpsSquared=0;
        p.tractionResponseSeconds=p.brakeApplySeconds=p.brakeReleaseSeconds=p.emergencyApplySeconds=1e-6;
        p.serviceBrakeNPerKg=1;p.emergencyBrakeNPerKg=1.5;p.adhesionCoefficient=1;}
    d.at(DefinitionId(1)).maxTractionN=120000;d.at(DefinitionId(1)).maxPowerWatts=1e12;d.at(DefinitionId(1)).tractionCutoffMps=1e6;return d;
}
}
TestReport RunHandlingTests(){
    TestReport report;auto add=[&](const char* name,bool passed,double error=0,double tolerance=0,const char* detail=""){
        report.cases.push_back({name,passed,error,tolerance,detail});};
    const auto flat=StraightFixture();Rig zero;bool ok=zero.Initialize(flat,flat.Long())&&zero.Steps(300);
    add("S3-T01.StationaryZeroCommand",ok&&zero.handling.Current().speedMps==0&&zero.spatial.Current().headStationMetres==1200);
    Rig powered;ok=powered.Initialize(flat,flat.Long())&&powered.Command(CommandKind::Throttle,1);
    const bool deferred=powered.handling.Current().command.throttle==0&&powered.handling.Current().speedMps==0;
    ok&=powered.Steps(1);add("S3-T02.ThrottleNextFixedStep",ok&&deferred&&powered.handling.Current().accelerationMps2>0&&powered.handling.Current().tractionN>0);
    ok&=powered.Steps(299)&&powered.Command(CommandKind::Throttle,0)&&powered.Steps(30);const double coastStart=powered.handling.Current().speedMps;
    ok&=powered.Steps(60);add("S3-T03.CoastingResistance",ok&&powered.handling.Current().speedMps<coastStart&&powered.handling.Current().tractionN==0&&powered.handling.Current().resistanceN<0);
    const double beforeBrake=powered.handling.Current().speedMps;ok&=powered.Command(CommandKind::Brake,1)&&powered.Steps(90);
    add("S3-T04.BrakingReducesSpeed",ok&&powered.handling.Current().speedMps<beforeBrake&&powered.handling.Current().brakingN<=0);
    Rig seed;ok=seed.Initialize(flat,flat.Long())&&seed.Command(CommandKind::Throttle,1)&&seed.Steps(300)&&seed.Command(CommandKind::Throttle,0)&&seed.Steps(30);
    Rig mild=seed,strong=seed;ok&=mild.Command(CommandKind::Brake,.3)&&strong.Command(CommandKind::Brake,1)&&mild.Steps(60)&&strong.Steps(60);
    add("S3-T05.StrongerBrake",ok&&strong.handling.Current().accelerationMps2<mild.handling.Current().accelerationMps2&&strong.handling.Current().speedMps<mild.handling.Current().speedMps);
    ok&=strong.Steps(600);const double stop=strong.spatial.Current().headStationMetres;ok&=strong.Steps(300);
    add("S3-T06.StopsWithoutBounce",ok&&strong.handling.Current().speedMps==0&&strong.spatial.Current().headStationMetres==stop);
    Rig light,heavy;ok=light.Initialize(flat,flat.Short())&&heavy.Initialize(flat,flat.Long())&&light.Command(CommandKind::Throttle,1)&&heavy.Command(CommandKind::Throttle,1)&&light.Steps(90)&&heavy.Steps(90);
    add("S3-T07.TotalFormationMass",ok&&light.handling.Current().massKg==120000&&heavy.handling.Current().massKg==360000&&light.handling.Current().speedMps>heavy.handling.Current().speedMps);
    auto up=StraightFixture(.02),down=StraightFixture(-.02);Rig uphill,downhill;
    ok=uphill.Initialize(up,up.Long())&&downhill.Initialize(down,down.Long())&&uphill.Steps(30)&&downhill.Steps(30);
    add("S3-T08.SignedGradeRollback",ok&&uphill.handling.Current().speedMps<0&&downhill.handling.Current().speedMps>0&&std::abs(uphill.handling.Current().speedMps+downhill.handling.Current().speedMps)<1e-12);
    const auto y=MakeFixture();Rig crest;ok=crest.Initialize(y,y.Long(),MakeHandlingDefinitions(),550)&&crest.Steps(1);
    const double gradeError=std::abs(crest.handling.Current().gradeN+160000*9.80665*.02);
    add("S3-T08.MassWeightedGradeAcrossSeam",ok&&gradeError<1e-6,gradeError,1e-6,"expected gravity -31381.28N from three vehicle centres on grade; error unit Newtons");
    Rig journey;ok=journey.Initialize(y,y.Long(),MakeHandlingDefinitions(),200)&&journey.Command(CommandKind::Throttle,1);
    bool changed=false,spanned=false,committed=true;double bogieError=0;
    for(int i=0;i<6000&&ok&&journey.spatial.Current().headStationMetres<550;++i){
        ok&=journey.Steps(1);const auto& s=journey.spatial.Current();std::map<EdgeId,int> occupied;
        if(s.headStationMetres>250&&!changed){ok&=journey.spatial.SetChoice({EdgeId(1),1},TransitionId(1)).ok;changed=true;}
        for(const auto& v:s.vehicles){committed&=v.trackCentre.edge!=EdgeId(2);occupied[v.trackCentre.edge]++;
            Sample expected;const double offset=v.definition==DefinitionId(1)?7:8;
            ok&=journey.spatial.History().SampleAt(y.graph,v.centreStationMetres+offset,expected).ok;
            bogieError=std::max(bogieError,(v.frontBogie-expected.position).Length());}
        spanned|=occupied.size()>=3;
    }
    add("S3-T09.PoweredForwardTraversal",ok&&journey.spatial.Current().head.edge==EdgeId(7));
    add("S3-T11.MultiEdgePlacement",ok&&spanned&&bogieError<=.05,bogieError,.05,"S3 snapshots checked against trusted S2 path sampler at each powered tick");
    add("S3-T12.CommittedJunction",ok&&changed&&committed&&journey.spatial.History().Validate(y.graph).ok);
    ok&=journey.Command(CommandKind::Throttle,0)&&journey.Command(CommandKind::Brake,1)&&journey.Steps(600);
    ok&=journey.handling.Current().speedMps==0;
    ok&=journey.Command(CommandKind::Direction,-1)&&journey.Command(CommandKind::Brake,0)&&journey.Command(CommandKind::Throttle,1);
    for(int i=0;i<6000&&ok&&journey.spatial.Current().headStationMetres>320;++i){ok&=journey.Steps(1);for(const auto& v:journey.spatial.Current().vehicles)committed&=v.trackCentre.edge!=EdgeId(2);}
    const bool reversed=journey.handling.Current().speedMps<0&&journey.spatial.Current().leadingEnd.direction==-1;
    ok&=journey.Command(CommandKind::Throttle,0)&&journey.Command(CommandKind::Brake,1)&&journey.Steps(600);
    add("S3-T10.ReverseBrakeOnCommittedHistory",ok&&reversed&&committed&&journey.handling.Current().speedMps==0&&journey.spatial.Current().vehicles.size()==8);
    double referenceHead=0,referenceSpeed=0,renderError=0;ok=true;
    for(int fps:{30,60,144}){Rig timed;ok&=timed.Initialize(flat,flat.Long());
        ok&=timed.Command(CommandKind::Throttle,1,1)&&timed.Command(CommandKind::Throttle,0,301)&&timed.Command(CommandKind::Brake,1,331)
           &&timed.Command(CommandKind::Direction,-1,601)&&timed.Command(CommandKind::Brake,0,601)&&timed.Command(CommandKind::Throttle,.5,602)
           &&timed.Command(CommandKind::Throttle,0,901)&&timed.Command(CommandKind::Brake,1,931);
        for(int i=0;i<fps*40&&ok;++i)ok&=timed.handling.Update(timed.spatial,1.0/fps).ok;
        ok&=timed.spatial.Current().tick==1200;
        if(fps==30){referenceHead=timed.spatial.Current().headStationMetres;referenceSpeed=timed.handling.Current().speedMps;}
        renderError=std::max(renderError,std::abs(timed.spatial.Current().headStationMetres-referenceHead));
        renderError=std::max(renderError,std::abs(timed.handling.Current().speedMps-referenceSpeed));
    }
    add("S3-T13.RenderRate30_60_144",ok&&renderError<=1e-9,renderError,1e-9,"same40s command trace and1200 logical ticks; max position/speed delta");
    Rig suspended=seed;const auto savedSpeed=suspended.handling.Current().speedMps;const auto savedHead=suspended.spatial.Current().headStationMetres;
    suspended.handling.SetSuspended(true);ok=suspended.handling.Update(suspended.spatial,300).ok;suspended.handling.SetSuspended(false);ok&=suspended.handling.Update(suspended.spatial,300).ok;
    const bool frozen=suspended.handling.Current().speedMps==savedSpeed&&suspended.spatial.Current().headStationMetres==savedHead;
    ok&=suspended.Steps(1);add("S3-T14.SuspendExcludesElapsedTime",ok&&frozen&&suspended.handling.Current().command.throttle==0&&suspended.handling.Current().command.brake==1);
    Rig invalid;ok=invalid.Initialize(flat,flat.Short());
    ok&=!invalid.Command(CommandKind::Throttle,-.1)&&!invalid.Command(CommandKind::Brake,1.1)&&!invalid.Command(CommandKind::Throttle,std::numeric_limits<double>::quiet_NaN())&&!invalid.Command(CommandKind::Direction,2);
    ok&=!invalid.handling.Submit(invalid.spatial,{TrainId(9),1,50,1,CommandKind::Throttle,1}).ok;
    ok&=!invalid.handling.Submit(invalid.spatial,{TrainId(1),2,50,1,CommandKind::Throttle,1}).ok;
    ok&=invalid.Command(CommandKind::Throttle,.5);ok&=!invalid.handling.Submit(invalid.spatial,{TrainId(1),1,invalid.sequence,1,CommandKind::Throttle,1}).ok;
    add("S3-T15.InvalidCommandsAndOwnership",ok&&invalid.handling.Current().speedMps==0);
    Rig reversing=seed;ok=reversing.Command(CommandKind::Direction,-1)&&reversing.Steps(1);
    add("S3-T16.ReverserRejectedWhileMoving",ok&&reversing.handling.Current().command.direction==1&&reversing.handling.Current().rejectedCommands>0&&reversing.handling.Current().speedMps>0);
    add("S3-T17.FullS2Regression39",RunKernelTests().Passed());
    Rig analytic;ok=analytic.Initialize(flat,flat.Short(),AnalyticDefinitions())&&analytic.Command(CommandKind::Throttle,1)&&analytic.Steps(300);
    double analyticError=std::max(std::abs(analytic.handling.Current().speedMps-10),std::abs(analytic.spatial.Current().headStationMetres-1250));
    add("S3.AnalyticConstantAcceleration",ok&&analyticError<=1e-9,analyticError,1e-9,"120kN /120000kg =1m/s2; after10s speed10m/s and distance50m");
    ok&=analytic.Command(CommandKind::Throttle,0)&&analytic.Command(CommandKind::Brake,1)&&analytic.Steps(301);
    analyticError=std::abs(analytic.spatial.Current().headStationMetres-1300);
    add("S3.AnalyticStoppingDistance",ok&&analytic.handling.Current().speedMps==0&&analyticError<=.01,analyticError,.01,"constant1m/s2 braking from10m/s; independent stopping distance50m");
    Rig lag=seed;ok=lag.Command(CommandKind::Brake,1)&&lag.Steps(1);
    add("S3.BrakeResponseNotInstant",ok&&lag.handling.Current().appliedBrake>0&&lag.handling.Current().appliedBrake<1&&lag.handling.Current().tractionN==0);
    Rig emergency=seed;ok=emergency.Command(CommandKind::Emergency,1)&&emergency.Command(CommandKind::Emergency,0)&&emergency.Command(CommandKind::Throttle,1)&&emergency.Steps(1);
    add("S3.EmergencyPrecedence",ok&&emergency.handling.Current().command.emergency&&emergency.handling.Current().tractionN==0&&emergency.handling.Current().rejectedCommands>0);
    Rig hold;ok=hold.Initialize(up,up.Long())&&hold.Command(CommandKind::Brake,1)&&hold.Steps(180);const double heldHead=hold.spatial.Current().headStationMetres;
    ok&=hold.Steps(300);add("S3.GradeHoldAfterBrakeResponse",ok&&hold.handling.Current().speedMps==0&&hold.spatial.Current().headStationMetres==heldHead);
    auto wetDefs=MakeHandlingDefinitions();wetDefs.at(DefinitionId(1)).adhesionCoefficient=.03;
    Rig dry,wet;ok=dry.Initialize(flat,flat.Long())&&wet.Initialize(flat,flat.Long(),wetDefs)&&dry.Command(CommandKind::Throttle,1)&&wet.Command(CommandKind::Throttle,1)&&dry.Steps(300)&&wet.Steps(300);
    add("S3.AdhesionBoundsTraction",ok&&wet.handling.Current().speedMps<dry.handling.Current().speedMps);
    Rig original=seed;ok=original.Command(CommandKind::Brake,.5)&&original.Steps(10);Rig restored=original;
    ok&=original.Steps(300)&&restored.Steps(300);const double snapshotError=std::abs(original.spatial.Current().headStationMetres-restored.spatial.Current().headStationMetres)+std::abs(original.handling.Current().appliedBrake-restored.handling.Current().appliedBrake);
    add("S3.InMemoryCheckpointContinuation",ok&&snapshotError==0,snapshotError,0,"copy full pure spatial/model state while braking; no persistence implementation");
    double stops[2]={0,0};ok=true;
    for(int index=0;index<2;++index){Rig rate;rate.spatial.FixedStepSeconds=1.0/(index==0?30:60);ok&=rate.Initialize(flat,flat.Long())&&rate.Command(CommandKind::Throttle,1)&&rate.Steps(index==0?600:1200);
        ok&=rate.Command(CommandKind::Throttle,0)&&rate.Command(CommandKind::Brake,1);const double start=rate.spatial.Current().headStationMetres;
        int count=0;while(rate.handling.Current().speedMps>0&&count++<3000&&ok)ok&=rate.Steps(1);ok&=rate.handling.Current().speedMps==0;stops[index]=rate.spatial.Current().headStationMetres-start;
    }
    const double stepError=std::abs(stops[0]-stops[1]),stepTolerance=std::max(.5,.01*stops[1]);
    add("S3.Step30vs60StoppingConvergence",ok&&stepError<=stepTolerance,stepError,stepTolerance,"roadmap T06: max0.5m or1percent stopping distance, unchanged criterion");
    auto bad=MakeHandlingDefinitions();bad.at(DefinitionId(1)).brakeApplySeconds=0;Rig malformed;
    add("S3.InvalidDefinitionRejected",!malformed.Initialize(flat,flat.Long(),bad));
    Rig overload;ok=overload.Initialize(flat,flat.Long());add("S3.S2OverloadPreserved",ok&&!overload.handling.Update(overload.spatial,1).ok&&overload.spatial.Paused()&&overload.spatial.Current().tick==0);
    return report;
}
}
