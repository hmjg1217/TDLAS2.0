#include "core/sample_router.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace tdlas {
SampleRouter::SampleRouter(std::size_t capacity, std::size_t display) : capacity_(capacity), displaySize_(display) {
    if (!capacity || !display) throw std::invalid_argument("Router capacity must be positive");
}
SubmitResult SampleRouter::submit(std::shared_ptr<const SampleBlock> block) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (closed_) { ++latest_.stats.rejectedBlocks; return SubmitResult::Closed; }
    if (!block || block->samples.empty() || !std::isfinite(block->sampleRateHz) || block->sampleRateHz <= 0) {
        ++latest_.stats.rejectedBlocks; return SubmitResult::Invalid;
    }
    if (block->samples.size() > capacity_ - latest_.stats.queuedSamples) {
        ++latest_.stats.rejectedBlocks; closed_ = true; ready_.notify_all(); return SubmitResult::Overflow;
    }
    queue_.push_back(block);
    ++latest_.stats.acceptedBlocks;
    latest_.stats.acceptedSamples += block->samples.size();
    ++latest_.stats.queuedBlocks;
    latest_.stats.queuedSamples += block->samples.size();
    latest_.stats.highWaterBlocks = std::max(latest_.stats.highWaterBlocks, latest_.stats.queuedBlocks);
    latest_.stats.highWaterSamples = std::max(latest_.stats.highWaterSamples, latest_.stats.queuedSamples);
    const auto start = block->samples.size() > displaySize_ ? block->samples.size() - displaySize_ : 0;
    display_.insert(display_.end(), block->samples.begin() + start, block->samples.end());
    while (display_.size() > displaySize_) display_.pop_front();
    latest_.runId = block->runId; latest_.lastSequence = block->sequence; latest_.sampleRateHz = block->sampleRateHz;
    latest_.acquiredAtUtcMs = block->acquiredAtUtcMs;
    ready_.notify_one();
    return SubmitResult::Accepted;
}
std::shared_ptr<const SampleBlock> SampleRouter::take() {
    std::unique_lock<std::mutex> lock(mutex_);
    ready_.wait(lock, [this] { return closed_ || !queue_.empty(); });
    if (queue_.empty()) return {};
    if (inFlight_) throw std::logic_error("Previous router block has no terminal outcome");
    auto block = queue_.front(); queue_.pop_front();
    --latest_.stats.queuedBlocks;
    latest_.stats.queuedSamples -= block->samples.size();
    latest_.stats.processingBlocks = 1;
    latest_.stats.processingSamples = block->samples.size();
    inFlight_ = block;
    return block;
}
void SampleRouter::close() { std::lock_guard<std::mutex> lock(mutex_); closed_ = true; ready_.notify_all(); }
void SampleRouter::complete(bool success) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!inFlight_) throw std::logic_error("Router completion has no in-flight block");
    const auto samples = inFlight_->samples.size();
    if (success) {
        ++latest_.stats.processedBlocks;
        latest_.stats.processedSamples += samples;
    } else {
        ++latest_.stats.failedBlocks;
        latest_.stats.failedSamples += samples;
    }
    latest_.stats.processingBlocks = 0;
    latest_.stats.processingSamples = 0;
    inFlight_.reset();
}
DisplaySnapshot SampleRouter::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto snapshot = latest_; snapshot.samples.assign(display_.begin(), display_.end()); return snapshot;
}
}
