// Offline render of the WAP-7 acoustic model (RailLocoSound.h) over a scripted Tambaram-style run,
// for listening reviews and spectral checks without the editor. Longitudinal motion uses the
// service WAP-7 figures (323 kN, 4.5 MW, 182 kN BE) and a 7-coach LHB rake; joints are spaced as on
// the automatic-signalled corridor (one insulated joint per signal) plus one 1 in 12 turnout.
// Build: cl /std:c++17 /O2 /EHsc /I <Source/MyProject> LocoSoundPreview.cpp
// Run:   LocoSoundPreview.exe <out-prefix> [seconds]  -> <prefix>-cab.wav, <prefix>-outside.wav
#define _CRT_SECURE_NO_WARNINGS
#include "RailLocoSound.h"
#include <cstdio>
#include <string>
#include <vector>

namespace {
void WriteWav(const std::string& path,const std::vector<float>& x,int rate){
    FILE* f=std::fopen(path.c_str(),"wb");if(!f)return;
    auto u32=[&](uint32_t v){std::fwrite(&v,4,1,f);};auto u16=[&](uint16_t v){std::fwrite(&v,2,1,f);};
    std::fwrite("RIFF",1,4,f);u32(36+uint32_t(x.size()*2));std::fwrite("WAVEfmt ",1,8,f);u32(16);u16(1);u16(1);u32(rate);u32(rate*2);u16(2);u16(16);
    std::fwrite("data",1,4,f);u32(uint32_t(x.size()*2));
    for(float v:x){const int16_t s=int16_t(std::lround(std::clamp(v,-1.f,1.f)*32767));std::fwrite(&s,2,1,f);}
    std::fclose(f);
}
struct Axle{double fromHead;bool loco;};
std::vector<Axle> Axles(){
    std::vector<Axle> a;const double loco=21.33,coach=24,gap=1;
    for(double b:{loco*.5-5.85,loco*.5+5.85})for(int k=-1;k<=1;++k)a.push_back({b+Rail::Wap7AxlePitch*k,true});
    for(int c=0;c<7;++c){const double centre=loco+gap+coach*.5+c*(coach+gap);for(double b:{centre-7.45,centre+7.45})for(int k:{-1,1})a.push_back({b+1.28*k,false});}
    return a;
}
}

int main(int argc,char** argv){
    const std::string prefix=argc>1?argv[1]:"loco-preview";const double total=argc>2?std::atof(argv[2]):135;
    const int rate=48000,tickSamples=rate/30;const double dt=1.0/30;
    const auto axles=Axles();
    std::vector<double> joints;for(double j=180;j<6000;j+=620)joints.push_back(j);
    const double turnout=1500;
    for(int view=0;view<2;++view){
        Rail::LocoSoundSynth synth(7);synth.Prepare(rate);std::vector<float> out;
        double v=0,x=0,odo=0,bc=1,effort=0,brakeTarget=1;bool released=false;
        for(double t=0;t<total;t+=dt){
            // Driver script.
            double demand=0;
            if(t>=6&&!released){brakeTarget=0;released=true;}
            if(t>=9&&t<70)demand=v<25?.8:.25;
            if(t>=85&&t<100)demand=-.6;
            if(t>=100)brakeTarget=v>.05?.55:0;
            effort+=(demand-effort)*(1-std::exp(-dt/1.2));
            const double previous=bc;bc+=std::clamp(brakeTarget-bc,-dt/18,dt/3.6);
            const double m=459000,te=effort>0?effort*std::min(323000.,4.5e6/std::max(v,.1)):effort<0?effort*std::min(182000.,4.5e6/std::max(v,.1))*std::clamp((v-.55)/2.25,0.,1.):0;
            const double resist=v>0?.015*m+25*v*v+17.5*v*v:0,braking=bc*m*.9*(v>0?1:0);
            v=std::max(0.,v+(te-resist-braking)/m*dt);if(v<.02&&te<=0)v=0;
            x+=v*dt;odo+=v*dt;
            Rail::LocoSoundInput in;in.speedMps=v;in.effort=effort;in.brakeCylinder=bc;in.odometer=odo;in.cab=view==0?1:0;
            const double rateBc=(bc-previous)/dt;in.applyFlow=std::clamp(rateBc/(1/3.6),0.,1.);in.releaseFlow=std::clamp(-rateBc/(1/18.),0.,1.)*(.3+.7*bc);
            synth.Submit(in);
            const double window=v*.6+3;
            auto schedule=[&](double at,uint64_t id,Rail::StrikeKind kind){
                for(size_t k=0;k<axles.size();++k){const double ahead=at-(x-axles[k].fromHead);
                    if(ahead>0&&ahead<window){const double d=axles[k].fromHead;
                        const double gain=Rail::StrikeGain(d,axles[k].loco);
                        synth.Schedule({id*64+k+1,odo+ahead,gain,kind});}}
            };
            for(size_t j=0;j<joints.size();++j)schedule(joints[j],j+1,Rail::StrikeKind::Joint);
            schedule(turnout,500,Rail::StrikeKind::Switch);schedule(turnout+28,501,Rail::StrikeKind::CrossingNose);
            const size_t at=out.size();out.resize(at+tickSamples);synth.Render(out.data()+at,tickSamples);
            if(view==0&&std::fmod(t+1e-9,5)<dt)std::printf("t=%5.1f v=%5.1f km/h effort=%+.2f bc=%.2f stator=%.1f Hz carrier=%.0f mesh=%.0f strikes=%d\n",t,v*3.6,effort,bc,synth.StatorHz(),synth.CarrierHz(),synth.MeshHz(),synth.StrikesFired());
        }
        WriteWav(prefix+(view==0?"-cab.wav":"-outside.wav"),out,rate);
    }
    return 0;
}
