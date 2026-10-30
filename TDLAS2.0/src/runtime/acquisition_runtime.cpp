#include "runtime/acquisition_runtime.h"
#include <QUuid>
#include <exception>
#include <stdexcept>

namespace tdlas {
AcquisitionRuntime::AcquisitionRuntime(QObject* parent) : QObject(parent) {
    stopDeadline_.setSingleShot(true); stopDeadline_.setInterval(5000);
    connect(&stopDeadline_, &QTimer::timeout, this, [this] {
        stopTimedOut_ = true;
        const QString timeout = QStringLiteral("停止超过 5 秒，后台任务尚未退出；保留窗口并等待安全清理");
        if (error_.isEmpty()) error_ = timeout;
        else if (!error_.contains(timeout)) error_ += QStringLiteral("\n") + timeout;
        setState(RunState::Error); emit faultOccurred(error_);
    });
}
AcquisitionRuntime::~AcquisitionRuntime() {
    // UI closeEvent defers destruction until stopped(). This final fallback is
    // for non-UI owners only; no forced termination of a hardware-owning thread.
    stop_ = true;
    if (producer_) { producer_->wait(); delete producer_; }
    if (router_) router_->close();
    if (consumer_) { consumer_->wait(); delete consumer_; }
}
void AcquisitionRuntime::setState(RunState s) { if (state_ == s) return; state_ = s; emit stateChanged(s); }
void AcquisitionRuntime::resetShutdownObservation() noexcept {
    stopRequested_ = false; sourceClosed_ = false; inputClosed_ = false; stopTimedOut_ = false;
    shutdownOrder_ = 0; sourceClosedOrder_ = 0; inputCloseOrder_ = 0; processingDrainedOrder_ = 0;
}
std::uint64_t AcquisitionRuntime::nextShutdownOrder() noexcept { return shutdownOrder_.fetch_add(1) + 1; }
void AcquisitionRuntime::reportFault(const QString& message) {
    if (error_.isEmpty()) error_ = message;
    setState(RunState::Error); emit faultOccurred(error_); requestStop();
}
bool AcquisitionRuntime::start(const AcquisitionConfig& c, std::unique_ptr<ISampleSource> source, Processor process,
                               std::shared_ptr<AsyncRawArchive> archive) {
    if (active()) return false;
    const auto invalid = validate(c);
    if (!invalid.empty() || !source || !process) {
        error_ = invalid.empty() ? QStringLiteral("缺少数据源或处理器") : QString::fromStdString(invalid);
        setState(RunState::Error); emit faultOccurred(error_); return false;
    }
    error_.clear(); stop_ = false;
    resetShutdownObservation();
    router_ = std::make_shared<SampleRouter>(c.queueCapacitySamples, c.displayWindowSamples);
    const auto router = router_;
    const auto runId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    results_.beginRun(runId);
    consumer_ = QThread::create([this, router, process = std::move(process)] {
        bool failed = false;
        while (auto block = router->take()) {
            if (failed) { router->complete(false); continue; }
            try { process(*block); router->complete(true); }
            catch (const std::exception& e) {
                failed = true; router->complete(false); stop_ = true;
                const QString reason = QStringLiteral("处理失败：") + QString::fromUtf8(e.what());
                QMetaObject::invokeMethod(this, [this, reason] { reportFault(reason); }, Qt::QueuedConnection);
            } catch (...) {
                failed = true; router->complete(false); stop_ = true;
                QMetaObject::invokeMethod(this, [this] { reportFault(QStringLiteral("处理器发生未知异常")); }, Qt::QueuedConnection);
            }
        }
        processingDrainedOrder_ = nextShutdownOrder();
    });
    producer_ = QThread::create([this, router, c, runId, source = std::move(source), archive = std::move(archive)] {
        bool sourceSucceeded = true;
        try {
            if (archive) {
                const auto archiveError = archive->begin(c, runId);
                if (!archiveError.isEmpty()) throw std::runtime_error(archiveError.toStdString());
            }
            if (!stop_) source->open(c, runId);
            QMetaObject::invokeMethod(this, [this] {
                if (!stop_ && state_ == RunState::Starting) setState(RunState::Running);
            }, Qt::QueuedConnection);
            while (!stop_) {
                auto block = source->read(stop_);
                if (!block) break;
                const auto archivedBlock = block;
                const auto result = router->submit(std::move(block));
                if (result != SubmitResult::Accepted) {
                    throw std::runtime_error(result == SubmitResult::Overflow ? "算法输入队列已满，已停止接收，正在排空" : "输入块无效或队列已关闭");
                }
                if (archive) {
                    const auto archived = archive->submit(archivedBlock);
                    if (archived != ArchiveSubmitResult::Accepted)
                        throw std::runtime_error(archived == ArchiveSubmitResult::Overflow ? "原始数据写入队列已满，已停止采集" : "原始数据归档失败");
                }
            }
        } catch (const std::exception& e) {
            sourceSucceeded = false;
            const QString reason = QStringLiteral("数据源失败：") + QString::fromUtf8(e.what());
            QMetaObject::invokeMethod(this, [this, reason] { reportFault(reason); }, Qt::QueuedConnection);
        } catch (...) {
            sourceSucceeded = false;
            QMetaObject::invokeMethod(this, [this] { reportFault(QStringLiteral("数据源发生未知异常")); }, Qt::QueuedConnection);
        }
        source->close(); // Source/task cleanup precedes input closure and drain.
        sourceClosed_ = true;
        sourceClosedOrder_ = nextShutdownOrder();
        if (archive) {
            const auto archiveError = archive->finish(sourceSucceeded);
            if (!archiveError.isEmpty()) {
                const QString reason = QStringLiteral("原始数据归档失败：") + archiveError;
                QMetaObject::invokeMethod(this, [this, reason] { reportFault(reason); }, Qt::QueuedConnection);
            }
        }
        inputCloseOrder_ = nextShutdownOrder();
        router->close();
        inputClosed_ = true;
    });
    producer_->setObjectName("TDLAS.SampleSource"); consumer_->setObjectName("TDLAS.Processing");
    connect(producer_, &QThread::finished, this, [this] { finishWorker(true); });
    connect(consumer_, &QThread::finished, this, [this] { finishWorker(false); });
    setState(RunState::Starting);
    consumer_->start(); producer_->start(); return true;
}
void AcquisitionRuntime::requestStop() {
    if (!active() || stopDeadline_.isActive() || stopTimedOut_) return;
    stopRequested_ = true;
    stop_ = true;
    if (state_ != RunState::Error && state_ != RunState::Draining) setState(RunState::StopRequested);
    stopDeadline_.start();
}
void AcquisitionRuntime::finishWorker(bool producer) {
    auto*& worker = producer ? producer_ : consumer_;
    worker->deleteLater(); worker = nullptr;
    if (producer && consumer_ && state_ != RunState::Error) setState(RunState::Draining);
    if (!active()) {
        stopDeadline_.stop();
        const auto stats = router_ ? router_->snapshot().stats : RouterStats{};
        if (!stats.terminalComplete() && error_.isEmpty()) {
            error_ = QStringLiteral("输入块终态核对失败：仍有排队或处理中的块");
            emit faultOccurred(error_);
        }
        setState(error_.isEmpty() ? RunState::Stopped : RunState::Error); emit stopped();
    }
}
bool AcquisitionRuntime::reset() {
    if (active()) return false;
    router_.reset(); results_.clear(); error_.clear(); resetShutdownObservation(); setState(RunState::Ready); return true;
}
bool AcquisitionRuntime::startWithResults(const AcquisitionConfig& config, std::unique_ptr<ISampleSource> source, ResultProcessor process,
                                          std::shared_ptr<AsyncRawArchive> archive) {
    if (!process) return start(config, std::move(source), {}, std::move(archive));
    return start(config, std::move(source), [this, process = std::move(process)](const SampleBlock& block) {
        for (auto& frame : process(block)) {
            if (frame.runId != block.runId || frame.deviceId != block.deviceId || frame.channelId != block.channelId || frame.configRevision != block.configRevision)
                throw std::runtime_error("结果身份与输入会话/设备/通道/配置不一致");
            auto result = results_.submit(std::move(frame));
            if (result.disposition != ResultDisposition::Accepted && result.disposition != ResultDisposition::Duplicate) throw std::runtime_error(result.reason);
        }
    }, std::move(archive));
}
DisplaySnapshot AcquisitionRuntime::snapshot() const { return router_ ? router_->snapshot() : DisplaySnapshot{}; }
ShutdownSnapshot AcquisitionRuntime::shutdownSnapshot() const noexcept {
    return {stopRequested_.load(), sourceClosed_.load(), inputClosed_.load(), processingDrainedOrder_.load() != 0,
        stopTimedOut_.load(), sourceClosedOrder_.load(), inputCloseOrder_.load(), processingDrainedOrder_.load()};
}
}
