#include "RailKernel.h"
#include <algorithm>
#include <cmath>
#include <set>

namespace Rail
{
namespace
{
bool Finite(double v) { return std::isfinite(v); }
bool Finite(Vec3 v) { return Finite(v.x) && Finite(v.y) && Finite(v.z); }
bool Direction(int d) { return d == 1 || d == -1; }
bool Allowed(const Edge& e, int d) { return Direction(d) && (d == 1 ? e.allowForward : e.allowReverse); }
NodeId Entry(const Edge& e, int d) { return d == 1 ? e.start : e.end; }
NodeId Exit(const Edge& e, int d) { return d == 1 ? e.end : e.start; }
const Transition* Select(const Graph& graph, DirectedEdge from, const Choices& choices)
{
    const auto choice = choices.find(from);
    if (choice != choices.end())
    {
        const auto* t = graph.Find(choice->second);
        return t && t->from == from ? t : nullptr;
    }
    const Transition* result = nullptr;
    for (const auto& t : graph.transitions) if (t.from == from)
    {
        if (result) return nullptr; // An ambiguous junction NEVER gets an implicit first branch.
        result = &t;
    }
    return result;
}
}
double Vec3::Length() const { return std::sqrt(x*x+y*y+z*z); }
Vec3 Vec3::Unit() const { const double n=Length(); return n>1e-12 ? *this*(1.0/n) : Vec3{}; }
Sample Geometry::At(double s) const
{
    const double h=std::sqrt(std::max(0.0,1.0-gradeDzDs*gradeDzDs));
    const double a=yawRadians+curvaturePerMetre*s;
    Vec3 p=origin;
    if (std::abs(curvaturePerMetre)<1e-12)
        p=p+Vec3{h*std::cos(yawRadians)*s,h*std::sin(yawRadians)*s,gradeDzDs*s};
    else
        p=p+Vec3{h*(std::sin(a)-std::sin(yawRadians))/curvaturePerMetre,
                 h*(std::cos(yawRadians)-std::cos(a))/curvaturePerMetre,gradeDzDs*s};
    return {p,{h*std::cos(a),h*std::sin(a),gradeDzDs}};
}
const Edge* Graph::Find(EdgeId id) const { for(const auto& e:edges) if(e.id==id)return &e; return nullptr; }
const Transition* Graph::Find(TransitionId id) const { for(const auto& t:transitions)if(t.id==id)return &t;return nullptr; }
const Transition* Graph::Connection(DirectedEdge from, DirectedEdge to) const
{ for(const auto& t:transitions)if(t.from==from && t.to==to)return &t;return nullptr; }
Result Graph::Validate() const
{
    if(nodes.empty() || edges.empty()) return Result::Fail("Graph is empty");
    std::map<NodeId,Vec3> positions;
    for(const auto& n:nodes)
        if(!n.id.value || !Finite(n.position) || !positions.emplace(n.id,n.position).second)
            return Result::Fail("Invalid or duplicate node ID/position");
    std::set<EdgeId> edgeIds;
    for(const auto& e:edges)
    {
        if(!e.id.value || !edgeIds.insert(e.id).second || !positions.count(e.start) || !positions.count(e.end))
            return Result::Fail("Invalid edge ID/endpoints");
        const auto& g=e.geometry;
        if(!Finite(e.lengthMetres) || e.lengthMetres<=0 || !Finite(g.lengthMetres) || g.lengthMetres<=0 ||
           !Finite(g.origin) || !Finite(g.yawRadians) || !Finite(g.curvaturePerMetre) ||
           !Finite(g.gradeDzDs) || std::abs(g.gradeDzDs)>=1 || (!e.allowForward && !e.allowReverse))
            return Result::Fail("Invalid edge length/geometry/permissions");
        if(std::abs(e.lengthMetres-g.lengthMetres)>1e-7 ||
           (g.At(0).position-positions[e.start]).Length()>1e-6 ||
           (g.At(e.lengthMetres).position-positions[e.end]).Length()>1e-6)
            return Result::Fail("Geometry length or node seam mismatch");
    }
    std::set<TransitionId> transitionIds;
    std::set<std::pair<DirectedEdge,DirectedEdge>> pairs;
    for(const auto& t:transitions)
    {
        const auto* a=Find(t.from.edge); const auto* b=Find(t.to.edge);
        if(!t.id.value || !transitionIds.insert(t.id).second || !pairs.emplace(t.from,t.to).second ||
           !a || !b || !Allowed(*a,t.from.direction) || !Allowed(*b,t.to.direction) ||
           Exit(*a,t.from.direction)!=Entry(*b,t.to.direction))
            return Result::Fail("Invalid/duplicate directed transition");
    }
    return {};
}
Result Graph::SamplePosition(TrackPosition p, Sample& sample) const
{
    const auto* e=Find(p.edge);
    if(!e || !Allowed(*e,p.direction) || !Finite(p.distanceMetres) || p.distanceMetres<0 || p.distanceMetres>e->lengthMetres)
        return Result::Fail("Invalid canonical track position");
    sample=e->geometry.At(p.distanceMetres); sample.tangent=sample.tangent*double(p.direction); return {};
}
Result PathHistory::Initialize(const Graph& graph, DirectedEdge edge)
{
    const auto* e=graph.Find(edge.edge);
    if(!e || !Allowed(*e,edge.direction))return Result::Fail("Invalid initial path edge");
    occurrences={{edge,0,e->lengthMetres,TransitionId{}}}; return {};
}
Result PathHistory::Cover(const Graph& graph,double minimum,double maximum,const Choices& choices)
{
    if(occurrences.empty() || !Finite(minimum) || !Finite(maximum) || minimum>maximum)
        return Result::Fail("Invalid history extent");
    PathHistory candidate=*this;
    while(minimum < candidate.occurrences.front().beginMetres)
    {
        if(candidate.occurrences.size()>=MaxOccurrences)return Result::Fail("History capacity exhausted");
        const auto first=candidate.occurrences.front();
        const auto* back=Select(graph,first.directed.Reversed(),choices);
        if(!back)return Result::Fail("Missing or ambiguous reverse junction choice/end of track");
        const auto newDirected=back->to.Reversed();
        const auto* link=graph.Connection(newDirected,first.directed);
        const auto* e=graph.Find(newDirected.edge);
        if(!link || !e)return Result::Fail("No reciprocal legal path transition");
        candidate.occurrences.front().transitionFromPrevious=link->id;
        candidate.occurrences.insert(candidate.occurrences.begin(),
            {newDirected,first.beginMetres-e->lengthMetres,first.beginMetres,TransitionId{}});
    }
    while(maximum > candidate.occurrences.back().endMetres)
    {
        if(candidate.occurrences.size()>=MaxOccurrences)return Result::Fail("History capacity exhausted");
        const auto last=candidate.occurrences.back();
        const auto* link=Select(graph,last.directed,choices);
        if(!link)return Result::Fail("Missing or ambiguous forward junction choice/end of track");
        const auto* e=graph.Find(link->to.edge);
        if(!e || !Allowed(*e,link->to.direction))return Result::Fail("Invalid history destination");
        candidate.occurrences.push_back({link->to,last.endMetres,last.endMetres+e->lengthMetres,link->id});
    }
    const auto valid=candidate.Validate(graph); if(!valid.ok)return valid;
    *this=std::move(candidate);return {};
}
Result PathHistory::Locate(const Graph& graph,double station,TrackPosition& position) const
{
    if(occurrences.empty() || !Finite(station) || station<occurrences.front().beginMetres-1e-9 || station>occurrences.back().endMetres+1e-9)
        return Result::Fail("Requested station is outside retained history");
    for(size_t i=0;i<occurrences.size();++i)
    {
        const auto& o=occurrences[i];
        if(station<o.endMetres || i+1==occurrences.size())
        {
            const auto* e=graph.Find(o.directed.edge);if(!e)return Result::Fail("History refers to absent edge");
            const double local=std::clamp(station-o.beginMetres,0.0,e->lengthMetres);
            position={e->id,o.directed.direction==1 ? local : e->lengthMetres-local,o.directed.direction};return {};
        }
    }
    return Result::Fail("History lookup failed");
}
Result PathHistory::SampleAt(const Graph& graph,double station,Sample& sample) const
{ TrackPosition p;const auto r=Locate(graph,station,p);return r.ok?graph.SamplePosition(p,sample):r; }
Result PathHistory::Validate(const Graph& graph) const
{
    if(occurrences.empty() || occurrences.size()>MaxOccurrences)return Result::Fail("Invalid history size");
    for(size_t i=0;i<occurrences.size();++i)
    {
        const auto& o=occurrences[i];const auto* e=graph.Find(o.directed.edge);
        if(!e || !Allowed(*e,o.directed.direction) || !Finite(o.beginMetres) || !Finite(o.endMetres) ||
           std::abs(o.endMetres-o.beginMetres-e->lengthMetres)>1e-7)return Result::Fail("Invalid history occurrence");
        if(i)
        {
            const auto* t=graph.Find(o.transitionFromPrevious);
            if(!t || !(t->from==occurrences[i-1].directed) || !(t->to==o.directed) ||
                std::abs(occurrences[i-1].endMetres-o.beginMetres)>1e-7)return Result::Fail("History transition/seam mismatch");
        }
        else if(o.transitionFromPrevious.value)return Result::Fail("First history occurrence has predecessor");
    }
    return {};
}
Result VehicleDefinition::Validate() const
{
    if(!id.value || !version || !Finite(lengthMetres) || lengthMetres<=0 || !Finite(massKg) || massKg<=0 ||
       !Finite(frontBogieOffsetMetres) || !Finite(rearBogieOffsetMetres) ||
       frontBogieOffsetMetres<=rearBogieOffsetMetres || frontBogieOffsetMetres>lengthMetres*.5 ||
       rearBogieOffsetMetres< -lengthMetres*.5)
        return Result::Fail("Invalid vehicle definition/dimensions/mass/bogie ordering");
    return {};
}
Result Formation::Derive(const Definitions& definitions,FormationProperties& properties) const
{
    if(members.empty() || !Finite(gapMetres) || gapMetres<0)return Result::Fail("Invalid formation members/gap");
    FormationProperties result;std::set<VehicleId> ids;
    for(const auto& m:members)
    {
        const auto d=definitions.find(m.definition);
        if(!m.id.value || !ids.insert(m.id).second || !Direction(m.orientation) || d==definitions.end() ||
            d->first!=d->second.id || !d->second.Validate().ok)return Result::Fail("Invalid formation member/definition");
        result.lengthMetres+=d->second.lengthMetres;result.massKg+=d->second.massKg;
        result.poweredCount+=d->second.powered?1:0;++result.vehicleCount;
    }
    result.lengthMetres+=gapMetres*double(members.size()-1);
    if(!Finite(result.lengthMetres) || !Finite(result.massKg))return Result::Fail("Formation overflow");
    properties=result;return {};
}
Result Place(const Graph& graph,const Definitions& definitions,const Formation& formation,
             const PathHistory& history,double head,Snapshot& snapshot)
{
    FormationProperties props;auto r=formation.Derive(definitions,props);if(!r.ok)return r;
    Snapshot result=snapshot;result.properties=props;result.headStationMetres=head;result.vehicles.clear();
    r=history.Locate(graph,head,result.head);if(!r.ok)return r;
    const double leadingStation=result.commandedSpeedMps<0 ? head-props.lengthMetres : head;
    r=history.Locate(graph,leadingStation,result.leadingEnd);if(!r.ok)return r;
    if(result.commandedSpeedMps<0)result.leadingEnd.direction*=-1;
    double offset=0;
    for(const auto& member:formation.members)
    {
        const auto& d=definitions.at(member.definition);const double centre=head-offset-d.lengthMetres*.5;
        Sample front,rear,frontBogie,rearBogie;
        r=history.SampleAt(graph,centre+member.orientation*d.lengthMetres*.5,front);if(!r.ok)return r;
        r=history.SampleAt(graph,centre-member.orientation*d.lengthMetres*.5,rear);if(!r.ok)return r;
        r=history.SampleAt(graph,centre+member.orientation*d.frontBogieOffsetMetres,frontBogie);if(!r.ok)return r;
        r=history.SampleAt(graph,centre+member.orientation*d.rearBogieOffsetMetres,rearBogie);if(!r.ok)return r;
        VehiclePose pose;pose.id=member.id;pose.definition=member.definition;pose.centreStationMetres=centre;pose.lengthMetres=d.lengthMetres;
        r=history.Locate(graph,centre,pose.trackCentre);if(!r.ok)return r;pose.trackCentre.direction*=member.orientation;
        pose.frontReference=front.position;pose.rearReference=rear.position;
        pose.frontBogie=frontBogie.position;pose.rearBogie=rearBogie.position;
        pose.bodyForward=(frontBogie.position-rearBogie.position).Unit();
        if(pose.bodyForward.Length()<.5)return Result::Fail("Degenerate bogie/body chord");
        // Interpolate between asymmetric bogies to the authored local body origin.
        const double weight=-d.rearBogieOffsetMetres/(d.frontBogieOffsetMetres-d.rearBogieOffsetMetres);
        pose.bodyCentre=rearBogie.position+(frontBogie.position-rearBogie.position)*weight;
        result.vehicles.push_back(pose);offset+=d.lengthMetres+formation.gapMetres;
    }
    snapshot=std::move(result);return {};
}
Result Simulation::Initialize(Graph graph,Definitions definitions,Formation formation,DirectedEdge start,
                              double head,Choices choices,TrainId train)
{
    auto r=graph.Validate();if(!r.ok)return r;
    if(!train.value || !Finite(head) || !Finite(FixedStepSeconds) || FixedStepSeconds<=0 || !MaxStepsPerUpdate)
        return Result::Fail("Invalid initial simulation parameters");
    FormationProperties props;r=formation.Derive(definitions,props);if(!r.ok)return r;
    for(const auto& choice:choices){const auto* t=graph.Find(choice.second);if(!t || !(t->from==choice.first))return Result::Fail("Invalid junction choice");}
    PathHistory history;r=history.Initialize(graph,start);if(!r.ok)return r;
    r=history.Cover(graph,head-props.lengthMetres,head,choices);if(!r.ok)return r;
    Snapshot snapshot;snapshot.train=train;r=Place(graph,definitions,formation,history,head,snapshot);if(!r.ok)return r;
    graph_=std::move(graph);definitions_=std::move(definitions);formation_=std::move(formation);choices_=std::move(choices);
    history_=std::move(history);current_=std::move(snapshot);previousHead_=head;accumulator_=0;paused_=false;error_.clear();return {};
}
Result Simulation::AdvanceDistance(double metres)
{
    if(paused_ || !Finite(metres))return Result::Fail("Simulation paused or nonfinite distance");
    const double head=current_.headStationMetres+metres;
    auto history=history_;auto snapshot=current_;
    auto r=history.Cover(graph_,head-current_.properties.lengthMetres,head,choices_);
    if(r.ok && metres<0)
    {
        for(size_t i=0;i<history.occurrences.size();++i)
        {
            const auto& o=history.occurrences[i];
            if(o.endMetres<head-current_.properties.lengthMetres || o.beginMetres>current_.headStationMetres)continue;
            const auto* e=graph_.Find(o.directed.edge);
            if(!e || !Allowed(*e,-o.directed.direction))return Result::Fail("Reverse motion forbidden on occupied edge");
            if(i && o.beginMetres>head-current_.properties.lengthMetres && o.beginMetres<current_.headStationMetres &&
               !graph_.Connection(o.directed.Reversed(),history.occurrences[i-1].directed.Reversed()))
                return Result::Fail("Reverse motion has no legal committed inverse transition");
        }
    }
    if(r.ok)r=Place(graph_,definitions_,formation_,history,head,snapshot);
    if(!r.ok)return r;
    previousHead_=current_.headStationMetres;history_=std::move(history);current_=std::move(snapshot);return {};
}
Result Simulation::SetCommandedSpeed(double speed)
{
    if(!Finite(speed))return Result::Fail("Nonfinite commanded speed");
    auto candidate=current_;candidate.commandedSpeedMps=speed;
    auto r=Place(graph_,definitions_,formation_,history_,candidate.headStationMetres,candidate);
    if(r.ok)current_=std::move(candidate);return r;
}
Result Simulation::SetChoice(DirectedEdge incoming,TransitionId choice)
{
    const auto* t=graph_.Find(choice);if(!t || !(t->from==incoming))return Result::Fail("Illegal junction choice");
    choices_[incoming]=choice;return {};
}
Result Simulation::Update(double dt,const std::function<void(Simulation&)>& beforeStep)
{
    return UpdateSteps(dt,[&](Simulation& simulation,double stepSeconds){
        if(beforeStep)beforeStep(simulation);
        return simulation.AdvanceDistance(simulation.Current().commandedSpeedMps*stepSeconds);
    });
}
Result Simulation::UpdateSteps(double dt,const std::function<Result(Simulation&,double)>& step)
{
    return AdvanceFixedClock(dt,FixedStepSeconds,MaxStepsPerUpdate,accumulator_,paused_,error_,[&]{
        return AdvanceLogicalStep([&](Simulation& s){return step(s,FixedStepSeconds);});
    });
}
Result Simulation::AdvanceLogicalStep(const std::function<Result(Simulation&)>& step)
{
    if(paused_)return Result::Fail(error_.empty()?"Simulation paused":error_);
    auto r=step(*this);if(r.ok)++current_.tick;return r;
}
Result Simulation::CommitHistoryExtent(double minimum,double maximum)
{
    if(paused_)return Result::Fail("Simulation paused");
    return history_.Cover(graph_,minimum,maximum,choices_);
}
Result AdvanceFixedClock(double dt,double stepSeconds,unsigned maximumSteps,
                         double& accumulator,bool& paused,std::string& error,const std::function<Result()>& step)
{
    if(paused)return Result::Fail(error.empty()?"Simulation paused":error);
    if(!Finite(dt)||dt<0||!Finite(stepSeconds)||stepSeconds<=0||!maximumSteps||!step)return Result::Fail("Invalid simulation delta/step");
    accumulator+=dt;
    if(accumulator>stepSeconds*double(maximumSteps)+1e-9)
    {paused=true;error="Fixed-step overload: paused with time debt retained";return Result::Fail(error);}
    unsigned steps=0;
    while(accumulator+1e-10>=stepSeconds && steps<maximumSteps)
    {
        const auto r=step();
        if(!r.ok){paused=true;error=r.error;return r;}
        accumulator=std::max(0.0,accumulator-stepSeconds);++steps;
    }
    return {};
}
double Simulation::InterpolationAlpha() const{return std::clamp(accumulator_/FixedStepSeconds,0.0,1.0);}
Result Simulation::Presentation(double alpha,Snapshot& out) const
{
    if(!Finite(alpha))return Result::Fail("Nonfinite interpolation");
    out=current_;return Place(graph_,definitions_,formation_,history_,
        previousHead_+(current_.headStationMetres-previousHead_)*std::clamp(alpha,0.0,1.0),out);
}
Fixture MakeFixture(Vec3 translation)
{
    Fixture f;
    auto edge=[&](uint64_t id,uint64_t start,uint64_t end,Geometry g)
    {g.origin=g.origin+translation;f.graph.edges.push_back({EdgeId(id),NodeId(start),NodeId(end),g.lengthMetres,g,true,true});};
    edge(1,1,2,{{-240,0,0},0,0,0,240});
    edge(2,2,6,{{0,0,0},0,0,0,160});
    edge(3,2,3,{{0,0,0},0,1.0/40,0,20*Pi});
    edge(4,3,4,{{40,40,0},Pi/2,-1.0/40,0,20*Pi});
    edge(5,4,5,{{80,80,0},0,-1.0/40,0,20*Pi});
    edge(6,5,6,{{120,40,0},-Pi/2,1.0/40,0,20*Pi});
    edge(7,6,7,{{160,0,0},0,0,.02,220});
    std::map<NodeId,Vec3> nodes;
    for(const auto& e:f.graph.edges){nodes[e.start]=e.geometry.At(0).position;nodes[e.end]=e.geometry.At(e.lengthMetres).position;}
    for(const auto& n:nodes)f.graph.nodes.push_back({n.first,n.second});
    auto link=[&](uint64_t id,uint64_t a,uint64_t b)
    { f.graph.transitions.push_back({TransitionId(id),{EdgeId(a),1},{EdgeId(b),1}});
      f.graph.transitions.push_back({TransitionId(id+100),{EdgeId(b),-1},{EdgeId(a),-1}}); };
    link(1,1,2);link(2,1,3);link(3,3,4);link(4,4,5);link(5,5,6);link(6,6,7);link(7,2,7);
    f.definitions.emplace(DefinitionId(1),VehicleDefinition{DefinitionId(1),1,20,80000,7,-7,true});
    f.definitions.emplace(DefinitionId(2),VehicleDefinition{DefinitionId(2),1,22,40000,8,-8,false});
    return f;
}
Formation Fixture::Short(bool reversed) const
{return {{{VehicleId(1),DefinitionId(1),reversed?-1:1},{VehicleId(2),DefinitionId(2),reversed?-1:1}},1};}
Formation Fixture::Long(bool reversed) const
{
    Formation f;f.members.push_back({VehicleId(1),DefinitionId(1),reversed?-1:1});
    for(uint64_t i=2;i<=8;++i)f.members.push_back({VehicleId(i),DefinitionId(2),reversed?-1:1});return f;
}
Choices Fixture::SelectUpper(bool upper) const
{return {{{EdgeId(1),1},TransitionId(upper?2:1)},{{EdgeId(7),-1},TransitionId(upper?106:107)}};}
}
