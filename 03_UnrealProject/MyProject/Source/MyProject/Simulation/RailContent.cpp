#include "RailContent.h"
#include <algorithm>
#include <cmath>

namespace Rail {
const std::vector<PresentationChunk>& ContentManifest::Chunks() {
    static const std::vector<PresentationChunk> chunks = {
        {1,"/Game/IndianRailway/S6/L_ChunkA","west",true},
        {2,"/Game/IndianRailway/S6/L_ChunkB","east",true}
    };
    return chunks;
}
VisualDemand RequiredPresentation(const Snapshot& train,double speed,bool lookBack,
                                  const std::vector<Vec3>& route) {
    VisualDemand demand;
    if(train.vehicles.empty() || !std::isfinite(speed)) {
        demand.diagnostic="Invalid train presentation footprint"; return demand;
    }
    double minimum=0,maximum=0; bool first=true;
    auto include=[&](Vec3 point) {
        if(!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z))return false;
        if(first){minimum=maximum=point.x;first=false;}
        minimum=std::min(minimum,point.x);maximum=std::max(maximum,point.x);return true;
    };
    for(const auto& vehicle:train.vehicles) {
        if(!include(vehicle.frontReference)||!include(vehicle.rearReference)) {
            demand.diagnostic="Non-finite presentation footprint";return demand;
        }
    }
    for(const auto& point:route)if(!include(point)) {
        demand.diagnostic="Non-finite route preview";return demand;
    }
    // Conservative spatial margin around the complete formation covers curved
    // track and either travel direction; route preview adds junction branches.
    const double margin=std::max(lookBack?ContentManifest::LookBackMarginMetres:
        ContentManifest::RearMarginMetres,std::max(ContentManifest::AheadMinimumMetres,
        std::abs(speed)*ContentManifest::AheadSeconds));
    if(minimum-margin<=ContentManifest::BoundaryX)demand.chunks.insert(1);
    if(maximum+margin>=ContentManifest::BoundaryX)demand.chunks.insert(2);
    demand.valid=true;return demand;
}
bool EssentialPresentationReady(const VisualDemand& demand,const std::set<unsigned>& loaded) {
    if(!demand.valid)return false;
    for(auto chunk:demand.chunks)if(!loaded.count(chunk))return false;
    return true;
}
TestReport RunContentTests() {
    TestReport report;
    auto recordCheck=[&](const char* name,bool passed){report.cases.push_back({name,passed,0,0,"Pure presentation policy only; real streaming requires packaged evidence"});};
    Snapshot train;VehiclePose vehicle;vehicle.frontReference={-100,0,0};vehicle.rearReference={-120,0,0};train.vehicles.push_back(vehicle);
    const auto west=RequiredPresentation(train,0,false,{});
    recordCheck("S6-policy-west-only",west.valid&&west.chunks==std::set<unsigned>{1});
    train.vehicles[0].frontReference.x=10;
    const auto boundary=RequiredPresentation(train,0,false,{});
    recordCheck("S6-policy-whole-footprint",boundary.chunks==std::set<unsigned>({1,2}));
    recordCheck("S6-policy-essential-gate",!EssentialPresentationReady(boundary,{1})&&EssentialPresentationReady(boundary,{1,2}));
    train.vehicles[0].frontReference.x=-50;train.vehicles[0].rearReference.x=-70;
    recordCheck("S6-policy-look-back",RequiredPresentation(train,0,true,{}).chunks.count(2)==1);
    recordCheck("S6-policy-speed-preload",RequiredPresentation(train,10,false,{}).chunks.count(2)==1);
    recordCheck("S6-policy-route-preload",RequiredPresentation(train,0,false,{{10,0,0}}).chunks.count(2)==1);
    recordCheck("S6-policy-missing-footprint",!EssentialPresentationReady(RequiredPresentation({},0,false,{}),{1,2}));
    return report;
}
}
