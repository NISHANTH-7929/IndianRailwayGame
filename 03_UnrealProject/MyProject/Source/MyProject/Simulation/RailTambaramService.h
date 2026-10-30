#pragma once
#include "RailService.h"
#include "RailTambaramRoute.h"
#include "RailRouteSampling.h"
#include "RailScenarioContent.h"
namespace Rail {
TestReport RunTambaramJourneyTests();
inline ServiceDefinition MakeEgmoreTambaramService(){
 ServiceDefinition D;D.id="S8-MS-TBM-01";D.version="IR-S8-MSTBM-service-7";D.contentId=TambaramRoute::ContentId;
 D.name="Chennai Egmore to Tambaram representative corridor";D.station="Tambaram";D.formation="WAP-7 + seven LHB AC 3-tier coaches";
 D.startHead=TambaramRoute::StartHead;D.platformFrom=TambaramRoute::TambaramPlatformFrom;D.platformTo=TambaramRoute::TambaramPlatformTo;
 D.berthHead=TambaramRoute::TambaramBerthHead;D.finishHead=TambaramRoute::TambaramBerthHead;D.speedLimit=100.0/3.6;D.assistedCruise=90.0/3.6;D.terminalStop=true;
 TrackPosition Berth;const auto Graph=TambaramRoute::MakeGraph();
 if(PositionOnRoute(Graph,TambaramRoute::Path(1),D.berthHead,Berth).ok)D.platformEdge=Berth.edge;
 D.arrivalWindowSeconds=2100;D.departureWindowSeconds=2200;D.expectedDurationSeconds=2400;D.requireTrafficHold=true;
 D.unlock="Egmore-Tambaram representative service badge";
 D.objectives[2].title="Observe the route speed profile and 100 km/h scenario ceiling";
 D.objectives[3].title="Serve the Tambaram destination platform";D.objectives[8].title="Wait for the complete AI train to clear the shared Egmore throat";
 D.objectives[6].title="Remain stopped at the terminal";D.objectives[7].title="Complete the Tambaram arrival and save the result";
 D.tutorial[1]="Follow the authorised southbound corridor and observe signals through the intermediate station sequence.";
 D.tutorial[2]="Berth at Tambaram, hold the complete train within the platform and remain stopped for the full dwell.";
 D.tutorial[3]="The service ends at Tambaram after dwell. Review the result and save the reward; no departure is required.";
 return D;
}
inline ServiceDefinition MakeLiveTambaramService(){
 auto D=MakeEgmoreTambaramService();D.id="S9-MS-TBM-LIVE-01";D.version="IR-S9-MSTBM-live-service-1";D.contentId="IR-S9-MSTBM-LIVE-1";
 D.name="Egmore to Tambaram - crossover and opposing traffic";
 const auto G=TambaramRoute::MakeGraph();const auto P=ScenarioContent::PlayerPath(D.contentId);
 const double Offset=RouteLength(G,P)-RouteLength(G,TambaramRoute::Path(1));
 D.platformFrom+=Offset;D.platformTo+=Offset;D.berthHead+=Offset;D.finishHead=D.berthHead;
 TrackPosition Berth;if(PositionOnRoute(G,P,D.berthHead,Berth).ok)D.platformEdge=Berth.edge;
 D.objectives[8].title="Wait for priority traffic, then cross on the protected route";
 D.tutorial[1]="Wait at the Egmore throat. Once cleared, follow the crossover at the restricted speed. Three opposing services use separate protected tracks.";
 return D;
}

}
