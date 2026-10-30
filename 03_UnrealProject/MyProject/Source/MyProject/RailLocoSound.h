#pragma once
// Real-time acoustic model of a WAP-7 hauled train, driven by simulation state every frame.
// Pure C++17, no Unreal includes: rendered on the audio thread by URailLocoSoundComponent and
// offline by 07_Documentation/S10/Tools/LocoSoundPreview.cpp. Presentation only: it reads the
// railway and never feeds back. Every source is tied to a simulated quantity (motor speed, tractive
// or braking effort, cylinder fill, curvature, wheels crossing real joints), so the sound cannot
// disagree with the train. Mix levels and the converter pulse pattern are provisional
// (07_Documentation/S10/AUDIO.md).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Rail {
// WAP-7 drive train. CLW spec 4TMS 096 081 and CAMTECH 6FRA 6068 pamphlet: six 6-pole asynchronous
// traction motors, gear 72:20, wheels 1092 mm new / 1054 mm half-worn; 1283 rpm at the 71 km/h
// continuous rating (checked: 71 km/h on half-worn wheels gives 1286 rpm). The GTO converter's
// switching pattern is not published; GTO inverters are limited to a few hundred hertz, so the
// model uses asynchronous PWM at low speed, synchronous pulse modes, then square-wave operation
// above base speed (provisional).
struct Wap7Drive {
    static constexpr double WheelDiameter=1.054,GearRatio=72.0/20,PolePairs=3,PinionTeeth=20,MainsHz=50;
    static constexpr double StartingTractionN=323000; // service WAP-7 (RailTambaramDriving.h): effort fraction reference
    static double MotorRps(double speedMps){return std::abs(speedMps)/(3.14159265358979*WheelDiameter)*GearRatio;}
    // Stator frequency; slip adds while motoring and subtracts while regenerating (provisional 1.6 Hz).
    static double StatorHz(double speedMps,double effort){return std::max(0.0,PolePairs*MotorRps(speedMps)+1.6*effort);}
    struct Modulation{double carrier;int pulses;}; // pulses 0 = asynchronous carrier
    static Modulation ModulationFor(double fe){
        if(fe<27)return {400,0};
        if(fe<48)return {9*fe,9};
        if(fe<64)return {5*fe,5};
        return {fe,1}; // square wave (six-step) in the constant-power region
    }
};

struct LocoSoundInput {
    double speedMps=0;      // along the path; magnitude drives rolling sound
    double effort=0;        // realised TE (+) or regenerative BE (-) as a fraction of the loco maximum
    double curvature=0;     // 1/m under the leading bogie
    double brakeCylinder=0; // 0..1 cylinder fill
    double applyFlow=0,releaseFlow=0,ventFlow=0; // A9 service exhaust, distributor release, emergency vent (0..1)
    double odometer=0;      // metres run, monotonic; wheel strikes are scheduled against it
    double cab=1;           // 1 inside the WAP-7 cab, 0 outside the train
    double engineGain=1,effectsGain=1; // player volume categories
    bool energized=true;    // VCB closed: transformer, converters and blowers running
};
enum LocoSoundLayers : uint32_t { SoundDrive=1,SoundAuxiliaries=2,SoundRunning=4,SoundAir=8,SoundAll=15 };
enum class StrikeKind : int { Joint=0,Switch=1,CrossingNose=2 };
// One wheelset crossing one rail discontinuity, when the synth's odometer reaches atOdometer.
// The key (non-zero) identifies the wheelset and joint so repeated scheduling sounds once.
struct WheelStrike { uint64_t key=1; double atOdometer=0,gain=1; StrikeKind kind=StrikeKind::Joint; };
// Axles about each bogie centre: WAP-7 Co-Co, three axles at a provisional 1.9 m pitch; LHB FIAT
// bogie, two axles 2.56 m apart (FormationContent::BogieWheelbase).
constexpr double Wap7AxlePitch=1.9;
// Loudness of a wheel strike heard from the cab: the cab rides over the leading bogie, the rear
// bogie is further back, coach wheels are behind the loco body and fall away with distance.
inline double StrikeGain(double metresBehindHead,bool locomotive){
    return locomotive?1/(1+std::max(0.0,metresBehindHead-3)/5):.35/(1+std::max(0.0,metresBehindHead-22)/30);
}
// Metres run by one train from its head station. Another train or a jump (restore, camera source
// change) moves the odometer far enough that the synth drops strikes scheduled for the old position.
struct TrainOdometer {
    uint64_t train=0;double head=0,metres=0;bool valid=false;
    double Advance(uint64_t id,double station){
        if(!valid||id!=train||!std::isfinite(station)||std::abs(station-head)>50){train=id;head=station;valid=true;metres+=100;return metres;}
        metres+=std::abs(station-head);head=station;return metres;
    }
};

