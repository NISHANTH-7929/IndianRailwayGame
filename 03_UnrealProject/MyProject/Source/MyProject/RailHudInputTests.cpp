#include "Misc/AutomationTest.h"
#include "Simulation/RailHudInputRegions.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailHudInputTest,"IndianRailway.S9.DrivingHudInputOwnership",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailHudInputTest::RunTest(const FString& Parameters) {
    using namespace Rail::HudInputRegions;
    TestTrue(TEXT("Left pod boundary belongs to HUD"), Captures(LeftEdge, EdgeTop, true));
    TestTrue(TEXT("Right pod boundary belongs to HUD"), Captures(RightEdge, .6, true));
    TestTrue(TEXT("Top status area belongs to HUD"), Captures(.5, TopEdge, true));
    TestTrue(TEXT("Bottom instruments belong to HUD"), Captures(.5, BottomEdge, true));
    TestFalse(TEXT("Clear windscreen belongs to cab"), Captures(.5, .5, true));
    TestTrue(TEXT("Open drawer blocks the windscreen"), Captures(.5, .5, true, true));
    TestFalse(TEXT("Legacy layout keeps its previous centre area"), Captures(.5, .82, false));

    PointerRouter Router;
    PointerRouter::Samples Samples{};
    auto Step = [&](bool Allowed = true, bool Modal = false) { return Router.Update(Samples, 1000, 600, Allowed, true, Modal); };
    Samples[0] = {100, 360, true}; Step();
    Samples[0] = {500, 300, true};
    TestFalse(TEXT("Dragging a lever finger into the windscreen never pans"), Step().Pan);
    Samples[1] = {500, 300, true}; Step();
    Samples[1].X = 540;
    TestTrue(TEXT("Second finger can pan while first finger holds a lever"), Step().Pan);
    Samples[0].Down = false; Samples[1].X = 560;
    const auto Independent = Step();
    TestTrue(TEXT("Releasing first finger preserves the second finger camera drag"), Independent.Pan);
    TestFalse(TEXT("Lever release never clicks physical cab controls"), Independent.Tap);
    Samples[1].Down = false;
    TestFalse(TEXT("Camera drag release is not a cab tap"), Step().Tap);

    Samples[0] = {500, 300, true}; Step();
    Step(true, true);
    Samples[0].Down = false;
    TestFalse(TEXT("Opening drawer cancels a pending physical cab click"), Step().Tap);
    Samples[0] = {500, 300, true}; Step(); Router.Cancel();
    Samples[0].X = 600;
    TestFalse(TEXT("Focus or camera cancellation quarantines the held finger"), Step().Pan);
    Samples[0].Down = false;
    TestFalse(TEXT("Cancelled contact release cannot click a control"), Step().Tap);
    Samples[0] = {500, 300, true}; Step(); Samples[0].Down = false;
    TestTrue(TEXT("New windscreen tap works after cancellation and release"), Step().Tap);

    Samples[MousePointer] = {100, 360, true}; Step(); Samples[MousePointer].X = 500;
    TestFalse(TEXT("Mouse drag from HUD cannot become camera drag"), Step().Pan);
    Samples[MousePointer].Down = false; Step();
    Samples[MousePointer] = {500, 300, true}; Step(); Samples[MousePointer].X = 530;
    TestTrue(TEXT("Desktop mouse can pan from the windscreen"), Step().Pan);
    Samples[0] = {100, 360, true}; Samples[MousePointer].X = 560;
    TestFalse(TEXT("Active touch suppresses duplicate mouse camera input"), Step().Pan);
    Samples[MousePointer].Down = Samples[0].Down = false;
    TestFalse(TEXT("Suppressed mouse release does not click the cab"), Step().Tap);

    Samples[0] = {500, 300, true}; Step(false); Samples[0].X = 550;
    TestFalse(TEXT("Pointer started while paused stays blocked after resume"), Step().Pan);
    Samples[0].Down = false;
    TestFalse(TEXT("Paused contact never becomes a physical cab click"), Step().Tap);
    return true;
}
#endif
