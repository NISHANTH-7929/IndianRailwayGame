#pragma once
#include "RailKernel.h"

namespace Rail {
// Immutable content boundary. Legacy definitions and placement algorithms stay intact.
struct FormationContent {
    static constexpr const char* LegacyId = "IR-S6-fixture-1";
    static constexpr const char* LhbId = "IR-S8-LHB-3A-1";
    static constexpr uint64_t LhbDefinitionId = 3001;
    static constexpr uint64_t Wap7DefinitionId = 3000;
    // Measured acquired WAP-7 envelope/pivots, not a certified prototype dimension.
    static constexpr double Wap7EnvelopeLength = 21.33043212890625;
    static constexpr double Wap7BogieOffset = 5.85;
    static constexpr double ServiceBerthHead = 141.0;
    static constexpr unsigned LhbVersion = 1;
    static constexpr double BodyLength = 23.540;
    static constexpr double CoupledLength = 24.000;
    static constexpr double BogieOffset = 7.450;
    static constexpr double BogieWheelbase = 2.560;
    static constexpr double WheelDiameter = .915;
    static bool Supports(const std::string& id) { return id==LegacyId||id==LhbId; }
    static Result Resolve(const std::string& id,Fixture& fixture,Formation& shortTrain,Formation& longTrain) {
        if(!Supports(id))return Result::Fail("Required fixture content is unavailable; record retained");
        fixture=MakeFixture();shortTrain=fixture.Short();longTrain=fixture.Long();
        if(id==LegacyId)return {};
        // Length is over coupled CBC planes; zero additional gap avoids counting
        // coupler projection twice. The visible shell is 0.230 m inside each plane.
        // Bind the acquired locomotive to its measured presentation envelope.
        // Legacy definition 1 is unchanged; its handling calibration is retained.
        fixture.definitions.emplace(DefinitionId(Wap7DefinitionId),VehicleDefinition{
            DefinitionId(Wap7DefinitionId),1,Wap7EnvelopeLength,80000,Wap7BogieOffset,-Wap7BogieOffset,true});
        fixture.definitions.emplace(DefinitionId(LhbDefinitionId),VehicleDefinition{
            DefinitionId(LhbDefinitionId),LhbVersion,CoupledLength,40000,BogieOffset,-BogieOffset,false});
        for(auto* formation:{&shortTrain,&longTrain}) {
            formation->gapMetres=0;
            for(auto& member:formation->members){
                if(member.definition==DefinitionId(2))member.definition=DefinitionId(LhbDefinitionId);
                else if(member.definition==DefinitionId(1))member.definition=DefinitionId(Wap7DefinitionId);
            }
        }
        return {};
    }
};
}
