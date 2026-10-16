#pragma once
// Pure C++17: no Unreal includes, UObject pointers, actor transforms or rendering state.
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace Rail
{
template<class Tag> struct Id
{
    uint64_t value = 0;
    constexpr Id() = default;
    explicit constexpr Id(uint64_t v) : value(v) {}
    bool operator==(Id b) const { return value == b.value; }
    bool operator!=(Id b) const { return !(*this == b); }
    bool operator<(Id b) const { return value < b.value; }
};
using NodeId = Id<struct NodeTag>;
using EdgeId = Id<struct EdgeTag>;
using TransitionId = Id<struct TransitionTag>;
using DefinitionId = Id<struct DefinitionTag>;
using VehicleId = Id<struct VehicleTag>;
using TrainId = Id<struct TrainTag>;
constexpr double Pi = 3.14159265358979323846;
constexpr double MetresToCentimetres(double metres) { return metres * 100.0; }
constexpr double CentimetresToMetres(double cm) { return cm / 100.0; }
struct Vec3
{
    double x = 0, y = 0, z = 0;
    Vec3 operator+(Vec3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    Vec3 operator-(Vec3 b) const { return {x-b.x,y-b.y,z-b.z}; }
    Vec3 operator*(double s) const { return {x*s,y*s,z*s}; }
    double Length() const;
    Vec3 Unit() const;
};
struct Sample { Vec3 position, tangent; };
// Heading = yawRadians + curvaturePerMetre * true 3D arc distance.
// gradeDzDs is dz/ds, not rise/run. Zero curvature is an analytic line.
struct Geometry
{
    Vec3 origin;
    double yawRadians = 0, curvaturePerMetre = 0, gradeDzDs = 0, lengthMetres = 0;
    Sample At(double distanceMetres) const;
};
struct Node { NodeId id; Vec3 position; };
struct Edge
{
    EdgeId id; NodeId start, end;
    double lengthMetres = 0;
    Geometry geometry;
    bool allowForward = true, allowReverse = true;
};
struct DirectedEdge
{
    EdgeId edge; int direction = 1;
    bool operator==(const DirectedEdge& b) const { return edge == b.edge && direction == b.direction; }
    bool operator<(const DirectedEdge& b) const { return edge == b.edge ? direction < b.direction : edge < b.edge; }
    DirectedEdge Reversed() const { return {edge, -direction}; }
};
struct TrackPosition { EdgeId edge; double distanceMetres = 0; int direction = 1; };
struct Transition { TransitionId id; DirectedEdge from, to; };
struct Result
{
    bool ok = true; std::string error;
    static Result Fail(std::string message) { return {false, message}; }
};
// Shared accumulator policy for single-train and multi-train worlds.
Result AdvanceFixedClock(double delta,double stepSeconds,unsigned maximumSteps,
                         double& accumulator,bool& paused,std::string& error,
                         const std::function<Result()>& step);
struct Graph
{
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<Transition> transitions;
    const Edge* Find(EdgeId id) const;
    const Transition* Find(TransitionId id) const;
    const Transition* Connection(DirectedEdge from, DirectedEdge to) const;
    Result Validate() const;
    Result SamplePosition(TrackPosition position, Sample& sample) const;
};
// Choice affects only an UNVISITED tape boundary; committed occurrences are immutable.
using Choices = std::map<DirectedEdge, TransitionId>;
struct Occurrence
{
    DirectedEdge directed;
    double beginMetres = 0, endMetres = 0;
    TransitionId transitionFromPrevious;
};
struct PathHistory
{
    std::vector<Occurrence> occurrences;
    static constexpr size_t MaxOccurrences = 4096;
    Result Initialize(const Graph& graph, DirectedEdge edge);
    Result Cover(const Graph& graph, double minimum, double maximum, const Choices& choices);
    Result Locate(const Graph& graph, double station, TrackPosition& position) const;
    Result SampleAt(const Graph& graph, double station, Sample& sample) const;
    Result Validate(const Graph& graph) const;
};
struct VehicleDefinition
{
    DefinitionId id; uint32_t version = 1;
    double lengthMetres = 0, massKg = 0;
    double frontBogieOffsetMetres = 0, rearBogieOffsetMetres = 0;
    bool powered = false;
    Result Validate() const;
};
struct VehicleInstance { VehicleId id; DefinitionId definition; int orientation = 1; };
using Definitions = std::map<DefinitionId, VehicleDefinition>;
struct FormationProperties { double lengthMetres = 0, massKg = 0; size_t vehicleCount = 0, poweredCount = 0; };
struct Formation
{
    std::vector<VehicleInstance> members;
    double gapMetres = 1;
    Result Derive(const Definitions& definitions, FormationProperties& properties) const;
};
struct VehiclePose
{
    VehicleId id; DefinitionId definition;
    TrackPosition trackCentre;
    Vec3 bodyCentre, bodyForward, frontReference, rearReference, frontBogie, rearBogie;
    double centreStationMetres = 0, lengthMetres = 0;
};
struct Snapshot
{
    TrainId train;
    uint64_t tick = 0;
    double headStationMetres = 0, commandedSpeedMps = 0;
    TrackPosition head, leadingEnd;
    FormationProperties properties;
    std::vector<VehiclePose> vehicles;
};
Result Place(const Graph& graph, const Definitions& definitions, const Formation& formation,
             const PathHistory& history, double headStation, Snapshot& snapshot);
// One instance, owned by the world subsystem. Commands are distance/speed test inputs, not dynamics.
class Simulation
{
public:
    Result Initialize(Graph graph, Definitions definitions, Formation formation, DirectedEdge start,
                      double headMetres, Choices choices, TrainId train = TrainId(1));
    Result AdvanceDistance(double metres);
    Result Update(double renderDeltaSeconds, const std::function<void(Simulation&)>& beforeStep = {});
    // Same clock/overload policy; a train-level model supplies one transactional logical step.
    Result UpdateSteps(double renderDeltaSeconds, const std::function<Result(Simulation&,double)>& step);
    Result AdvanceLogicalStep(const std::function<Result(Simulation&)>& step);
    Result CommitHistoryExtent(double minimum,double maximum);
    Result SetCommandedSpeed(double metresPerSecond);
    Result SetChoice(DirectedEdge incoming, TransitionId choice);
    Result Presentation(double alpha, Snapshot& out) const;
    const Snapshot& Current() const { return current_; }
    const PathHistory& History() const { return history_; }
    const Graph& Network() const { return graph_; }
    bool Paused() const { return paused_; }
    const std::string& Error() const { return error_; }
    double InterpolationAlpha() const;
    double FixedStepSeconds = 1.0 / 30.0;
    unsigned MaxStepsPerUpdate = 4;
private:
    friend class WorldSnapshotCodec;
    Graph graph_; Definitions definitions_; Formation formation_; Choices choices_;
    PathHistory history_; Snapshot current_;
    double previousHead_ = 0, accumulator_ = 0;
    bool paused_ = true; std::string error_;
};
struct Fixture
{
    Graph graph; Definitions definitions;
    Formation Short(bool reversed = false) const;
    Formation Long(bool reversed = false) const;
    Choices SelectUpper(bool upper = true) const;
};
Fixture MakeFixture(Vec3 translation = {});
struct TestCase { std::string name; bool passed = false; double measuredError = 0, tolerance = 0; std::string detail; };
struct TestReport
{
    std::vector<TestCase> cases;
    bool Passed() const;
    std::string Json() const;
};
TestReport RunKernelTests();
}
