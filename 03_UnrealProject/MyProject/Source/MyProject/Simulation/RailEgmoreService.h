#pragma once
#include "RailService.h"
#include "RailScenarioContent.h"
namespace Rail {
inline ServiceDefinition MakeEgmoreService(){
 ServiceDefinition D;D.id="S8-Egmore-01";D.version="IR-S8-Egmore-service-1";D.contentId=EgmoreLayout::ReservedContentId;
 D.name="Chennai Egmore platform service";D.station="Chennai Egmore";D.formation="WAP-7 + seven LHB AC 3-tier coaches";
 D.startHead=EgmoreLayout::StationMetres(EgmoreLayout::StartHeadX);D.platformFrom=EgmoreLayout::StationMetres(EgmoreLayout::PlatformFromX);
 D.platformTo=EgmoreLayout::StationMetres(EgmoreLayout::PlatformToX);D.berthHead=EgmoreLayout::StationMetres(EgmoreLayout::BerthHeadX);D.finishHead=EgmoreLayout::StationMetres(EgmoreLayout::FinishHeadX);
 D.arrivalWindowSeconds=160;D.departureWindowSeconds=190;D.expectedDurationSeconds=420;
 D.requireTrafficHold=false;D.unlock="Egmore service mastery badge";
 D.objectives[3].title="Serve the Egmore platform";D.objectives[8].title="Observe protected traffic on the adjacent track";
 D.tutorial[1]="Wait for your signal to clear. Adjacent traffic has its own protected track.";
 return D;
}
}
