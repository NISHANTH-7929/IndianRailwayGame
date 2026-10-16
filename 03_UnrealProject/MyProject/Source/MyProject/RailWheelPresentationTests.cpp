#include "RailWheelPresentation.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailWheelRollingTest,"IndianRailway.S8.WheelRolling",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailWheelRollingTest::RunTest(const FString&){
 for(double R:{.4575,.546}){
  TestTrue(TEXT("one circumference returns same phase"),FMath::Abs(RailWheelPresentation::Angle(2*PI*R,R))<1.e-9);
  TestTrue(TEXT("forward quarter turn"),FMath::Abs(RailWheelPresentation::Angle(PI*R*.5,R)-PI*.5)<1.e-9);
  TestTrue(TEXT("reverse quarter turn"),FMath::Abs(RailWheelPresentation::Angle(-PI*R*.5,R)+PI*.5)<1.e-9);
  const FVector Bottom(0,0,-R),Omega(0,12/R,0);
  TestTrue(TEXT("zero-slip bottom contact velocity"),(FVector(12,0,0)+FVector::CrossProduct(Omega,Bottom)).IsNearlyZero(1.e-9));
  const FQuat A(FVector::RightVector,RailWheelPresentation::Angle(14320.7,R)),B(FVector::RightVector,RailWheelPresentation::Angle(14320.7,R));
  TestTrue(TEXT("cold rebuild preserves phase"),A.Equals(B,1.e-9));
 }
 return true;
}
#endif
