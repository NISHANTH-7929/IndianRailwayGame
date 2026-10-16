#include "CoreMinimal.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Simulation/RailDispatchDiagnostics.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailDispatchTest,"IndianRailway.S9.Dispatch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailDispatchTest::RunTest(const FString& Parameters){
 const auto Report=Rail::RunDispatchTests();
 if(!Report.Passed())AddError(UTF8_TO_TCHAR(Report.Json().c_str()));
 return Report.Passed();
}
#endif
