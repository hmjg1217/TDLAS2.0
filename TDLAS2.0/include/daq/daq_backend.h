#pragma once

#include "daq/sample_source.h"
#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace tdlas {

struct DaqDeviceDescriptor {
    std::string id;
    std::string productType;
    DeviceCapabilities capabilities;
};

struct DaqDiscoveryResult {
    std::vector<DaqDeviceDescriptor> devices;
    std::string error;
};

// Backends are stateless factories. Discovery may block in a vendor API and
// must therefore be called from a worker thread. The returned sample source is
// subsequently owned and called only by AcquisitionRuntime's producer thread.
class IDaqBackend {
public:
    virtual ~IDaqBackend() = default;
    virtual std::string name() const = 0;
    virtual DaqDiscoveryResult discover() = 0;
    virtual std::unique_ptr<ISampleSource> makeSource(const DaqDeviceDescriptor&) = 0;
};

bool niDaqmxCompiled() noexcept;
std::shared_ptr<IDaqBackend> makeNiDaqBackend();

struct FakeDaqOptions {
    std::vector<DaqDeviceDescriptor> devices;
    bool failDiscovery = false;
    bool failOnOpen = false;
    std::optional<std::uint64_t> disconnectAfterReads;
    int discoveryDelayMs = 0;
    int readDelayMs = 0;
};

struct FakeDaqObservation {
    std::atomic_uint64_t discoveryCalls{0};
    std::atomic_uint64_t openCalls{0};
    std::atomic_uint64_t readCalls{0};
    std::atomic_uint64_t closeCalls{0};
    std::atomic_uint64_t aiStarts{0};
    std::atomic_uint64_t aoStarts{0};
    std::atomic_bool taskOpen{false};
};

DaqDeviceDescriptor defaultFakeDaqDevice();
std::shared_ptr<IDaqBackend> makeFakeDaqBackend(
    FakeDaqOptions options = {},
    std::shared_ptr<FakeDaqObservation> observation = {});

} // namespace tdlas
