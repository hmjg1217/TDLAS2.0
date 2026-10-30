#include "storage/data_storage.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTextStream>
#include <cmath>

namespace tdlas {
namespace {
QString csvField(QString value) {
    value.replace('"', "\"\"");
    return '"' + value + '"';
}
QString qualityName(Quality quality) {
    switch (quality) {
    case Quality::Valid: return QStringLiteral("valid");
    case Quality::Invalid: return QStringLiteral("invalid");
    case Quality::Uncalibrated: return QStringLiteral("uncalibrated");
    }
    return QStringLiteral("invalid");
}
QJsonValue finiteValue(double value) { return std::isfinite(value) ? QJsonValue(value) : QJsonValue(QJsonValue::Null); }
QJsonValue optionalValue(const std::optional<double>& value) { return value ? finiteValue(*value) : QJsonValue(QJsonValue::Null); }
QJsonArray values(const std::vector<double>& source) {
    QJsonArray result;
    for (const auto value : source) result.append(finiteValue(value));
    return result;
}
QJsonObject curveObject(const Curve& curve) {
    QJsonArray valid;
    for (const auto value : curve.pointValid) valid.append(value);
    return {{"x", values(curve.x)}, {"y", values(curve.y)},
        {"xQuantity", QString::fromStdString(curve.xQuantity)}, {"xUnit", QString::fromStdString(curve.xUnit)},
        {"yQuantity", QString::fromStdString(curve.yQuantity)}, {"yUnit", QString::fromStdString(curve.yUnit)},
        {"pointValid", valid}};
}
bool commitJson(const QString& path, const QJsonObject& object, QString& error) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) { error = file.errorString(); return false; }
    if (file.write(QJsonDocument(object).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        error = file.errorString(); return false;
    }
    return true;
}
QJsonObject configObject(const AcquisitionConfig& c) {
    return {{"deviceId", QString::fromStdString(c.deviceId)}, {"inputChannel", QString::fromStdString(c.inputChannel)},
        {"outputChannel", QString::fromStdString(c.outputChannel)}, {"sampleRateHz", c.sampleRateHz},
        {"readBlockSamples", static_cast<qint64>(c.readBlockSamples)}, {"displayWindowSamples", static_cast<qint64>(c.displayWindowSamples)},
        {"processingWindowSamples", static_cast<qint64>(c.processingWindowSamples)}, {"inputMinimumV", c.inputMinimumV},
        {"inputMaximumV", c.inputMaximumV}, {"configRevision", static_cast<qint64>(c.revision)}};
}
}

