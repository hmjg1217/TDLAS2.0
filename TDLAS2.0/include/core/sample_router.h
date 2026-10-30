#pragma once
#include "core/contracts.h"
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>

namespace tdlas {
struct RouterStats {
    std::uint64_t acceptedBlocks = 0, processedBlocks = 0, rejectedBlocks = 0, failedBlocks = 0;
    std::uint64_t acceptedSamples = 0, processedSamples = 0, failedSamples = 0;
    std::size_t queuedBlocks = 0, queuedSamples = 0;
    std::size_t processingBlocks = 0, processingSamples = 0;
    std::size_t highWaterBlocks = 0, highWaterSamples = 0;
    std::uint64_t terminalBlocks() const noexcept { return processedBlocks + failedBlocks; }
    std::uint64_t terminalSamples() const noexcept { return processedSamples + failedSamples; }
    bool accountingConsistent() const noexcept {
        return acceptedBlocks == terminalBlocks() + queuedBlocks + processingBlocks &&
            acceptedSamples == terminalSamples() + queuedSamples + processingSamples;
    }
    bool terminalComplete() const noexcept {
        return accountingConsistent() && queuedBlocks == 0 && processingBlocks == 0;
    }
};
struct DisplaySnapshot {
    std::string runId;
    std::vector<double> samples;
    std::uint64_t lastSequence = 0;
    double sampleRateHz = 0;
    std::int64_t acquiredAtUtcMs = 0;
    RouterStats stats;
};
enum class SubmitResult { Accepted, Closed, Overflow, Invalid };
// No QObject, signals or UI dependency. Producer calls submit directly.
class SampleRouter {
public:
    SampleRouter(std::size_t capacitySamples, std::size_t displaySamples);
    SubmitResult submit(std::shared_ptr<const SampleBlock> block);
    std::shared_ptr<const SampleBlock> take(); // Drains accepted blocks after close().
    void close();
    void complete(bool success);
    DisplaySnapshot snapshot() const;
private:
    const std::size_t capacity_, displaySize_;
    mutable std::mutex mutex_;
    std::condition_variable ready_;
    bool closed_ = false;
    std::deque<std::shared_ptr<const SampleBlock>> queue_;
    std::shared_ptr<const SampleBlock> inFlight_;
    std::deque<double> display_;
    DisplaySnapshot latest_;
};
}