class LocoSoundSynth {
public:
    explicit LocoSoundSynth(uint32_t seed=1,uint32_t layers=SoundAll):layers_(layers),seed_(seed?seed:1){
        for(int i=0;i<3;++i)noise_[i]=seed_*2654435761u+0x9E3779B9u*(i+1)|1;
        Prepare(48000);
    }
    void Prepare(double rate){
        rate_=rate>=8000?rate:48000;inv_=1/rate_;block_=0;
        rumbleA_.Set(Biquad::Low,95,.7,rate_);rumbleB_.Set(Biquad::Low,95,.7,rate_);roarBand_.Set(Biquad::Band,900,.7,rate_);
        windHigh_.Set(Biquad::High,900,.7,rate_);frictionBand_.Set(Biquad::Band,2500,2,rate_);blowerLow_.Set(Biquad::Band,450,.6,rate_);
        blowerHigh_.Set(Biquad::Band,1400,.8,rate_);oilCooler_.Set(Biquad::Low,300,.7,rate_);applyHigh_.Set(Biquad::High,1500,.7,rate_);
        applyBand_.Set(Biquad::Band,3500,.5,rate_);releaseBand_.Set(Biquad::Band,1800,.6,rate_);ventLow_.Set(Biquad::Low,6000,.7,rate_);
        ventRoar_.Set(Biquad::Band,400,.8,rate_);
    }
    // Newest simulation state. A large odometer jump (camera cut, restore) snaps and drops strikes.
    void Submit(const LocoSoundInput& in){
        if(!submitted_||std::abs(in.odometer-odometer_)>6){odometer_=in.odometer;pending_=0;}
        in_=in;anchor_=in.odometer;sinceAnchor_=0;submitted_=true;
    }
    // Ignored when the same wheelset/joint is pending or has already sounded.
    void Schedule(const WheelStrike& s){
        if(!std::isfinite(s.atOdometer)||s.atOdometer<odometer_-.5)return;
        for(int i=0;i<pending_;++i)if(strikes_[i].key==s.key)return;
        for(uint64_t k:recent_)if(k==s.key)return;
        if(pending_==int(strikes_.size()))return;
        int at=pending_++;while(at>0&&strikes_[at-1].atOdometer>s.atOdometer){strikes_[at]=strikes_[at-1];--at;}
        strikes_[at]=s;
    }
    void Render(float* out,int count){
        for(int n=0;n<count;++n){
            if(block_==0)Control();
            block_=(block_+1)%Block;
            out[n]=submitted_?Sample():0.f;
        }
    }
    // Diagnostics for tests and the offline preview.
    double StatorHz()const{return statorHz_;} double MeshHz()const{return meshHz_;} double CarrierHz()const{return carrierHz_;}
    double Odometer()const{return odometer_;} int StrikesFired()const{return fired_;} int Pending()const{return pending_;}
private:
    static constexpr int Block=32;
    struct Biquad {
        float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
        enum Type{Low,High,Band};
        void Set(Type t,double f,double q,double rate){ // RBJ cookbook
            f=std::clamp(f,10.0,rate*.45);const double w=2*3.14159265358979*f/rate,c=std::cos(w),a=std::sin(w)/(2*q),n=1/(1+a);
            double x0,x1,x2;
            if(t==Low){x0=(1-c)*.5;x1=1-c;x2=x0;}else if(t==High){x0=(1+c)*.5;x1=-(1+c);x2=x0;}else{x0=a;x1=0;x2=-a;}
            b0=float(x0*n);b1=float(x1*n);b2=float(x2*n);a1=float(-2*c*n);a2=float((1-a)*n);
        }
        float operator()(float x){const float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
    };
    struct Tone { // phase-continuous sinusoid; frequency and amplitude ramp across a control block
        double phase=0,freq=0,dfreq=0;float amp=0,damp=0;
        void Target(double f,float a){dfreq=(f-freq)/Block;damp=(a-amp)/Block;}
        float operator()(double inv){
            const float v=amp*Sine(phase);phase+=freq*inv;phase-=std::floor(phase);freq+=dfreq;amp+=damp;return v;
        }
    };
    struct Smooth{double v=0;void To(double goal,double seconds,double dt){v+=(goal-v)*(1-std::exp(-dt/std::max(seconds,1e-4)));}};
    struct Voice { // one wheel strike: body thump, rail ring, wheel ring and contact click
        bool on=false;double pT=0,pR=0,pW=0,fT=0,fR=0,fW=0;float eT=0,eR=0,eW=0,eC=0,dT=0,dR=0,dW=0,dC=0;
    };
    static float Sine(double phase){
        static const auto table=[]{std::array<float,4097> t{};for(int i=0;i<=4096;++i)t[i]=float(std::sin(2*3.14159265358979*i/4096));return t;}();
        const double x=phase*4096;const int i=int(x);const float f=float(x-i);return table[i]+(table[i+1]-table[i])*f;
    }
    float White(int s){uint32_t& x=noise_[s];x^=x<<13;x^=x>>17;x^=x<<5;return float(int32_t(x))*(1.f/2147483648.f);}
    double Random01(){return (White(2)+1)*.5;}

