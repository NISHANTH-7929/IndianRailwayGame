#include "RailWorldSnapshot.h"
#include <fstream>
#include <iomanip>
#include <iostream>
int main(int argc,char** argv){
    if(argc!=2){std::cerr<<"Usage: S6Inspect.exe checkpoint.irs\n";return 2;}
    std::ifstream file(argv[1],std::ios::binary|std::ios::ate);
    const auto size=file.tellg();
    if(!file||size<=0||size>static_cast<std::streamoff>(Rail::SnapshotRecord::MaximumBytes)){std::cerr<<"Invalid checkpoint size\n";return 1;}
    std::vector<uint8_t> bytes(static_cast<size_t>(size));file.seekg(0);
    if(!file.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()))){std::cerr<<"Read failed\n";return 1;}
    Rail::SnapshotRecord record;auto result=Rail::SnapshotRecord::Read(bytes,record);
    Rail::OperationsWorld world;if(result.ok)result=Rail::WorldSnapshotCodec::Restore(record,world);
    if(!result.ok){std::cerr<<result.error<<'\n';return 1;}
    std::cout<<std::setprecision(17)<<"{\"valid\":true,\"generation\":"<<record.Generation()<<",\"logical\":"<<world.Json()<<",\"pathHistory\":[";
    bool firstTrain=true;
    for(uint64_t id:{1ull,2ull}){
        if(!firstTrain)std::cout<<',';firstTrain=false;
        const auto* train=world.Spatial(Rail::TrainId(id));
        std::cout<<"{\"train\":"<<id<<",\"formationLengthMetres\":"<<train->Current().properties.lengthMetres<<",\"occurrences\":[";
        bool first=true;for(const auto& occurrence:train->History().occurrences){
            if(!first)std::cout<<',';first=false;
            std::cout<<"{\"edge\":"<<occurrence.directed.edge.value<<",\"direction\":"<<occurrence.directed.direction<<",\"beginMetres\":"<<occurrence.beginMetres<<",\"endMetres\":"<<occurrence.endMetres<<",\"transition\":"<<occurrence.transitionFromPrevious.value<<'}';
        }
        std::cout<<"],\"vehicles\":[";first=true;
        for(const auto& vehicle:train->Current().vehicles){
            if(!first)std::cout<<',';first=false;
            std::cout<<"{\"vehicle\":"<<vehicle.id.value<<",\"definition\":"<<vehicle.definition.value<<",\"edge\":"<<vehicle.trackCentre.edge.value<<",\"centreStationMetres\":"<<vehicle.centreStationMetres<<'}';
        }
        std::cout<<"]}";
    }
    std::cout<<"]}\n";return 0;
}
