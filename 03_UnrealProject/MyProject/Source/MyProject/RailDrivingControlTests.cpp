#include "Misc/AutomationTest.h"
#include "Simulation/RailDrivingControlModel.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailDrivingControlTest,"IndianRailway.S9.DrivingControlPrecision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailDrivingControlTest::RunTest(const FString& Parameters){
 using namespace Rail::DrivingControl;
 TestEqual(TEXT("Grabbing anywhere in a rail retains current command"),DragValue(.25,400,400,177),.25);
 TestEqual(TEXT("Upward drag increases power gradually"),DragValue(.25,400,382.3,177),.35);
 TestEqual(TEXT("Downward drag reduces brake gradually"),DragValue(.25,400,417.7,177),.15);
 TestEqual(TEXT("Precision supports one percent rather than preset jumps"),DragValue(.25,400,398.23,177),.26);
 TestEqual(TEXT("Drag above rail clamps to full"),DragValue(.25,400,-200,177),1.0);
 TestEqual(TEXT("Drag below rail clamps to released"),DragValue(.25,400,900,177),0.0);
 TestEqual(TEXT("Plus is a five-percent adjustment"),StepValue(.25,1),.30);
 TestEqual(TEXT("Minus cannot command negative power"),StepValue(0,-1),0.0);
 TestEqual(TEXT("Plus cannot exceed full service"),StepValue(1,1),1.0);
 TestEqual(TEXT("Invalid geometry cannot produce NaN command"),DragValue(.25,400,390,0),0.0);
 TestEqual(TEXT("Nonfinite input cannot reach the simulation"),DragValue(.25,400,std::numeric_limits<double>::infinity(),177),0.0);
 return true;
}
#endif
