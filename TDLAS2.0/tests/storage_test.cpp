#include "storage/data_storage.h"
#include "result_fixture.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>

using namespace tdlas;
class StorageTest final : public QObject {
    Q_OBJECT
private slots:
    void boundedRawArchiveRoundTrip() {
        QTemporaryDir root; QVERIFY(root.isValid());
        AcquisitionConfig config; config.deviceId = "fixture"; config.inputChannel = "fixture/ai";
        auto block = std::make_shared<SampleBlock>(); block->runId = "run-a"; block->sequence = 7;
        block->acquiredAtUtcMs = 1793088000123; block->elapsedNs = 42; block->deviceId = config.deviceId;
        block->channelId = config.inputChannel; block->configRevision = config.revision; block->sampleRateHz = config.sampleRateHz;
        block->samples = {-1.25, 0.0, 2.5};
        AsyncRawArchive archive(root.path(), 10);
        QVERIFY(archive.begin(config, block->runId).isEmpty());
        QCOMPARE(archive.submit(block), ArchiveSubmitResult::Accepted);
        QVERIFY(archive.finish(true).isEmpty());
        QFile raw(archive.runDirectory() + "/raw.csv"); QVERIFY(raw.open(QIODevice::ReadOnly));
        const auto content = raw.readAll(); QCOMPARE(content.count('\n'), 4); QVERIFY(content.contains("-1.25")); QVERIFY(content.contains("2.5"));
        QFile manifest(archive.runDirectory() + "/manifest.json"); QVERIFY(manifest.open(QIODevice::ReadOnly));
        const auto object = QJsonDocument::fromJson(manifest.readAll()).object(); QCOMPARE(object["state"].toString(), QString("complete"));
        QCOMPARE(object["blocks"].toInteger(), qint64(1)); QCOMPARE(object["samples"].toInteger(), qint64(3));
    }
    void overflowAndWriteFailureAreExplicit() {
        QTemporaryDir root; QVERIFY(root.isValid());
        AcquisitionConfig config; config.deviceId = "fixture"; config.inputChannel = "fixture/ai";
        AsyncRawArchive bounded(root.path(), 2); QVERIFY(bounded.begin(config, "too-large").isEmpty());
        auto block = std::make_shared<SampleBlock>(); block->samples = {1, 2, 3};
        QCOMPARE(bounded.submit(block), ArchiveSubmitResult::Overflow); QVERIFY(bounded.finish(false).isEmpty());
        QFile obstacle(root.path() + "/not-a-directory"); QVERIFY(obstacle.open(QIODevice::WriteOnly)); obstacle.write("x"); obstacle.close();
        AsyncRawArchive failed(obstacle.fileName(), 10); QVERIFY(!failed.begin(config, "run").isEmpty());
    }
    void csvAndJsonReadBackMatchSnapshot() {
        QTemporaryDir root; QVERIFY(root.isValid());
        ResultHistory history; history.beginRun("test-run");
        history.submit(testFrame(2)); history.submit(testFrame(0));
        auto gap = testFrame(1); gap.fv.x.clear(); gap.fv.y.clear(); history.submit(gap);
        const auto snapshot = history.snapshot(); const auto result = exportResults(snapshot, root.path() + "/export");
        QVERIFY2(result.succeeded, qPrintable(result.error));
        QFile csv(result.csvPath); QVERIFY(csv.open(QIODevice::ReadOnly)); QCOMPARE(csv.readAll().count('\n'), 4);
        QFile json(result.jsonPath); QVERIFY(json.open(QIODevice::ReadOnly));
        const auto object = QJsonDocument::fromJson(json.readAll()).object(); QCOMPARE(object["schemaVersion"].toInt(), 1);
        const auto entries = object["entries"].toArray(); QCOMPARE(entries.size(), 3);
        QCOMPARE(entries[0].toObject()["sequence"].toInteger(), qint64(0));
        QVERIFY(entries[1].toObject()["curves"].toObject()["fv"].toObject()["y"].toArray().isEmpty());
        QCOMPARE(entries[2].toObject()["sequence"].toInteger(), qint64(2));
    }
};
QTEST_GUILESS_MAIN(StorageTest)
#include "storage_test.moc"