    void Control(){
        const double dt=Block*inv_,v=std::abs(in_.speedMps);
        speed_.To(v,.08,dt);effort_.To(in_.energized?std::clamp(in_.effort,-1.0,1.0):0,.15,dt);cab_.To(std::clamp(in_.cab,0.0,1.0),.2,dt);
        apply_.To(in_.applyFlow,.04,dt);release_.To(in_.releaseFlow,.08,dt);vent_.To(in_.ventFlow,.03,dt);
        brake_.To(in_.brakeCylinder,.1,dt);curve_.To(std::abs(in_.curvature),.3,dt);
        energized_.To(in_.energized?1:0,.6,dt);engine_.To(in_.engineGain,.05,dt);effects_.To(in_.effectsGain,.05,dt);
        for(auto& w:wobble_)w.To(Random01()*2-1,.7,dt);
        const double s=speed_.v,c=cab_.v,e=1-c,load=std::pow(std::abs(effort_.v),.6),moving=std::clamp(s/1.5,0.0,1.0);
        const double eng=engine_.v,fx=effects_.v;
        const bool drive=layers_&SoundDrive,aux=layers_&SoundAuxiliaries,run=layers_&SoundRunning,air=layers_&SoundAir;

        // Traction converter and motor magnetic tones. Pulses are blocked at zero effort.
        statorHz_=Wap7Drive::StatorHz(s,effort_.v);const auto m=Wap7Drive::ModulationFor(statorHz_);carrierHz_=m.carrier;
        const double fe=statorHz_,fc=m.carrier,pwm=drive&&fe>.3?load*eng*(.07*c+.14*e)*energized_.v:0;
        double f[5],a[5];
        if(m.pulses==1){const double h[5]={5,7,11,13,17},w[5]={.55,.42,.22,.17,.08};for(int i=0;i<5;++i){f[i]=h[i]*fe;a[i]=w[i];}}
        else{f[0]=fc;f[1]=fc-2*fe;f[2]=fc+2*fe;f[3]=2*fc-fe;f[4]=2*fc+fe;const double w[5]={.30,.55,.55,.22,.22};for(int i=0;i<5;++i)a[i]=w[i];}
        for(int i=0;i<5;++i)pwm_[i].Target(f[i],float(f[i]>40?a[i]*pwm:0));
        ripple_[0].Target(6*fe,float(drive?.22*load*eng*(.06*c+.06*e)*std::clamp(fe/3,0.0,1.0):0));
        ripple_[1].Target(12*fe,float(drive?.10*load*eng*(.06*c+.06*e)*std::clamp(fe/3,0.0,1.0):0));
        // Gear mesh (20-tooth pinion): always present when turning, louder under torque.
        meshHz_=Wap7Drive::MotorRps(s)*Wap7Drive::PinionTeeth;
        const double mesh=drive?eng*(.10*c+.10*e)*(.25+.75*load)*std::clamp(s/2,0.0,1.0)*(1+.15*wobble_[0].v):0;
        const double mw[3]={.30,.12,.05};for(int i=0;i<3;++i)mesh_[i].Target(meshHz_*(i+1),float(mesh*mw[i]));
        // Main transformer magnetostriction (2x mains) and its harmonics, louder under load.
        const double hum=aux?eng*energized_.v*(.10*c+.05*e)*(.7+.3*std::abs(effort_.v)):0;
        const double hw[4]={.25,.18,.10,.05};for(int i=0;i<4;++i)hum_[i].Target(2*Wap7Drive::MainsHz*(i+1),float(hum*hw[i]));
        // Traction-motor and oil-cooler blowers (blade-pass tones provisional).
        blower_=aux?eng*energized_.v*(.14*c+.22*e)*(1+.08*wobble_[1].v):0;
        blade_[0].Target(539*(1+.004*wobble_[1].v),float(blower_*.10));blade_[1].Target(220*(1+.004*wobble_[2].v),float(blower_*.08));
        // Rolling: structure-borne rumble, wheel/rail roughness roar (~30 log v), aerodynamic (~60 log v).
        const double rough=1+.25*wobble_[3].v;
        rumble_=run?fx*moving*(.9*c+.25*e)*std::pow(std::min(s,45.0)/30,1.2)*rough:0;
        roar_=run?fx*moving*(.22*c+.55*e)*std::pow(std::min(s,45.0)/30,1.5)*rough:0;
        wind_=run?fx*(.12*c+.35*e)*std::pow(std::min(s,45.0)/42,3):0;
        if(run){roarBand_.Set(Biquad::Band,350+28*s,.7,rate_);}
        // Flange contact on sharp curves at low and moderate speed.
        const double sq=run?fx*(.10*c+.25*e)*std::clamp((curve_.v-1/500.)/(1/200.),0.0,1.0)*std::clamp(s/5,0.0,1.0)*std::clamp((25-s)/10,0.0,1.0):0;
        squeal_[0].Target(2250+60*wobble_[4].v,float(sq*.12*std::max(0.0,.4+wobble_[5].v)));
        squeal_[1].Target(3700+90*wobble_[5].v,float(sq*.07*std::max(0.0,.4+wobble_[4].v)));
        // Friction brakes: block/disc grind while braking, squeal in the last metres before a stop.
        friction_=run?fx*brake_.v*std::clamp(s/3,0.0,1.0)*std::clamp((30-s)/20,0.0,1.0)*(.05*c+.10*e):0;
        const double stop=run?fx*brake_.v*std::clamp((5-s)/4,0.0,1.0)*std::clamp(s/.4,0.0,1.0)*(.04*c+.08*e):0;
        squeal_[2].Target(4300+80*wobble_[2].v,float(stop*std::max(0.0,.5+wobble_[3].v)));
        // Air: A9 service exhaust under the desk, distributor release exhaust, emergency vent.
        applyAir_=air?fx*apply_.v*(.35*c+.05*e):0;releaseAir_=air?fx*release_.v*(.12*c+.30*e):0;ventAir_=air?fx*vent_.v*(.50*c+.40*e):0;
        cabWeight_=c;
    }
    float Sample(){
        const double v=speed_.v;
        // Odometer: integrate speed, gently pulled to the simulation's (lag stays within a buffer).
        sinceAnchor_+=inv_;
        const double expected=anchor_+v*sinceAnchor_;
        odometer_+=v*inv_+(expected-odometer_)*std::min(1.0,inv_/.3);
        while(pending_>0&&strikes_[0].atOdometer<=odometer_){Strike(strikes_[0]);recent_[recentAt_++%recent_.size()]=strikes_[0].key;for(int i=1;i<pending_;++i)strikes_[i-1]=strikes_[i];--pending_;}

        float y=0;
        for(auto& t:pwm_)if(t.amp>1e-6f||t.damp!=0)y+=t(inv_);
        for(auto& t:ripple_)if(t.amp>1e-6f||t.damp!=0)y+=t(inv_);
        for(auto& t:mesh_)if(t.amp>1e-6f||t.damp!=0)y+=t(inv_);
        for(auto& t:hum_)if(t.amp>1e-6f||t.damp!=0)y+=t(inv_);
        for(auto& t:blade_)if(t.amp>1e-6f||t.damp!=0)y+=t(inv_);
        for(auto& t:squeal_)if(t.amp>1e-6f||t.damp!=0)y+=t(inv_);
        if(rumble_>1e-5||roar_>1e-5||wind_>1e-5||friction_>1e-5){
            const float w=White(0);
            y+=float(rumble_)*2.2f*rumbleB_(rumbleA_(w));
            y+=float(roar_)*roarBand_(w);
            y+=float(wind_)*windHigh_(White(1));
            y+=float(friction_)*frictionBand_(w);
        }
        if(blower_>1e-5){const float w=White(1);y+=float(blower_)*(.9f*blowerLow_(w)+.35f*blowerHigh_(w)+.5f*oilCooler_(w));}
        if(applyAir_>1e-5||releaseAir_>1e-5||ventAir_>1e-5){
            const float w=White(2);
            y+=float(applyAir_)*applyBand_(applyHigh_(w));
            y+=float(releaseAir_)*releaseBand_(w);
            y+=float(ventAir_)*(ventLow_(w)+.6f*ventRoar_(w));
        }
        for(auto& s:voices_)if(s.on){
            y+=s.eT*Sine(s.pT)+s.eR*Sine(s.pR)+s.eW*Sine(s.pW)+s.eC*White(1);
            s.pT+=s.fT*inv_;s.pT-=std::floor(s.pT);s.pR+=s.fR*inv_;s.pR-=std::floor(s.pR);s.pW+=s.fW*inv_;s.pW-=std::floor(s.pW);
            s.eT*=s.dT;s.eR*=s.dR;s.eW*=s.dW;s.eC*=s.dC;
            if(std::abs(s.eT)+std::abs(s.eR)+std::abs(s.eW)+std::abs(s.eC)<1e-5f)s.on=false;
        }
        return std::clamp(y,-1.f,1.f);
    }
    void Strike(const WheelStrike& s){
        ++fired_;
        if(!(layers_&SoundRunning))return;
        const double v=speed_.v;if(v<.3)return;
        const double c=cabWeight_,e=1-c,level=s.gain*std::pow(std::clamp(v/22,0.0,1.4),.9)*effects_.v;
        const double heavy=s.kind==StrikeKind::CrossingNose?1.35:s.kind==StrikeKind::Switch?.7:1.0;
        auto& d=voices_[voiceAt_++%voices_.size()];
        auto decay=[&](double seconds){return float(std::exp(-inv_/seconds));};
        d.on=true;d.pT=d.pR=d.pW=0;
        d.fT=(58+24*Random01())*(s.kind==StrikeKind::CrossingNose?.9:1);d.fR=420+260*Random01();d.fW=1500+900*Random01();
        d.eT=float(level*heavy*(.55*c+.25*e));d.eR=float(level*heavy*(.18*c+.32*e));d.eW=float(level*(.05*c+.18*e));d.eC=float(level*heavy*(.10*c+.25*e));
        d.dT=decay(.07);d.dR=decay(.028);d.dW=decay(.012);d.dC=decay(.003);
    }

