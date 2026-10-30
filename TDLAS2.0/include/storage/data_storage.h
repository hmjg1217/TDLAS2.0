#pragma once

#include "core/contracts.h"
#include "core/result_history.h"
#include <QString>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

namespace tdlas {

enum class ArchiveSubmitResult { Accepted, NotStarted, Closed, Overflow, Failed };

// A producer-facing, bounded queue. File I/O is performed only by the owned
// worker thread. finish() may wait, so AcquisitionRuntime calls it from the
// data-source thread after the device has been released, never from the UI.
class AsyncRawArchive final {
public:
    explicit AsyncRawArchive(QString rootDirectory, std::size_t maximumQueuedSamples = 200000);
    ~AsyncRawArchive();
    AsyncRawArchive(const AsyncRawArchive&) = delete;
    AsyncRawArchive& operator=(const AsyncRawArchive&) = delete;

    QString begin(const AcquisitionConfig& config, const std::string& runId);
    ArchiveSubmitResult submit(std::shared_ptr<const SampleBlock> block);
    QString finish(bool acquisitionSucceeded);

    QString runDirectory() const;
    QString error() const;
    std::size_t queuedSamples() const;

private:
    void writeLoop(AcquisitionConfig config, QString runId, QString runDirectory);
    void setError(QString error);

    const QString rootDirectory_;
    const std::size_t maximumQueuedSamples_;
    mutable std::mutex mutex_;
    std::condition_variable ready_;
    std::deque<std::shared_ptr<const SampleBlock>> queue_;
    std::thread worker_;
    QString runDirectory_, error_;
    std::size_t queuedSamples_ = 0;
    bool started_ = false, closing_ = false, acquisitionSucceeded_ = true;
};

struct ExportResult {
    bool succeeded = false;
    QString directory;
    QString csvPath;
    QString jsonPath;
    QString error;
};

// Writes a consistent immutable snapshot using atomic replacement. The CSV is
// the peak table for common analysis tools; JSON retains complete frame/curve
// metadata and is the lossless application interchange format.
ExportResult exportResults(const ResultSnapshot& snapshot, const QString& directory);

} // namespace tdlas