AsyncRawArchive::AsyncRawArchive(QString root, std::size_t capacity)
    : rootDirectory_(std::move(root)), maximumQueuedSamples_(capacity) {
    if (!maximumQueuedSamples_) throw std::invalid_argument("Raw archive queue capacity must be positive");
}
AsyncRawArchive::~AsyncRawArchive() { finish(false); }
QString AsyncRawArchive::begin(const AcquisitionConfig& config, const std::string& runId) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (started_) return QStringLiteral("原始数据归档已启动，不能重复使用");
    if (runId.empty()) return QStringLiteral("原始数据归档缺少会话标识");
    const QString root = QDir::cleanPath(rootDirectory_);
    if (!QDir().mkpath(root)) return QStringLiteral("无法创建原始数据目录：%1").arg(root);
    runDirectory_ = QDir(root).filePath(QString::fromStdString(runId));
    if (!QDir().mkpath(runDirectory_)) return QStringLiteral("无法创建会话归档目录：%1").arg(runDirectory_);
    started_ = true;
    worker_ = std::thread(&AsyncRawArchive::writeLoop, this, config, QString::fromStdString(runId), runDirectory_);
    return {};
}
ArchiveSubmitResult AsyncRawArchive::submit(std::shared_ptr<const SampleBlock> block) {
    if (!block || block->samples.empty()) return ArchiveSubmitResult::Failed;
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_) return ArchiveSubmitResult::NotStarted;
    if (!error_.isEmpty()) return ArchiveSubmitResult::Failed;
    if (closing_) return ArchiveSubmitResult::Closed;
    if (block->samples.size() > maximumQueuedSamples_ - std::min(maximumQueuedSamples_, queuedSamples_)) return ArchiveSubmitResult::Overflow;
    queuedSamples_ += block->samples.size(); queue_.push_back(std::move(block)); ready_.notify_one();
    return ArchiveSubmitResult::Accepted;
}
QString AsyncRawArchive::finish(bool succeeded) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!started_) return error_;
        closing_ = true; acquisitionSucceeded_ = acquisitionSucceeded_ && succeeded; ready_.notify_all();
    }
    if (worker_.joinable() && worker_.get_id() != std::this_thread::get_id()) worker_.join();
    std::lock_guard<std::mutex> lock(mutex_);
    return error_;
}
QString AsyncRawArchive::runDirectory() const { std::lock_guard<std::mutex> lock(mutex_); return runDirectory_; }
QString AsyncRawArchive::error() const { std::lock_guard<std::mutex> lock(mutex_); return error_; }
std::size_t AsyncRawArchive::queuedSamples() const { std::lock_guard<std::mutex> lock(mutex_); return queuedSamples_; }
void AsyncRawArchive::setError(QString error) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (error_.isEmpty()) error_ = std::move(error);
}
void AsyncRawArchive::writeLoop(AcquisitionConfig config, QString runId, QString directory) {
    const QString partialPath = QDir(directory).filePath(QStringLiteral("raw.csv.partial"));
    const QString finalPath = QDir(directory).filePath(QStringLiteral("raw.csv"));
    QFile file(partialPath);
    quint64 blocks = 0, samples = 0;
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) setError(QStringLiteral("无法写入原始数据：%1").arg(file.errorString()));
    else file.write("run_id,block_sequence,acquired_utc_ms,elapsed_ns,sample_index,sample_rate_hz,device_id,channel_id,config_revision,voltage_v\n");
    for (;;) {
        std::shared_ptr<const SampleBlock> block;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            ready_.wait(lock, [&] { return closing_ || !queue_.empty() || !error_.isEmpty(); });
            if (!queue_.empty()) { block = std::move(queue_.front()); queue_.pop_front(); }
            else if (closing_ || !error_.isEmpty()) break;
        }
        if (!block) continue;
        if (file.isOpen()) {
            QByteArray rows;
            for (std::size_t i = 0; i < block->samples.size(); ++i) {
                rows += csvField(QString::fromStdString(block->runId)).toUtf8() + ',' + QByteArray::number(block->sequence) + ',' +
                    QByteArray::number(block->acquiredAtUtcMs) + ',' + QByteArray::number(block->elapsedNs) + ',' + QByteArray::number(i) + ',' +
                    QByteArray::number(block->sampleRateHz, 'g', 17) + ',' + csvField(QString::fromStdString(block->deviceId)).toUtf8() + ',' +
                    csvField(QString::fromStdString(block->channelId)).toUtf8() + ',' + QByteArray::number(block->configRevision) + ',' +
                    QByteArray::number(block->samples[i], 'g', 17) + '\n';
            }
            if (file.write(rows) != rows.size()) setError(QStringLiteral("写入原始数据失败：%1").arg(file.errorString()));
            else { ++blocks; samples += block->samples.size(); }
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queuedSamples_ -= block->samples.size();
            if (!error_.isEmpty()) { queue_.clear(); queuedSamples_ = 0; }
        }
    }
    if (file.isOpen()) { if (!file.flush()) setError(QStringLiteral("刷新原始数据失败：%1").arg(file.errorString())); file.close(); }
    QString errorCopy; bool acquired = false;
    { std::lock_guard<std::mutex> lock(mutex_); errorCopy = error_; acquired = acquisitionSucceeded_; }
    bool complete = errorCopy.isEmpty() && acquired;
    if (complete) {
        QFile::remove(finalPath);
        if (!QFile::rename(partialPath, finalPath)) { setError(QStringLiteral("无法完成原始数据文件的原子命名")); complete = false; }
    }
    { std::lock_guard<std::mutex> lock(mutex_); errorCopy = error_; }
    QJsonObject manifest{{"schemaVersion", 1}, {"runId", runId}, {"state", complete ? "complete" : "incomplete"},
        {"createdAtUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)}, {"blocks", static_cast<qint64>(blocks)},
        {"samples", static_cast<qint64>(samples)}, {"rawFile", complete ? "raw.csv" : "raw.csv.partial"},
        {"error", errorCopy}, {"config", configObject(config)}};
    QString manifestError;
    if (!commitJson(QDir(directory).filePath(QStringLiteral("manifest.json")), manifest, manifestError))
        setError(QStringLiteral("无法提交归档清单：%1").arg(manifestError));
}

