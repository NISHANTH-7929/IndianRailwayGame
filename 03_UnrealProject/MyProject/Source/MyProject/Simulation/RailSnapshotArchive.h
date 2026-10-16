#pragma once
#include "RailOperations.h"
#include <cmath>
#include <cstring>
#include <type_traits>
namespace Rail {
class SnapshotArchive {
public:
    explicit SnapshotArchive(std::vector<uint8_t>& bytes,bool read):bytes_(bytes),read_(read){}
    bool Reading() const {return read_;}
    bool Good() const {return error_.empty();}
    Result Finish() const {return !Good()?Result::Fail(error_):(read_&&cursor_!=bytes_.size()?Result::Fail("Trailing logical payload bytes"):Result{});}
    void Fail(const char* error){if(Good())error_=error;}
    template<class... T> void Fields(T&... values){(Value(values),...);}
    template<class T,std::enable_if_t<std::is_integral_v<T>||std::is_enum_v<T>,int> =0>
    void Value(T& value) {
        uint64_t bits=static_cast<uint64_t>(value);
        if(read_)bits=0;
        for(unsigned i=0;i<8&&Good();++i){
            if(read_){if(cursor_>=bytes_.size()){Fail("Truncated logical field");break;}bits|=uint64_t(bytes_[cursor_++])<<(i*8);}
            else bytes_.push_back(static_cast<uint8_t>(bits>>(i*8)));
        }
        if(read_&&Good()){
            if constexpr(std::is_same_v<T,bool>){if(bits>1){Fail("Invalid boolean");return;}}
            value=static_cast<T>(bits);
            if(static_cast<uint64_t>(value)!=bits)Fail("Out-of-range integer");
        }
    }
    void Value(double& value){
        uint64_t bits=0;static_assert(sizeof(bits)==sizeof(value),"IEEE64 required");
        if(!read_)std::memcpy(&bits,&value,sizeof(bits));Value(bits);
        if(read_)std::memcpy(&value,&bits,sizeof(value));
        if(!std::isfinite(value))Fail("Non-finite logical value");
    }
    template<class T>void Value(Id<T>& value){Value(value.value);}
    size_t Count(size_t count,size_t limit=4096){uint64_t size=count;Value(size);if(size>limit){Fail("Logical container bound exceeded");return 0;}return static_cast<size_t>(size);}
    void Value(std::string& value){
        const auto size=Count(value.size());if(!Good())return;
        if(read_){if(size>bytes_.size()-cursor_){Fail("Truncated logical string");return;}value.assign(bytes_.begin()+cursor_,bytes_.begin()+cursor_+size);cursor_+=size;}
        else bytes_.insert(bytes_.end(),value.begin(),value.end());
    }
    void Blob(std::vector<uint8_t>& value){
        const auto size=Count(value.size(),1024*1024);if(!Good())return;
        if(read_){if(size>bytes_.size()-cursor_){Fail("Truncated logical blob");return;}value.assign(bytes_.begin()+cursor_,bytes_.begin()+cursor_+size);cursor_+=size;}
        else bytes_.insert(bytes_.end(),value.begin(),value.end());
    }
    template<class T>void Value(std::vector<T>& values){Sequence(values);}
    template<class T>void Value(std::deque<T>& values){Sequence(values);}
    template<class T>void Sequence(T& values){const auto count=Count(values.size());if(read_)values.resize(count);for(auto& value:values)if(Good())Value(value);}
    template<class K,class V>void Value(std::map<K,V>& values){
        const auto count=Count(values.size());if(read_){values.clear();for(size_t i=0;i<count&&Good();++i){K key{};V value{};Fields(key,value);if(!values.emplace(key,std::move(value)).second)Fail("Duplicate map key");}}
        else for(auto& pair:values){auto key=pair.first;Fields(key,pair.second);}
    }
    template<class T>void Value(std::set<T>& values){
        const auto count=Count(values.size());if(read_){values.clear();for(size_t i=0;i<count&&Good();++i){T value{};Value(value);if(!values.insert(value).second)Fail("Duplicate set key");}}
        else for(auto value:values)Value(value);
    }
    void Value(DirectedEdge& v){Fields(v.edge,v.direction);}
    void Value(TrackPosition& v){Fields(v.edge,v.distanceMetres,v.direction);}
    void Value(Occurrence& v){Fields(v.directed,v.beginMetres,v.endMetres,v.transitionFromPrevious);}
    void Value(VehicleInstance& v){Fields(v.id,v.definition,v.orientation);}
    void Value(Formation& v){Fields(v.members,v.gapMetres);}
    void Value(TrainCommand& v){Fields(v.train,v.source,v.sequence,v.applyTick,v.kind,v.value);}
    void Value(ControlState& v){Fields(v.throttle,v.brake,v.direction,v.emergency);}
    void Value(LongitudinalState& v){Fields(v.command,v.speedMps,v.accelerationMps2,v.appliedThrottle,v.appliedBrake,v.tractionN,v.brakingN,v.resistanceN,v.gradeN,v.netN,v.massKg,v.lastAppliedSequence,v.rejectedCommands,v.held,v.lastRejection);}
    void Value(OwnerToken& v){Fields(v.route,v.train,v.generation);}
    void Value(BlockState& v){Fields(v.occupants,v.reservation);}
    void Value(PointState& v){Fields(v.position,v.requested,v.lock);}
    void Value(RouteState& v){Fields(v.owner,v.entered,v.releaseReason);}
    void Value(MovementAuthority& v){Fields(v.train,v.valid,v.routeGranted,v.owner,v.minimumRear,v.maximumHead,v.endpoint,v.restriction);}
    void Value(OperationsRequest& v){Fields(v.kind,v.train,v.route,v.point,v.position,v.sequence,v.applyTick,v.expectedGeneration);}
    void Value(RequestResult& v){Fields(v.request,v.granted,v.reason,v.tick);}
private:
    std::vector<uint8_t>& bytes_;bool read_;size_t cursor_=0;std::string error_;
};
}
