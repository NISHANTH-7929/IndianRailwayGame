#include "RailKernel.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
namespace Rail {
bool TestReport::Passed() const { for(const auto& c:cases)if(!c.passed)return false;return !cases.empty(); }
std::string TestReport::Json() const {
 auto quote=[](const std::string& value){std::ostringstream out;out<<'"';for(unsigned char ch:value){if(ch=='"'||ch=='\\')out<<'\\'<<char(ch);else if(ch<32)out<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<unsigned(ch)<<std::dec;else out<<char(ch);}out<<'"';return out.str();};
 std::ostringstream s;s<<std::setprecision(17)<<"{\"passed\":"<<(Passed()?"true":"false")<<",\"cases\":[";
 for(size_t i=0;i<cases.size();++i){const auto& c=cases[i];if(i)s<<",";s<<"{\"name\":"<<quote(c.name)<<",\"passed\":"<<(c.passed?"true":"false")<<",\"errorMetres\":"<<c.measuredError<<",\"toleranceMetres\":"<<c.tolerance<<",\"detail\":"<<quote(c.detail)<<"}";}s<<"]}";return s.str();
}
TestReport RunKernelTests() {
 TestReport report;auto recordCheck=[&](const char* n,bool p,double e=0,double t=0,const char* d=""){report.cases.push_back({n,p,e,t,d});};
 const auto f=MakeFixture();recordCheck("Graph.Valid",f.graph.Validate().ok);
 auto bad=f.graph;bad.edges[0].lengthMetres=0;recordCheck("Graph.ZeroLength",!bad.Validate().ok);
 bad=f.graph;bad.edges[0].lengthMetres=-1;recordCheck("Graph.NegativeLength",!bad.Validate().ok);
 bad=f.graph;bad.edges[0].id=EdgeId(0);recordCheck("Graph.ZeroID",!bad.Validate().ok);
 bad=f.graph;bad.edges[1].id=bad.edges[0].id;recordCheck("Graph.DuplicateID",!bad.Validate().ok);
 bad=f.graph;bad.edges[0].end=NodeId(999);recordCheck("Graph.AbsentNode",!bad.Validate().ok);
 bad=f.graph;bad.edges[0].geometry.lengthMetres=241;recordCheck("Graph.LengthMismatch",!bad.Validate().ok);
 bad=f.graph;bad.transitions[0].to={EdgeId(7),1};recordCheck("Graph.IllegalTransition",!bad.Validate().ok);
 bad=f.graph;bad.transitions[0].from.direction=0;recordCheck("Graph.InvalidDirection",!bad.Validate().ok);
 bad=f.graph;bad.nodes[0].position.x=std::numeric_limits<double>::quiet_NaN();recordCheck("Graph.Nonfinite",!bad.Validate().ok);
 Sample sample;recordCheck("Position.AbsentID",!f.graph.SamplePosition({EdgeId(99),0,1},sample).ok);
 recordCheck("Position.OutOfRange",!f.graph.SamplePosition({EdgeId(1),241,1},sample).ok);
 FormationProperties p;auto form=f.Long();recordCheck("Formation.LongDerived",form.Derive(f.definitions,p).ok&&p.lengthMetres==181&&p.massKg==360000&&p.vehicleCount==8&&p.poweredCount==1);
 recordCheck("Formation.ShortDerived",f.Short().Derive(f.definitions,p).ok&&p.lengthMetres==43&&p.massKg==120000&&p.vehicleCount==2);
 form.members[1].id=form.members[0].id;recordCheck("Formation.DuplicateInstance",!form.Derive(f.definitions,p).ok);
 form=f.Long();form.members[0].definition=DefinitionId(999);recordCheck("Formation.AbsentDefinition",!form.Derive(f.definitions,p).ok);
 auto def=f.definitions.begin()->second;def.lengthMetres=0;recordCheck("Definition.ZeroLength",!def.Validate().ok);
 def=f.definitions.begin()->second;def.massKg=-1;recordCheck("Definition.NegativeMass",!def.Validate().ok);
 def=f.definitions.begin()->second;def.frontBogieOffsetMetres=11;recordCheck("Definition.BogieOutsideBody",!def.Validate().ok);
 def=f.definitions.begin()->second;def.rearBogieOffsetMetres=7;recordCheck("Definition.CoincidentBogies",!def.Validate().ok);
 double error=0;for(int i=0;i<=1000;++i){double s=240.0*i/1000;auto a=f.graph.edges[0].geometry.At(s);error=std::max(error,(a.position-Vec3{s-240,0,0}).Length());}
 recordCheck("T02.AnalyticStraight",error<=.01,error,.01,"x=s-240; y=z=0; 1001 samples");
 error=0;for(int i=0;i<=1000;++i){double a=(Pi/2)*i/1000;auto q=f.graph.edges[2].geometry.At(40*a);error=std::max(error,(q.position-Vec3{40*std::sin(a),40*(1-std::cos(a)),0}).Length());}
 recordCheck("T04.AnalyticCurve",error<=.01,error,.01,"radius 40m quarter circle; 1001 angular reference samples");
 error=0;for(int i=0;i<=1000;++i){double s=220.0*i/1000;auto q=f.graph.edges[6].geometry.At(s);error=std::max(error,(q.position-Vec3{160+s*std::sqrt(.9996),0,.02*s}).Length());}
 recordCheck("T02.AnalyticGrade",error<=.01,error,.01,"true arc 220m; dz/ds=.02; 1001 samples");
 error=0;bool ok=true;for(const auto& t:f.graph.transitions){const auto* a=f.graph.Find(t.from.edge);const auto* b=f.graph.Find(t.to.edge);Sample x,y;ok&=f.graph.SamplePosition({a->id,t.from.direction==1?a->lengthMetres:0,t.from.direction},x).ok;ok&=f.graph.SamplePosition({b->id,t.to.direction==1?0:b->lengthMetres,t.to.direction},y).ok;error=std::max(error,(x.position-y.position).Length());}
 recordCheck("T02.AllDirectedSeams",ok&&error<=.01,error,.01,"14 explicit transitions including reverse; positional C0 seams");
 Simulation sim;ok=sim.Initialize(f.graph,f.definitions,f.Long(),{EdgeId(1),1},200,f.SelectUpper()).ok;
 double arcError=0;bool spans=false;for(int i=1;i<=800&&ok;++i){ok&=sim.AdvanceDistance(.5).ok;if(i==100)ok&=sim.SetChoice({EdgeId(1),1},TransitionId(1)).ok;std::map<EdgeId,int> occupied;for(const auto& v:sim.Current().vehicles){ok&=v.trackCentre.edge!=EdgeId(2);occupied[v.trackCentre.edge]++;}spans|=occupied.size()>=3;arcError=std::max(arcError,std::abs(sim.Current().headStationMetres-(200+.5*i)));}
 recordCheck("T03.LongCommittedBranch",ok&&spans&&sim.Current().head.edge==EdgeId(7),arcError,.01,"8 vehicles; selection changed after entry; no member enters edge2; at least 3 occupied edges");
 ok&=sim.SetCommandedSpeed(-15).ok;for(int i=0;i<800&&ok;++i)ok&=sim.AdvanceDistance(-.5).ok;
 recordCheck("T02.ReverseRetainedPath",ok&&std::abs(sim.Current().headStationMetres-200)<=.01&&sim.Current().leadingEnd.direction==-1&&sim.History().Validate(f.graph).ok,std::abs(sim.Current().headStationMetres-200),.01);
 const double before=sim.Current().headStationMetres;const auto historyCount=sim.History().occurrences.size();recordCheck("Traversal.AtomicEndOfTrack",!sim.AdvanceDistance(-1000).ok&&sim.Current().headStationMetres==before&&sim.History().occurrences.size()==historyCount);
 Simulation lower;ok=lower.Initialize(f.graph,f.definitions,f.Short(),{EdgeId(1),1},200,f.SelectUpper(false)).ok&&lower.AdvanceDistance(100).ok;recordCheck("Traversal.LowerBranch",ok&&lower.Current().head.edge==EdgeId(2));
 Simulation ambiguous;ok=ambiguous.Initialize(f.graph,f.definitions,f.Short(),{EdgeId(1),1},200,{}).ok;recordCheck("Traversal.AmbiguousRejected",ok&&!ambiguous.AdvanceDistance(100).ok&&ambiguous.Current().headStationMetres==200);
 Simulation prepended;ok=prepended.Initialize(f.graph,f.definitions,f.Long(),{EdgeId(7),1},100,f.SelectUpper()).ok;recordCheck("History.PrependAtMerge",ok&&prepended.History().occurrences.front().directed.edge==EdgeId(5)&&prepended.History().Validate(f.graph).ok);
 Simulation reverse;ok=reverse.Initialize(f.graph,f.definitions,f.Short(),{EdgeId(7),-1},100,f.SelectUpper()).ok&&reverse.AdvanceDistance(180).ok;recordCheck("Traversal.NegativeDirectedStart",ok&&reverse.Current().head.edge==EdgeId(6)&&reverse.Current().head.direction==-1);
 Simulation oriented;ok=oriented.Initialize(f.graph,f.definitions,f.Long(true),{EdgeId(1),1},200,f.SelectUpper()).ok;for(size_t i=0;i<oriented.Current().vehicles.size();++i){const auto& v=oriented.Current().vehicles[i];ok&=v.id==VehicleId(i+1)&&v.bodyForward.x<-.999&&v.frontBogie.x<v.rearBogie.x;}
 recordCheck("Placement.ReversedVehicleOrdering",ok&&oriented.Current().vehicles.size()==8);
 // Independent quarter-circle reference for both bogies of a single coach.
 Formation single{{{VehicleId(42),DefinitionId(2),1}},0};Simulation curve;
 ok=curve.Initialize(f.graph,f.definitions,single,{EdgeId(3),1},40,f.SelectUpper()).ok;
 error=0;if(ok){const auto& v=curve.Current().vehicles[0];auto expected=[](double s){return Vec3{40*std::sin(s/40),40*(1-std::cos(s/40)),0};};error=std::max((v.frontBogie-expected(37)).Length(),(v.rearBogie-expected(21)).Length());ok&=(v.bodyCentre-(expected(37)+expected(21))*.5).Length()<1e-9;}
 recordCheck("T04.BogieAndBodyCurve",ok&&error<=.05,error,.05,"coach centre station29m; bogies37m/21m; body midpoint of chord");
 const auto far=MakeFixture({10000000,-10000000,1000000});Simulation distant;ok=distant.Initialize(far.graph,far.definitions,single,{EdgeId(3),1},40,far.SelectUpper()).ok;
 error=ok?(distant.Current().vehicles[0].frontBogie-Vec3{10000000,-10000000,1000000}-curve.Current().vehicles[0].frontBogie).Length():1;
 recordCheck("T04.LargeCoordinate",ok&&error<=.05,error,.05,"translation 10000km x/y and 1000km z; double precision kernel");
 error=0;ok=true;for(int fps:{30,60,144}){Simulation timed;ok&=timed.Initialize(f.graph,f.definitions,f.Long(),{EdgeId(1),1},200,f.SelectUpper()).ok;ok&=timed.SetCommandedSpeed(10).ok;for(int i=0;i<fps*20;++i)ok&=timed.Update(1.0/fps).ok;ok&=timed.Current().tick==600;error=std::max(error,std::abs(timed.Current().headStationMetres-400));}
 recordCheck("FixedStep.RenderFPSIndependence",ok&&error<=.01,error,.01,"20s at 30/60/144 render FPS; exactly600 fixed ticks; expected head400m");
 error=0;ok=true;for(int fps:{30,60,144}){Simulation timed;ok&=timed.Initialize(f.graph,f.definitions,f.Long(),{EdgeId(1),1},200,f.SelectUpper()).ok;for(int i=0;i<fps*100;++i)ok&=timed.Update(1.0/fps,[](Simulation& s){if(s.Current().tick==0)s.SetCommandedSpeed(8);if(s.Current().tick==1500)s.SetCommandedSpeed(-8);}).ok;ok&=timed.Current().tick==3000;error=std::max(error,std::abs(timed.Current().headStationMetres-200));}
 recordCheck("FixedStep.ScriptedReverseFPSIndependence",ok&&error<=.01,error,.01,"100s forward/reverse command trace at 30/60/144 FPS; 3000 ticks; returns200m");
 auto oneWay=f.graph;oneWay.edges[0].allowReverse=false;oneWay.transitions.erase(std::remove_if(oneWay.transitions.begin(),oneWay.transitions.end(),[](const Transition& t){return t.to.edge==EdgeId(1)&&t.to.direction==-1;}),oneWay.transitions.end());Simulation restricted;
 ok=restricted.Initialize(oneWay,f.definitions,f.Short(),{EdgeId(1),1},200,f.SelectUpper()).ok;
 recordCheck("Traversal.ForbiddenReverse",ok&&!restricted.AdvanceDistance(-1).ok&&restricted.Current().headStationMetres==200);
 Simulation overloaded;overloaded.Initialize(f.graph,f.definitions,f.Short(),{EdgeId(1),1},200,f.SelectUpper());recordCheck("FixedStep.OverloadExplicitPause",!overloaded.Update(1).ok&&overloaded.Paused()&&overloaded.Current().tick==0);
 Snapshot view;ok=sim.Presentation(1,view).ok;view.vehicles.clear();view=Snapshot{};ok&=sim.Presentation(1,view).ok;recordCheck("Ownership.DiscardRecreateSnapshot",ok&&view.vehicles.size()==8&&sim.Current().headStationMetres==before);
 return report;
}
}