ExportResult exportResults(const ResultSnapshot& snapshot, const QString& directory) {
    ExportResult result; result.directory = QDir::cleanPath(directory);
    if (!QDir().mkpath(result.directory)) { result.error = QStringLiteral("无法创建导出目录：%1").arg(result.directory); return result; }
    result.csvPath = QDir(result.directory).filePath(QStringLiteral("peak-history.csv"));
    result.jsonPath = QDir(result.directory).filePath(QStringLiteral("results.json"));
    QSaveFile csv(result.csvPath);
    if (!csv.open(QIODevice::WriteOnly | QIODevice::Text)) { result.error = csv.errorString(); return result; }
    csv.write("run_id,acquisition_id,sequence,acquired_utc_ms,elapsed_seconds,peak_value,peak_x,fv_unit,quality,quality_reason\n");
    QJsonArray entries;
    for (const auto& entry : snapshot.entries) {
        const auto& p = entry.peak; const auto& f = *entry.frame;
        const auto numeric = [](const std::optional<double>& value) { return value ? QByteArray::number(*value, 'g', 17) : QByteArray{}; };
        csv.write(csvField(QString::fromStdString(p.runId)).toUtf8() + ',' + csvField(QString::fromStdString(p.acquisitionId)).toUtf8() + ',' +
            QByteArray::number(p.sequence) + ',' + QByteArray::number(p.acquiredAtUtcMs) + ',' + QByteArray::number(p.elapsedSeconds, 'g', 17) + ',' +
            numeric(p.peakValue) + ',' + numeric(p.peakX) + ',' + csvField(QString::fromStdString(p.fvUnit)).toUtf8() + ',' +
            csvField(qualityName(p.quality)).toUtf8() + ',' + csvField(QString::fromStdString(p.qualityReason)).toUtf8() + '\n');
        QJsonArray diagnostics;
        for (const auto& d : f.processingDiagnostics) diagnostics.append(QJsonObject{{"stage", QString::fromStdString(d.stage)},
            {"succeeded", d.succeeded}, {"message", QString::fromStdString(d.message)}, {"durationMs", finiteValue(d.durationMs)}});
        entries.append(QJsonObject{{"runId", QString::fromStdString(f.runId)}, {"acquisitionId", QString::fromStdString(f.acquisitionId)},
            {"sequence", static_cast<qint64>(f.sequence)}, {"deviceId", QString::fromStdString(f.deviceId)}, {"channelId", QString::fromStdString(f.channelId)},
            {"configRevision", static_cast<qint64>(f.configRevision)}, {"acquiredAtUtcMs", f.acquiredAtUtcMs}, {"elapsedSeconds", finiteValue(f.elapsedSeconds)},
            {"batch", QJsonObject{{"firstSampleIndex", static_cast<qint64>(f.batch.firstSampleIndex)}, {"sampleCount", static_cast<qint64>(f.batch.sampleCount)},
                {"firstBlockSequence", static_cast<qint64>(f.batch.firstBlockSequence)}, {"lastBlockSequence", static_cast<qint64>(f.batch.lastBlockSequence)}}},
            {"quality", qualityName(f.quality)}, {"qualityReason", QString::fromStdString(f.qualityReason)},
            {"processingVersion", QString::fromStdString(f.processingVersion)}, {"baselineMethod", QString::fromStdString(f.baselineMethod)},
            {"polarityTransform", QString::fromStdString(f.polarityTransform)}, {"modelId", QString::fromStdString(f.modelId)},
            {"fitStatus", QString::fromStdString(f.fitStatus)}, {"diagnostics", diagnostics},
            {"metrics", QJsonObject{{"peakAbsorbance", optionalValue(f.metrics.peakAbsorbance)}, {"intensityPercent", optionalValue(f.metrics.intensityPercent)},
                {"concentration", optionalValue(f.metrics.concentration)}, {"concentrationUnit", QString::fromStdString(f.metrics.concentrationUnit)},
                {"calibrationId", QString::fromStdString(f.metrics.calibrationId)}, {"gasSpecies", QString::fromStdString(f.metrics.gasSpecies)},
                {"quality", qualityName(f.metrics.quality)}}},
            {"curves", QJsonObject{{"raw", curveObject(f.raw)}, {"observed", curveObject(f.observed)}, {"fitted", curveObject(f.fitted)}, {"fv", curveObject(f.fv)}}}});
    }
    if (!csv.commit()) { result.error = csv.errorString(); return result; }
    QString jsonError;
    const QJsonObject root{{"schemaVersion", 1}, {"exportedAtUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)},
        {"historyRevision", static_cast<qint64>(snapshot.revision)}, {"activeRunId", QString::fromStdString(snapshot.activeRunId)}, {"entries", entries}};
    if (!commitJson(result.jsonPath, root, jsonError)) { result.error = jsonError; return result; }
    result.succeeded = true; return result;
}
} // namespace tdlas
