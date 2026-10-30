#include "RailTambaramDriving.h"
#include "RailTambaramRoute.h"
#include <cmath>
#include <string>

namespace Rail {
namespace {
// WAP-7 + LHB rake on the straight Tambaram corridor, driven directly through S3 commands.
struct Rig {
    Simulation spatial;HandlingModel handling;uint64_t sequence=0;
    bool Initialize(){
        Fixture F;Formation Player,Traffic;auto R=TambaramRoute::ResolveSpatial(F,Player,Traffic);Choices Straight;
        for(size_t I=0;I+1<TambaramRoute::SegmentCount;++I)Straight.emplace(DirectedEdge{TambaramRoute::EdgeFor(1,I),1},TambaramRoute::TransitionFor(1,I,false));
        if(R.ok)R=spatial.Initialize(F.graph,F.definitions,Player,{TambaramRoute::EdgeFor(1,0),1},TambaramRoute::StartHead,Straight);
        if(R.ok)R=handling.Initialize(spatial,F.definitions,Player,TambaramDriving::Definitions());return R.ok;
    }
    bool Command(CommandKind K,double V){return handling.Submit(spatial,{spatial.Current().train,1,++sequence,spatial.Current().tick+1,K,V}).ok;}
    bool Steps(unsigned Count){for(unsigned I=0;I<Count;++I)if(!handling.Update(spatial,1.0/30).ok)return false;return true;}
    double Cylinder() const {return handling.Current().appliedBrake*AirBrake::MaxCylinder;}
    // Seconds (at the 30 Hz clock) until a condition holds, or -1.
    template<class F> double Until(F&& done,unsigned limit=3600){for(unsigned I=0;I<limit;++I){if(done())return I/30.0;if(!Steps(1))return -1;}return -1;}
};
}

TestReport RunAirBrakeTests(){
    TestReport report;auto expect=[&](const std::string& name,bool ok,const std::string& detail=""){report.cases.push_back({name,ok,0,0,detail});};
    expect("S10-AB.PipeChargedInReleaseAndRunning",AirBrake::BrakePipe(0,false)==AirBrake::ChargedPipe&&AirBrake::BrakePipe(.05,false)==AirBrake::ChargedPipe);
    expect("S10-AB.MinimumReduction",std::abs(AirBrake::BrakePipe(AirBrake::RunningZone,false)-4.6)<1e-9);
    expect("S10-AB.FullServiceReduction",std::abs(AirBrake::BrakePipe(1,false)-3.5)<1e-9);
    expect("S10-AB.EmergencyVentsPipe",AirBrake::BrakePipe(.3,true)==0);
    expect("S10-AB.LhbCylinderLimit",AirBrake::CylinderFraction(3.5)==1&&AirBrake::CylinderFraction(0)==1&&AirBrake::CylinderFraction(5.0)==0);

    {   Rig r;bool ok=r.Initialize()&&r.Command(CommandKind::Brake,0)&&r.Steps(900)&&r.Cylinder()<1e-9;
        expect("S10-AB.RigReleased",ok);
        // Application: a full service fills the LHB cylinder to 95 % within the 3-5 s handbook band.
        ok=ok&&r.Command(CommandKind::Brake,1);
        const double apply=ok?r.Until([&]{return r.Cylinder()>=.95*AirBrake::MaxCylinder;}):-1;
        expect("S10-AB.ApplicationTime3to5s",apply>=3&&apply<=5,std::to_string(apply));
        ok=ok&&r.Steps(120);
        // Release: from full to 0.4 kg/cm2 within the 15-20 s handbook band.
        ok=ok&&r.Command(CommandKind::Brake,0);
        const double release=ok?r.Until([&]{return r.Cylinder()<=.4;},1800):-1;
        expect("S10-AB.ReleaseTime15to20s",release>=15&&release<=20,std::to_string(release));
        // Graduated application then graduated (partial) release, as the distributor allows.
        ok=ok&&r.Steps(900)&&r.Command(CommandKind::Brake,.5)&&r.Steps(300);
        const double half=r.Cylinder(),expected=AirBrake::CylinderFraction(AirBrake::BrakePipe(.5,false))*AirBrake::MaxCylinder;
        expect("S10-AB.GraduatedApplication",ok&&std::abs(half-expected)<1e-6&&half>.5&&half<AirBrake::MaxCylinder,std::to_string(half));
        ok=ok&&r.Command(CommandKind::Brake,.2)&&r.Steps(900);
        const double lower=r.Cylinder(),lowerExpected=AirBrake::CylinderFraction(AirBrake::BrakePipe(.2,false))*AirBrake::MaxCylinder;
        expect("S10-AB.GraduatedRelease",ok&&std::abs(lower-lowerExpected)<1e-6&&lower>0&&lower<half,std::to_string(lower));
        // Traction interlock while the pipe is reduced.
        ok=ok&&r.Command(CommandKind::Throttle,1)&&r.Steps(300);
        expect("S10-AB.TractionInterlockWithReducedPipe",ok&&r.handling.Current().appliedThrottle==0);
    }
    {   Rig r;bool ok=r.Initialize()&&r.Command(CommandKind::Brake,0)&&r.Steps(900)&&r.Command(CommandKind::Throttle,1);
        const double reach=ok?r.Until([&]{return r.handling.Current().speedMps>=60/3.6;},18000):-1;
        ok=ok&&reach>0&&r.Command(CommandKind::Throttle,-1)&&r.Steps(150);
        const auto& s=r.handling.Current();const double v=s.speedMps;
        const auto& wap7=TambaramDriving::Definitions().at(DefinitionId(FormationContent::Wap7DefinitionId));
        const double expected=RegenerativeEffortN(wap7,v,wap7.adhesionCoefficient*wap7.adhesiveMassFraction*1e9)*-s.appliedThrottle;
        expect("S10-AB.RegenerativeEffortAtSpeed",ok&&s.appliedThrottle<-.99&&s.tractionN<0&&std::abs(-s.tractionN-expected)<expected*.05&&s.brakingN==0,
               std::to_string(s.tractionN)+" vs "+std::to_string(expected));
        expect("S10-AB.RegenerativeLimitIs182kN",std::abs(RegenerativeEffortN(wap7,10,1e9)-182000)<1&&RegenerativeEffortN(wap7,30,1e9)<182000);
        const double slow=ok?r.Until([&]{return r.handling.Current().speedMps<.5;},36000):-1;
        ok=ok&&r.Steps(30); // one more second, wholly below the regeneration fade
        expect("S10-AB.RegenerationFadesAtLowSpeed",slow>0&&r.handling.Current().tractionN==0,std::to_string(r.handling.Current().tractionN));
        expect("S10-AB.RegenerationDoesNotHoldStandstill",RegenerativeEffortN(wap7,0,1e9)==0);
    }
    {   Rig r;bool ok=r.Initialize()&&r.Command(CommandKind::Brake,0)&&r.Steps(900)&&r.Command(CommandKind::Emergency,1);
        const double fill=ok?r.Until([&]{return r.Cylinder()>=.95*AirBrake::MaxCylinder;}):-1;
        expect("S10-AB.EmergencyFillsCylinderFast",fill>0&&fill<=3.0,std::to_string(fill));
    }
    {   // Earlier fixtures keep the lumped brake and cannot regenerate.
        Simulation spatial;HandlingModel handling;auto F=MakeFixture();
        auto R=spatial.Initialize(F.graph,F.definitions,F.Short(),{EdgeId(1),1},200,F.SelectUpper());
        if(R.ok)R=handling.Initialize(spatial,F.definitions,F.Short(),MakeHandlingDefinitions());
        const bool rejected=R.ok&&!handling.Submit(spatial,{spatial.Current().train,1,1,spatial.Current().tick+1,CommandKind::Throttle,-1}).ok;
        expect("S10-AB.LegacyFormationRejectsRegeneration",rejected,R.error);
    }
    return report;
}
}
