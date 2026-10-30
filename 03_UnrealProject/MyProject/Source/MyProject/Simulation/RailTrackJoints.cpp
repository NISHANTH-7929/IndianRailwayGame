#include "RailTrackJoints.h"
#include "RailTambaramService.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <string>

namespace Rail {
namespace {
int Transitions(const Graph& g,const DirectedEdge& e,bool leaving){int n=0;for(const auto& t:g.transitions)n+=(leaving?t.from:t.to)==e;return n;}
NodeId EntryNode(const Graph& g,const DirectedEdge& e){const auto* edge=g.Find(e.edge);return edge?(e.direction==1?edge->start:edge->end):NodeId();}
}

std::vector<TrackJoint> JointsBetween(const OperationsWorld& world,TrainId train,double from,double to){
    std::vector<TrackJoint> out;const auto* s=world.Spatial(train);if(!s||!(to>=from))return out;
    // Insulated joints: one at every stop signal (track-circuit boundary).
    for(const auto& site:SignalPlanFor(world).sites){double at=0;
        if(PathStation(s->History(),site.position,at)&&at>=from&&at<=to)out.push_back({(uint64_t(1)<<40)|site.index,JointKind::InsulatedJoint,at});}
    // Turnouts: a node where the path leaves an edge with several exits (facing points) or enters
    // an edge with several entries (trailing points). The crossing nose lies on the diverging side.
    const auto& g=s->Network();const auto& o=s->History().occurrences;
    for(size_t i=1;i<o.size();++i){
        const double b=o[i].beginMetres;if(b<from-TurnoutLeadMetres||b>to+TurnoutLeadMetres)continue;
        const bool facing=Transitions(g,o[i-1].directed,true)>1,trailing=Transitions(g,o[i].directed,false)>1;if(!facing&&!trailing)continue;
        const uint64_t node=(uint64_t(2)<<40)|(EntryNode(g,o[i].directed).value<<2);
        for(const TrackJoint& j:{TrackJoint{node,JointKind::Switch,b},TrackJoint{node|1,JointKind::CrossingNose,facing?b+TurnoutLeadMetres:b-TurnoutLeadMetres}})
            if(j.station>=from&&j.station<=to)out.push_back(j);
    }
    std::sort(out.begin(),out.end(),[](const TrackJoint& a,const TrackJoint& b){return a.station<b.station;});
    return out;
}

TestReport RunTrackJointTests(){
    TestReport report;auto expect=[&](const std::string& name,bool ok,const std::string& detail=""){report.cases.push_back({name,ok,0,0,detail});};
    const auto definition=MakeLiveTambaramService();OperationsWorld world;ServiceRuntime service(definition);
    auto r=service.Select();if(r.ok)r=service.ConfirmFormation();if(r.ok)r=service.Start(world);
    expect("S10-JNT.LiveServiceStarts",r.ok,r.error);if(!r.ok)return report;
    const auto& plan=SignalPlanFor(world);
    // Paths are committed as routes are granted: drive the assisted journey until the player has
    // been routed over the Egmore crossover (two turnouts) before checking every train.
    r=service.Input(world,CommandKind::Brake,0);if(r.ok)r=service.Input(world,CommandKind::Throttle,1);
    auto routedOverCrossover=[&]{const auto j=JointsBetween(world,TrainId(2),-1e9,1e9);
        return std::count_if(j.begin(),j.end(),[](const TrackJoint& x){return x.kind==JointKind::Switch;})>=2;};
    for(unsigned i=0;i<30000&&r.ok&&service.Current().phase==ServicePhase::Running&&(i%30||!routedOverCrossover());++i)r=service.Update(world,1.0/30);
    expect("S10-JNT.JourneyRuns",r.ok,r.error);
    bool everySignal=true,pairs=true,ordered=true,unique=true,stable=true,turnout=routedOverCrossover();std::string fault;
    for(uint64_t id=2;id<=ScenarioContent::TrainCount(world.ContentId());++id){
        const TrainId train(id);const auto* s=world.Spatial(train);if(!s)continue;
        const auto joints=JointsBetween(world,train,-1e9,1e9);
        // Every signal on the path has its insulated joint at the signal.
        for(const auto& site:plan.sites){double at=0;if(!PathStation(s->History(),site.position,at))continue;
            const bool found=std::any_of(joints.begin(),joints.end(),[&](const TrackJoint& j){return j.kind==JointKind::InsulatedJoint&&std::abs(j.station-at)<1e-9;});
            if(!found){everySignal=false;fault="signal "+std::to_string(site.index)+" train "+std::to_string(id);}}
        // Each switch has its crossing nose one turnout lead away; keys are unique and ordered.
        std::set<uint64_t> keys;
        for(size_t i=0;i<joints.size();++i){
            unique&=keys.insert(joints[i].key).second&&joints[i].key!=0;if(i)ordered&=joints[i-1].station<=joints[i].station;
            if(joints[i].kind!=JointKind::Switch)continue;
            pairs&=std::any_of(joints.begin(),joints.end(),[&](const TrackJoint& j){return j.key==(joints[i].key|1)&&std::abs(std::abs(j.station-joints[i].station)-TurnoutLeadMetres)<1e-9;});
        }
        const auto again=JointsBetween(world,train,-1e9,1e9);
        stable&=again.size()==joints.size()&&std::equal(again.begin(),again.end(),joints.begin(),[](const TrackJoint& a,const TrackJoint& b){return a.key==b.key&&a.station==b.station;});
    }
    expect("S10-JNT.InsulatedJointAtEverySignal",everySignal,fault);
    expect("S10-JNT.PlayerRoutedOverCrossoverTurnouts",turnout);
    expect("S10-JNT.CrossingNoseOneLeadFromSwitch",pairs);
    expect("S10-JNT.OrderedUniqueKeys",ordered&&unique);
    expect("S10-JNT.Deterministic",stable);
    // A window returns exactly the joints in range.
    const auto* p=world.Spatial(TrainId(2));const double head=p?p->Current().headStationMetres:0;
    const auto all=JointsBetween(world,TrainId(2),-1e9,1e9),near=JointsBetween(world,TrainId(2),head,head+1500);
    const auto inRange=std::count_if(all.begin(),all.end(),[&](const TrackJoint& j){return j.station>=head&&j.station<=head+1500;});
    expect("S10-JNT.WindowSelectsRange",p&&!near.empty()&&std::ptrdiff_t(near.size())==inRange,std::to_string(near.size()));
    expect("S10-JNT.EmptyForUnknownTrain",JointsBetween(world,TrainId(99),0,1e9).empty());
    return report;
}
}