    uint32_t layers_,seed_;std::array<uint32_t,3> noise_{};
    double rate_=48000,inv_=1/48000.,odometer_=0,anchor_=0,sinceAnchor_=0;int block_=0;bool submitted_=false;
    LocoSoundInput in_;
    Smooth speed_,effort_,cab_,apply_,release_,vent_,brake_,curve_,energized_,engine_,effects_;std::array<Smooth,6> wobble_;
    std::array<Tone,5> pwm_;std::array<Tone,2> ripple_;std::array<Tone,3> mesh_,squeal_;std::array<Tone,4> hum_;std::array<Tone,2> blade_;
    double statorHz_=0,meshHz_=0,carrierHz_=0,blower_=0,rumble_=0,roar_=0,wind_=0,friction_=0,applyAir_=0,releaseAir_=0,ventAir_=0,cabWeight_=1;
    Biquad rumbleA_,rumbleB_,roarBand_,windHigh_,frictionBand_,blowerLow_,blowerHigh_,oilCooler_,applyHigh_,applyBand_,releaseBand_,ventLow_,ventRoar_;
    std::array<WheelStrike,96> strikes_{};int pending_=0;std::array<uint64_t,512> recent_{};size_t recentAt_=0;
    std::array<Voice,16> voices_{};size_t voiceAt_=0;int fired_=0;
};
}
