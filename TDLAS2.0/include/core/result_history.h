#pragma once
#include "core/contracts.h"
#include <map>
#include <memory>
#include <mutex>
#include <unordered_set>

namespace tdlas {
enum class ResultDisposition { Accepted, Duplicate, Conflict, Invalid, CapacityExceeded };
struct ResultSubmission {
    ResultDisposition disposition;
    std::string reason;
};
struct ResultEntry {
    std::shared_ptr<const AcquisitionFrame> frame;
    PeakRecord peak;
};
struct ResultSnapshot {
    std::uint64_t revision = 0;
    std::string activeRunId;
    std::vector<ResultEntry> entries; // Session insertion order, then sequence.
    std::shared_ptr<const AcquisitionFrame> latest; // Highest sequence in active run.
};
struct PeakSegment {
    std::string runId, unit;
    std::vector<double> elapsedSeconds, values;
};
std::vector<PeakSegment> peakSegments(const ResultSnapshot&);

// Processing thread submits complete frames directly. UI polls snapshots only.
// Limits retain full accepted curves and peaks; never silently evict data.
class ResultHistory {
public:
    explicit ResultHistory(std::size_t maximumFrames = 2000, std::size_t maximumCurvePoints = 2000000);
    void beginRun(const std::string& runId);
    ResultSubmission submit(AcquisitionFrame frame);
    ResultSnapshot snapshot() const;
    std::optional<ResultSnapshot> snapshotAfter(std::uint64_t revision) const;
    void clear();
private:
    struct Run {
        std::string id;
        std::map<std::uint64_t, ResultEntry> entries;
        std::unordered_set<std::string> ids;
    };
    ResultSnapshot snapshotLocked() const;
    const std::size_t maximumFrames_, maximumPoints_;
    mutable std::mutex mutex_;
    std::vector<Run> runs_;
    std::string activeRunId_;
    std::size_t frameCount_ = 0, pointCount_ = 0;
    std::uint64_t revision_ = 1;
};
}
