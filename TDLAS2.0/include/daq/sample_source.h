#pragma once
#include "core/contracts.h"
#include <atomic>
#include <memory>

namespace tdlas {
// All methods run on the producer thread. read() must return within a bounded
// interval and observe cancellation. Future NI task handles stay in this owner.
class ISampleSource {
public:
    virtual ~ISampleSource() = default;
    virtual void open(const AcquisitionConfig&, const std::string& runId) = 0;
    virtual std::shared_ptr<const SampleBlock> read(const std::atomic_bool& stop) = 0;
    virtual void close() noexcept = 0;
};
// Explicit, hardware-free diagnostics. Defaults always select normal operation.
struct SimulationOptions {
    bool failOnOpen = false;
    std::optional<std::uint64_t> failAfterBlocks;
    int readDelayMs = 0; // Cancellable delay, 0..10000 ms, per read.
};
DeviceCapabilities simulationCapabilities();
std::unique_ptr<ISampleSource> makeSimulationSource(SimulationOptions options = {});
}
