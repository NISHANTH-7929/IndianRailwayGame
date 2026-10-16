#pragma once
#include "RailKernel.h"
#include <set>

namespace Rail {
// Presentation-only contract. These identifiers cannot grant railway permission.
struct PresentationChunk {
    unsigned id;
    const char* level;
    const char* rebindKey;
    bool essential;
};
struct ContentManifest {
    static constexpr unsigned Schema = 1;
    static constexpr const char* ContentId = "IR-S6-fixture-1";
    static constexpr double BoundaryX = 0;
    static constexpr double RearMarginMetres = 30;
    static constexpr double LookBackMarginMetres = 60;
    static constexpr double AheadSeconds = 8;
    static constexpr double AheadMinimumMetres = 40;
    static const std::vector<PresentationChunk>& Chunks();
};
struct VisualDemand {
    std::set<unsigned> chunks;
    bool valid = false;
    std::string diagnostic;
};
// Uses complete physical footprints, not Actor locations. Caller supplies the
// route's sampled near-term points; both plausible junction branches may be supplied.
VisualDemand RequiredPresentation(const Snapshot& train, double speedMps,
    bool lookBack, const std::vector<Vec3>& nearTermRoute);
bool EssentialPresentationReady(const VisualDemand& demand, const std::set<unsigned>& loaded);
TestReport RunContentTests();
}
