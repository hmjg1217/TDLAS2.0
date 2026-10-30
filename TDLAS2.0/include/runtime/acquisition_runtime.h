#pragma once
#include "core/sample_router.h"
#include "core/result_history.h"
#include "daq/sample_source.h"
#include "storage/data_storage.h"
#include <QObject>
#include <QThread>
#include <QTimer>
#include <atomic>
#include <functional>

namespace tdlas {
enum class RunState { Ready, Starting, Running, StopRequested, Draining, Stopped, Error };
struct ShutdownSnapshot {
    bool stopRequested = false;
    bool sourceClosed = false;
    bool inputClosed = false;
    bool processingDrained = false;
    bool stopTimedOut = false;
    std::uint64_t sourceClosedOrder = 0;
    std::uint64_t inputCloseOrder = 0;
    std::uint64_t processingDrainedOrder = 0;
};
class AcquisitionRuntime final : public QObject {
    Q_OBJECT
public:
    using Processor = std::function<void(const SampleBlock&)>;
    using ResultProcessor = std::function<std::vector<AcquisitionFrame>(const SampleBlock&)>;
    explicit AcquisitionRuntime(QObject* parent = nullptr);
    ~AcquisitionRuntime() override;
    bool start(const AcquisitionConfig&, std::unique_ptr<ISampleSource>, Processor,
               std::shared_ptr<AsyncRawArchive> archive = {});
    bool startWithResults(const AcquisitionConfig&, std::unique_ptr<ISampleSource>, ResultProcessor,
                          std::shared_ptr<AsyncRawArchive> archive = {});
    const ResultHistory& results() const { return results_; }
    void requestStop();
    bool reset();
    bool active() const { return producer_ || consumer_; }
    RunState state() const { return state_; }
    DisplaySnapshot snapshot() const;
    ShutdownSnapshot shutdownSnapshot() const noexcept;
    QString error() const { return error_; }
signals:
    void stateChanged(tdlas::RunState state);
    void faultOccurred(const QString& message);
    void stopped();
private:
    void setState(RunState);
    void reportFault(const QString&);
    void finishWorker(bool producer);
    void resetShutdownObservation() noexcept;
    std::uint64_t nextShutdownOrder() noexcept;
    QThread* producer_ = nullptr;
    QThread* consumer_ = nullptr;
    std::shared_ptr<SampleRouter> router_;
    std::atomic_bool stop_{false};
    std::atomic_bool stopRequested_{false}, sourceClosed_{false}, inputClosed_{false}, stopTimedOut_{false};
    std::atomic_uint64_t shutdownOrder_{0}, sourceClosedOrder_{0}, inputCloseOrder_{0}, processingDrainedOrder_{0};
    QTimer stopDeadline_;
    RunState state_ = RunState::Ready;
    QString error_;
    ResultHistory results_;
};
}
Q_DECLARE_METATYPE(tdlas::RunState)
