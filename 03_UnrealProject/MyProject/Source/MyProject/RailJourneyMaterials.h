#pragma once
#include "CoreMinimal.h"
#include "Components/MeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

inline bool OriginalCabForVisualReview(){
#if WITH_EDITOR
 return FParse::Param(FCommandLine::Get(),TEXT("TambaramCabReview"))&&FParse::Param(FCommandLine::Get(),TEXT("OriginalCabReview"));
#else
 return false;
#endif
}

// Presentation-only derivative assets. Original mesh slots remain untouched.
inline void ApplyJourneyMaterials(UMeshComponent* Component){
 for(int32 Slot=0;Slot<Component->GetNumMaterials();++Slot){
  auto* Original=Component->GetMaterial(Slot);if(!Original)continue;
  const FString Source=Original->GetPathName(),Name=Original->GetName();
  const bool Cab=Source.StartsWith(TEXT("/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Materials/"))&&
   (Name==TEXT("DeskEnamel")||Name==TEXT("DeskEnamel_StaticAO")||
    Name==TEXT("CabIvory")||Name==TEXT("CabIvory_StaticAO")||
    Name==TEXT("PanelCharcoal")||Name==TEXT("PanelCharcoal_StaticAO")||
    Name==TEXT("Rubber")||Name==TEXT("Rubber_StaticAO")||
    Name==TEXT("Floor")||Name==TEXT("Floor_StaticAO")||
    Name==TEXT("SeatBlue")||Name==TEXT("SeatBlue_StaticAO")||
    Name==TEXT("Steel")||Name==TEXT("Steel_StaticAO")||
    Name==TEXT("Brass")||Name==TEXT("Brass_StaticAO")||Name==TEXT("InstrumentBezel")||
    Name==TEXT("WindowGlass")||Name==TEXT("InstrumentGlass"));
  const bool Coach=Source.StartsWith(TEXT("/Game/IndianRailway/S8/Coaches/LHB3A/"))&&
   (Name==TEXT("LHB_GlassV2")||Name==TEXT("LHB_PaintRedV2")||Name==TEXT("LHB_PaintGreyV2")||Name==TEXT("FrameSteel")||Name==TEXT("DarkSteel")||
    Name==TEXT("DustSteel")||Name==TEXT("TankIvory")||Name==TEXT("Rubber")||
    Name==TEXT("PistonSteel")||Name==TEXT("WeldSteel"));
  if(!Cab&&!Coach)continue;
  if(Cab&&!OriginalCabForVisualReview()){
   const FString CabPath=TEXT("/Game/IndianRailway/S9/CabRealismV1/Materials/")+Name+TEXT(".")+Name;
   if(auto* Finish=LoadObject<UMaterialInterface>(nullptr,*CabPath)){Component->SetMaterial(Slot,Finish);continue;}
  }
  // Only these cab surfaces had a previous journey finish. Other cab slots
  // keep their original material when a derivative is unavailable.
  if(Cab&&!(Name.StartsWith(TEXT("DeskEnamel"))||Name.StartsWith(TEXT("CabIvory"))||Name.StartsWith(TEXT("PanelCharcoal"))||Name==TEXT("WindowGlass")||Name==TEXT("InstrumentGlass")))continue;
  const FString FinishPath=TEXT("/Game/IndianRailway/S9/JourneyFinishV2/")+Name+TEXT(".")+Name;
  if(auto* Finish=LoadObject<UMaterialInterface>(nullptr,*FinishPath)){Component->SetMaterial(Slot,Finish);continue;}
  const FString Path=TEXT("/Game/IndianRailway/S9/JourneyMaterials/")+Name+TEXT(".")+Name;
  if(auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path))Component->SetMaterial(Slot,Material);
  else UE_LOG(LogTemp,Warning,TEXT("Journey finish unavailable: %s; retained authored material"),*Path);
 }
}
