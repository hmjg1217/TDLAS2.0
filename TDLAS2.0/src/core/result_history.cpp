#include "core/result_history.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace tdlas {
ResultHistory::ResultHistory(std::size_t frames, std::size_t points) : maximumFrames_(frames), maximumPoints_(points) {
    if (!frames || !points) throw std::invalid_argument("Result history limits must be positive");
}
void ResultHistory::beginRun(const std::string& id) {
    if (id.empty()) throw std::invalid_argument("Empty result runId");
    std::lock_guard<std::mutex> lock(mutex_); activeRunId_ = id; ++revision_;
}
ResultSubmission ResultHistory::submit(AcquisitionFrame frame) {
    if (auto error = validateIdentity(frame); !error.empty()) return {ResultDisposition::Invalid, error};
    frame = normalizeFrame(std::move(frame));
    std::lock_guard<std::mutex> lock(mutex_);
    auto run = std::find_if(runs_.begin(), runs_.end(), [&](const Run& r) { return r.id == frame.runId; });
    if (run != runs_.end()) {
        if (run->ids.count(frame.acquisitionId)) return {ResultDisposition::Duplicate, "重复批次；保留第一次接收的完整结果"};
        if (run->entries.count(frame.sequence)) return {ResultDisposition::Conflict, "同一会话批次序号冲突"};
    }
    if (frameCount_ >= maximumFrames_) return {ResultDisposition::CapacityExceeded, "完整结果缓存已满，请停止并保留当前数据"};
    std::size_t points = 0;
    for (const auto* curve : {&frame.raw, &frame.observed, &frame.fitted, &frame.fv}) {
        const auto n = std::max(curve->x.size(), curve->y.size());
        if (n > maximumPoints_ - pointCount_ - points) return {ResultDisposition::CapacityExceeded, "完整曲线缓存已满，请停止并保留当前数据"};
        points += n;
    }
    if (run == runs_.end()) { runs_.push_back(Run{frame.runId, {}, {}}); run = std::prev(runs_.end()); }
    auto immutable = std::make_shared<const AcquisitionFrame>(std::move(frame));
    run->entries.emplace(immutable->sequence, ResultEntry{immutable, extractPeak(*immutable)});
    run->ids.insert(immutable->acquisitionId);
    ++frameCount_; pointCount_ += points; ++revision_;
    return {ResultDisposition::Accepted, {}};
}
ResultSnapshot ResultHistory::snapshotLocked() const {
    ResultSnapshot s; s.revision = revision_; s.activeRunId = activeRunId_; s.entries.reserve(frameCount_);
    for (const auto& run : runs_) {
        for (const auto& entry : run.entries) s.entries.push_back(entry.second);
        if (run.id == activeRunId_ && !run.entries.empty()) s.latest = run.entries.rbegin()->second.frame;
    }
    return s;
}
ResultSnapshot ResultHistory::snapshot() const { std::lock_guard<std::mutex> lock(mutex_); return snapshotLocked(); }
std::optional<ResultSnapshot> ResultHistory::snapshotAfter(std::uint64_t revision) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (revision == revision_) return {};
    return snapshotLocked();
}
void ResultHistory::clear() {
    std::lock_guard<std::mutex> lock(mutex_); runs_.clear(); activeRunId_.clear(); frameCount_ = pointCount_ = 0; ++revision_;
}
std::vector<PeakSegment> peakSegments(const ResultSnapshot& s) {
    std::vector<PeakSegment> segments;
    const PeakRecord* previous = nullptr;
    for (const auto& entry : s.entries) {
        const auto& p = entry.peak;
        if (!p.peakValue || !std::isfinite(p.elapsedSeconds)) { previous = nullptr; continue; }
        const bool continuous = previous && previous->runId == p.runId && previous->fvUnit == p.fvUnit &&
            previous->sequence != std::numeric_limits<std::uint64_t>::max() && p.sequence == previous->sequence + 1 && p.elapsedSeconds >= previous->elapsedSeconds;
        if (!continuous) segments.push_back(PeakSegment{p.runId, p.fvUnit, {}, {}});
        segments.back().elapsedSeconds.push_back(p.elapsedSeconds); segments.back().values.push_back(*p.peakValue);
        previous = &p;
    }
    return segments;
}
}
