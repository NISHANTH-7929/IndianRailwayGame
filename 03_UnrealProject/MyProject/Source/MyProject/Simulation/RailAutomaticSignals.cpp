#include "RailAutomaticSignals.h"
#include "RailTambaramService.h"
#include <algorithm>
#include <cmath>
#include <functional>

namespace Rail {
namespace {
constexpr double MergeMetres=60;   // signals of overlapping routes this close are one physical signal
constexpr double EndClearance=300; // no automatic signal within this distance of a route end
using Piece=RoutePiece;
using Pieces=std::vector<Piece>;
Pieces PiecesFor(const RouteDefinition& route,const Graph& graph){
    Pieces out;double s=0;
    for(const auto& e:route.path){const auto* edge=graph.Find(e.edge);const double L=edge?edge->lengthMetres:0;out.push_back({e,s,L});s+=L;}
    return out;
}
// TrackPosition distances are edge-local for either direction of travel.
bool StationOn(const Pieces& pieces,TrackPosition p,double& out){
    for(const auto& piece:pieces)if(piece.edge.edge==p.edge&&piece.edge.direction==p.direction){
        out=piece.begin+(p.direction==1?p.distanceMetres:piece.length-p.distanceMetres);return true;}
    return false;
}
TrackPosition PositionOn(const Pieces& pieces,double s){
    for(const auto& piece:pieces)if(s<=piece.begin+piece.length+1e-9||&piece==&pieces.back()){
        const double local=std::clamp(s-piece.begin,0.0,piece.length);
        return {piece.edge.edge,piece.edge.direction==1?local:piece.length-local,piece.edge.direction};}
    return {};
}
// Path-station interval [a,b] of route pieces lying on `edge` between edge-local la..lb.
template<class F> void OnPath(const SignalPlan& plan,RouteId route,EdgeId edge,double la,double lb,F&& visit){
    const auto& index=plan.pieceByEdge.at(route);const auto found=index.find(edge.value);if(found==index.end())return;
    for(size_t i:found->second){const auto& piece=plan.paths.at(route)[i];
        const double a=piece.edge.direction==1?piece.begin+la:piece.begin+piece.length-lb;
        const double b=piece.edge.direction==1?piece.begin+lb:piece.begin+piece.length-la;visit(a,b);}
}
bool Overlap(double a,double b,double c,double d){return a<d&&c<b;}
bool HistoryStation(const PathHistory& history,TrackPosition p,double& out){
    bool found=false;
    for(const auto& o:history.occurrences)if(o.directed.edge==p.edge&&o.directed.direction==p.direction){
        if(found)return false;
        out=o.beginMetres+(p.direction==1?p.distanceMetres:o.endMetres-o.beginMetres-p.distanceMetres);found=true;}
    return found;
}
struct Footprint { EdgeId edge; double from=0,to=0; TrainId train; };
std::vector<Footprint> Footprints(const OperationsWorld& world){
    std::vector<Footprint> out;
    for(const auto& pair:world.Current().authorities){
        const auto* s=world.Spatial(pair.first);if(!s)continue;
        const double head=s->Current().headStationMetres,rear=head-s->Current().properties.lengthMetres;
        for(const auto& o:s->History().occurrences){
            const double a=std::max(rear,o.beginMetres),b=std::min(head,o.endMetres);if(a>=b)continue;
            const double la=o.directed.direction==1?a-o.beginMetres:o.endMetres-b,lb=o.directed.direction==1?b-o.beginMetres:o.endMetres-a;
            out.push_back({o.directed.edge,la,lb,pair.first});
        }
    }
    return out;
}
// A site protects movement along routes where it is not the route's final position.
RouteId Governing(const SignalPlan& plan,const SignalSite& site,const OperationsWorld& world){
    RouteId fallback;
    for(auto r:site.routes){
        if(site.station.at(r)>=plan.endStation.at(r)-1)continue;
        const auto state=world.Current().routes.find(r);
        if(state!=world.Current().routes.end()&&state->second.owner.Active())return r;
        if(!fallback.value)fallback=r;
    }
    return fallback;
}
}

bool PathStation(const PathHistory& history,TrackPosition position,double& station){return HistoryStation(history,position,station);}

const char* LinesideAspectName(LinesideAspect aspect){
    switch(aspect){case LinesideAspect::Caution:return "Caution";case LinesideAspect::Attention:return "Attention";
    case LinesideAspect::Proceed:return "Proceed";default:return "Stop";}
}

Result SignalPlan::Validate() const {
    for(const auto& site:sites){
        if(site.routes.empty())return Result::Fail("Signal without a route");
        for(const auto& other:sites)if(&other!=&site&&other.position.edge==site.position.edge&&other.position.direction==site.position.direction&&
            std::abs(other.position.distanceMetres-site.position.distanceMetres)<MergeMetres-1e-6)return Result::Fail("Duplicate physical signal");
    }
    return {};
}

SignalPlan BuildSignalPlan(const OperationsDefinitions& definitions,const Graph& graph,const std::string& contentId){
    SignalPlan plan;plan.contentId=contentId;
    std::map<RouteId,Pieces> pieces;std::map<RouteId,double> origin;
    for(const auto& pair:definitions.routes){
        auto& p=pieces[pair.first];p=PiecesFor(pair.second,graph);plan.paths[pair.first]=p;
        for(size_t i=0;i<p.size();++i)plan.pieceByEdge[pair.first][p[i].edge.edge.value].push_back(i);
        double o=0,e=0;if(!StationOn(p,pair.second.origin,o)||!StationOn(p,pair.second.endpoint,e)||e<=o)continue;
        origin[pair.first]=o;plan.endStation[pair.first]=e;
    }
    auto place=[&](RouteId route,double s,SignalRole role){
        const auto p=PositionOn(pieces.at(route),s);
        for(auto& site:plan.sites)if(site.position.edge==p.edge&&site.position.direction==p.direction&&std::abs(site.position.distanceMetres-p.distanceMetres)<MergeMetres){
            if(role==SignalRole::Manual)site.role=SignalRole::Manual;return;}
        SignalSite site;site.index=uint32_t(plan.sites.size());site.role=role;site.position=p;plan.sites.push_back(site);
    };
    // Station signals first, so automatic signals never displace a route origin.
    for(const auto& pair:origin)place(pair.first,pair.second,SignalRole::Manual);
    for(const auto& pair:origin)for(double s=pair.second+AutomaticSignalSpacingMetres;s<plan.endStation.at(pair.first)-EndClearance;s+=AutomaticSignalSpacingMetres)
        place(pair.first,s,SignalRole::Automatic);
    // Every route whose path passes a physical signal reads it, including shared throat sections.
    for(auto& site:plan.sites)for(const auto& pair:origin){double s=0;
        if(StationOn(pieces.at(pair.first),site.position,s)&&s>=pair.second-.5&&s<=plan.endStation.at(pair.first)+.5){site.routes.push_back(pair.first);site.station[pair.first]=s;}}
    for(const auto& pair:origin){
        std::vector<uint32_t> along;for(const auto& site:plan.sites)if(site.station.count(pair.first))along.push_back(site.index);
        std::sort(along.begin(),along.end(),[&](uint32_t a,uint32_t b){return plan.sites[a].station.at(pair.first)<plan.sites[b].station.at(pair.first);});
        const double end=plan.endStation.at(pair.first);
        for(size_t i=0;i<along.size();++i){
            auto& site=plan.sites[along[i]];const int next=i+1<along.size()?int(along[i+1]):-1;site.next[pair.first]=next;
            const double from=site.station.at(pair.first),to=std::min(end,next>=0?plan.sites[next].station.at(pair.first)+AdequateDistanceMetres:end);
            site.sectionEnd[pair.first]=to;auto& blocks=site.sectionBlocks[pair.first];
            for(const auto& block:definitions.blocks){bool hit=false;
                for(const auto& range:block.second.coverage)OnPath(plan,pair.first,range.edge,range.from,range.to,[&](double a,double b){hit|=Overlap(a,b,from,to);});
                if(hit)blocks.push_back(block.first);}
        }
        plan.beyondEnd[pair.first]=-1;
        const auto& endpoint=definitions.routes.at(pair.first).endpoint;
        for(const auto& site:plan.sites)if(site.position.edge==endpoint.edge&&site.position.direction==endpoint.direction&&std::abs(site.position.distanceMetres-endpoint.distanceMetres)<1)plan.beyondEnd[pair.first]=int(site.index);
    }
    return plan;
}

std::vector<LinesideAspect> EvaluateSignals(const SignalPlan& plan,const OperationsWorld& world){
    const auto& definitions=world.DefinitionsView();const auto& snapshot=world.Current();
    std::vector<LinesideAspect> aspects(plan.sites.size(),LinesideAspect::Stop);
    if(plan.contentId!=world.ContentId())return aspects;
    const auto feet=Footprints(world);
    auto clear=[&](const SignalSite& site,RouteId route){
        const auto state=snapshot.routes.find(route);
        const bool granted=state!=snapshot.routes.end()&&state->second.owner.Active();
        if(site.role==SignalRole::Manual&&!granted)return false;
        const TrainId owner=granted?state->second.owner.train:TrainId();
        const double from=site.station.at(route),to=site.sectionEnd.at(route);
        // Line clear: no vehicle of any train inside the section, the passing train included.
        for(const auto& f:feet){bool hit=false;OnPath(plan,route,f.edge,f.from,f.to,[&](double a,double b){hit|=Overlap(a,b,from,to);});if(hit)return false;}
        // No conflicting reservation: the section belongs to this route's train or to nobody.
        for(const auto block:site.sectionBlocks.at(route)){
            const auto b=snapshot.blocks.find(block);if(b==snapshot.blocks.end()||!b->second.reservation.Active())continue;
            if(!granted||b->second.reservation.train!=owner)return false;
        }
        (void)definitions;return true;
    };
    std::vector<int> memo(plan.sites.size(),-1);
    std::function<LinesideAspect(uint32_t,int)> aspect=[&](uint32_t i,int depth)->LinesideAspect{
        if(memo[i]>=0)return LinesideAspect(memo[i]);
        LinesideAspect a=LinesideAspect::Stop;const auto& site=plan.sites[i];const RouteId route=Governing(plan,site,world);
        if(route.value&&depth<128&&clear(site,route)){
            int next=site.next.at(route);if(next<0)next=plan.beyondEnd.at(route);
            const LinesideAspect ahead=next>=0&&uint32_t(next)!=i?aspect(uint32_t(next),depth+1):LinesideAspect::Stop;
            a=ahead==LinesideAspect::Stop?LinesideAspect::Caution:ahead==LinesideAspect::Caution?LinesideAspect::Attention:LinesideAspect::Proceed;
        }
        memo[i]=int(a);return a;
    };
    for(uint32_t i=0;i<plan.sites.size();++i)aspects[i]=aspect(i,0);
    return aspects;
}

SignalAhead NextSignal(const SignalPlan& plan,const std::vector<LinesideAspect>& aspects,const OperationsWorld& world,TrainId train){
    SignalAhead best;const auto* s=world.Spatial(train);if(!s||aspects.size()!=plan.sites.size())return best;
    const double head=s->Current().headStationMetres;
    for(const auto& site:plan.sites){double at=0;
        if(!HistoryStation(s->History(),site.position,at)||at<=head+.5)continue;
        if(!best.found||at-head<best.distance)best={true,site.index,at-head,aspects[site.index],site.role};
    }
    return best;
}

const SignalPlan& SignalPlanFor(const OperationsWorld& world){
    static std::map<std::string,SignalPlan> cache;
    auto it=cache.find(world.ContentId());if(it!=cache.end())return it->second;
    const Simulation* any=world.Spatial(TrainId(2));if(!any&&!world.Current().authorities.empty())any=world.Spatial(world.Current().authorities.begin()->first);
    static const SignalPlan empty;if(!any)return empty;
    return cache.emplace(world.ContentId(),BuildSignalPlan(world.DefinitionsView(),any->Network(),world.ContentId())).first->second;
}

TestReport RunSignalTests(){
    TestReport report;auto expect=[&](const std::string& name,bool ok,const std::string& detail=""){report.cases.push_back({name,ok,0,0,detail});};
    const auto definition=MakeLiveTambaramService();OperationsWorld world;ServiceRuntime service(definition);
    auto r=service.Select();if(r.ok)r=service.ConfirmFormation();if(r.ok)r=service.Start(world);
    expect("S10-SIG.LiveServiceStarts",r.ok,r.error);if(!r.ok)return report;
    const auto plan=BuildSignalPlan(world.DefinitionsView(),world.Spatial(TrainId(2))->Network(),world.ContentId());
    auto v=plan.Validate();expect("S10-SIG.OnePhysicalSignalPerPosition",v.ok,v.error);
    bool origins=true,spacing=true;double widest=0;
    for(const auto& pair:world.DefinitionsView().routes){
        bool manual=false;double previous=-1;
        std::vector<std::pair<double,SignalRole>> along;
        for(const auto& site:plan.sites)if(site.station.count(pair.first))along.push_back({site.station.at(pair.first),site.role});
        std::sort(along.begin(),along.end(),[](const auto& a,const auto& b){return a.first<b.first;});
        for(const auto& a:along){if(previous>=0)widest=std::max(widest,a.first-previous);previous=a.first;}
        double o=0;const auto p=PiecesFor(pair.second,world.Spatial(TrainId(2))->Network());StationOn(p,pair.second.origin,o);
        for(const auto& a:along)manual|=a.second==SignalRole::Manual&&std::abs(a.first-o)<MergeMetres;
        origins&=manual;
    }
    spacing=widest<=AutomaticSignalSpacingMetres+1;
    expect("S10-SIG.StationSignalAtEveryRouteOrigin",origins);
    expect("S10-SIG.AutomaticSpacingWithinOneKilometre",spacing,std::to_string(widest));
    // Drive the complete assisted journey; check the aspect rules on the authoritative state.
    r=service.Input(world,CommandKind::Brake,0);if(r.ok)r=service.Input(world,CommandKind::Throttle,1);
    bool chain=true,rear=true,authority=true,heldAtStarter=false,clearedStarter=false,sawCaution=false,sawAttention=false,sawProceed=false;
    std::string firstFault;
    for(unsigned i=0;i<85000&&r.ok&&service.Current().phase==ServicePhase::Running;++i){
        r=service.Update(world,1.0/30);if(!r.ok||i%15)continue;
        const auto aspects=EvaluateSignals(plan,world);
        for(const auto& site:plan.sites){
            const auto a=aspects[site.index];if(a==LinesideAspect::Stop)continue;
            const RouteId route=Governing(plan,site,world);int next=site.next.at(route);if(next<0)next=plan.beyondEnd.at(route);
            const auto n=next>=0?aspects[next]:LinesideAspect::Stop;
            const auto expected=n==LinesideAspect::Stop?LinesideAspect::Caution:n==LinesideAspect::Caution?LinesideAspect::Attention:LinesideAspect::Proceed;
            if(a!=expected){chain=false;if(firstFault.empty())firstFault="chain at site "+std::to_string(site.index);}
        }
        for(const auto& pair:world.Current().authorities){
            const auto* s=world.Spatial(pair.first);const auto* h=world.Handling(pair.first);if(!s||!h)continue;
            const double head=s->Current().headStationMetres,tail=head-s->Current().properties.lengthMetres;
            // A signal the train has passed (still over its rear) protects it at danger.
            for(const auto& site:plan.sites){double at=0;if(HistoryStation(s->History(),site.position,at)&&at>tail+1&&at<head-1&&aspects[site.index]!=LinesideAspect::Stop){
                rear=false;if(firstFault.empty())firstFault="clear signal over train "+std::to_string(pair.first.value);}}
            // A clear aspect never leads a train beyond its movement authority.
            const auto ahead=NextSignal(plan,aspects,world,pair.first);const auto& a=pair.second;
            if(ahead.found&&ahead.aspect!=LinesideAspect::Stop&&a.valid&&a.routeGranted){
                const auto& site=plan.sites[ahead.site];const RouteId route=Governing(plan,site,world);
                // The next stop signal (not its overlap) is where the train may be required to stop.
                const int next=site.next.at(route);
                const double stopAt=std::min(plan.endStation.at(route),next>=0?plan.sites[next].station.at(route):plan.endStation.at(route));
                const double reach=head+ahead.distance+(stopAt-site.station.at(route));
                const auto owner=world.Current().routes.at(route).owner;
                if(owner.train==pair.first&&reach>a.maximumHead+1){authority=false;if(firstFault.empty())firstFault="proceed beyond authority, train "+std::to_string(pair.first.value);}
            }
            if(pair.first==TrainId(2)&&ahead.found){
                heldAtStarter|=ahead.role==SignalRole::Manual&&ahead.aspect==LinesideAspect::Stop&&std::abs(h->speedMps)<.05;
                clearedStarter|=heldAtStarter&&ahead.aspect!=LinesideAspect::Stop;
                sawCaution|=ahead.aspect==LinesideAspect::Caution;sawAttention|=ahead.aspect==LinesideAspect::Attention;sawProceed|=ahead.aspect==LinesideAspect::Proceed;
            }
        }
    }
    expect("S10-SIG.JourneyRuns",r.ok,r.error);
    expect("S10-SIG.AspectSequenceRedYellowDoubleYellowGreen",chain,firstFault);
    expect("S10-SIG.SignalInRearOfTrainAtDanger",rear,firstFault);
    expect("S10-SIG.ClearAspectWithinMovementAuthority",authority,firstFault);
    expect("S10-SIG.PlayerHeldAtStarterThenCleared",heldAtStarter&&clearedStarter);
    expect("S10-SIG.PlayerSeesAllFourAspects",sawCaution&&sawAttention&&sawProceed);
    return report;
}
}
