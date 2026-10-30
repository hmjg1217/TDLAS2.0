#include "core/result_history.h"
#include "result_fixture.h"
#include <QtTest>
#include <thread>
using namespace tdlas;
class ResultTest : public QObject {
    Q_OBJECT
private slots:
    void orderingDeduplicationAndClockRollback() {
        ResultHistory history; history.beginRun("test-run");
        auto late = testFrame(2); late.acquiredAtUtcMs = 10;
        QCOMPARE(history.submit(late).disposition, ResultDisposition::Accepted);
        history.submit(testFrame(0)); history.submit(testFrame(1));
        const auto before = history.snapshot();
        late.fv.y = {100, 200, 300};
        QCOMPARE(history.submit(late).disposition, ResultDisposition::Duplicate);
        late.acquisitionId = "conflict"; QCOMPARE(history.submit(late).disposition, ResultDisposition::Conflict);
        const auto s = history.snapshot(); QCOMPARE(s.entries.size(), std::size_t(3)); QCOMPARE(s.revision, before.revision);
        QCOMPARE(s.entries[0].peak.sequence, std::uint64_t(0)); QCOMPARE(s.entries[2].peak.acquiredAtUtcMs, std::int64_t(10));
        QCOMPARE(s.latest->sequence, std::uint64_t(2)); QCOMPARE(*s.entries.back().peak.peakValue, -1.0);
        const auto segments = peakSegments(s); QCOMPARE(segments.size(), std::size_t(1)); QCOMPARE(segments[0].elapsedSeconds[2], 0.5);
        QVERIFY(!history.snapshotAfter(s.revision));
    }
    void nullGapUnitsAndSessions() {
        ResultHistory history; history.beginRun("test-run");
        history.submit(testFrame(0));
        auto empty = testFrame(1); empty.fv.x.clear(); empty.fv.y.clear(); history.submit(empty);
        history.submit(testFrame(2)); history.submit(testFrame(4));
        auto unit = testFrame(5); unit.fv.yUnit = "other"; history.submit(unit);
        history.beginRun("second-run"); QVERIFY(!history.snapshot().latest);
        history.submit(testFrame(0, "second-run"));
        const auto s = history.snapshot(); QCOMPARE(s.entries.size(), std::size_t(6));
        QVERIFY(!s.entries[1].peak.peakValue); QVERIFY(!s.entries[1].peak.qualityReason.empty());
        QCOMPARE(peakSegments(s).size(), std::size_t(5)); QCOMPARE(s.latest->runId, std::string("second-run"));
        history.clear(); QVERIFY(history.snapshot().entries.empty()); QVERIFY(!history.snapshot().latest);
        QCOMPARE(s.entries.size(), std::size_t(6)); // Old immutable snapshot remains usable.
    }
    void malformedAndUncalibratedFrames() {
        ResultHistory history; auto f = testFrame(0);
        f.runId.clear(); QCOMPARE(history.submit(f).disposition, ResultDisposition::Invalid);
        f = testFrame(0); f.elapsedSeconds = NAN; QCOMPARE(history.submit(f).disposition, ResultDisposition::Invalid);
        f = testFrame(0); f.batch.sampleCount = 0; QCOMPARE(history.submit(f).disposition, ResultDisposition::Invalid);
        f = testFrame(0); f.fv.x.pop_back(); QCOMPARE(history.submit(f).disposition, ResultDisposition::Accepted);
        QVERIFY(!history.snapshot().entries[0].peak.peakValue); QVERIFY(!history.snapshot().entries[0].frame->metrics.peakAbsorbance);
        f = testFrame(1); f.quality = Quality::Uncalibrated; history.submit(f);
        QCOMPARE(history.snapshot().entries[1].peak.quality, Quality::Uncalibrated);
        QVERIFY(!history.snapshot().entries[1].peak.peakValue);
        f = testFrame(2); f.fv.y = {1, NAN, 3}; f.metrics.peakAbsorbance = INFINITY; f.metrics.concentration = 10;
        history.submit(f); const auto e = history.snapshot().entries.back(); QCOMPARE(*e.peak.peakValue, 3.0);
        QVERIFY(!e.frame->metrics.peakAbsorbance); QVERIFY(!e.frame->metrics.concentration); QCOMPARE(*e.frame->metrics.intensityPercent, 120.0);
        f = testFrame(3); f.fv.yUnit.clear(); history.submit(f); QVERIFY(!history.snapshot().entries.back().peak.peakValue);
        f = testFrame(4); f.processingDiagnostics.push_back({"baseline", false, "injected", NAN}); history.submit(f);
        QCOMPARE(history.snapshot().entries.back().frame->quality, Quality::Invalid);
        QVERIFY(history.snapshot().entries.back().frame->qualityReason.find("诊断") != std::string::npos);
    }
    void capacityNeverEvicts() {
        ResultHistory count(1, 100); count.submit(testFrame(0));
        QCOMPARE(count.submit(testFrame(0)).disposition, ResultDisposition::Duplicate);
        QCOMPARE(count.submit(testFrame(1)).disposition, ResultDisposition::CapacityExceeded);
        QCOMPARE(count.snapshot().entries.size(), std::size_t(1));
        ResultHistory points(100, 8); QCOMPARE(points.submit(testFrame(0)).disposition, ResultDisposition::CapacityExceeded); QVERIFY(points.snapshot().entries.empty());
    }
    void concurrentSnapshotsDoNotLoseBatches() {
        ResultHistory history; history.beginRun("test-run"); std::atomic_bool done{false};
        std::thread writer([&] { for (int i = 0; i < 1000; ++i) history.submit(testFrame(i)); done = true; });
        bool consistent = true;
        while (!done) { auto snapshot = history.snapshot(); if (!snapshot.entries.empty()) consistent = consistent && snapshot.latest->sequence + 1 == snapshot.entries.size(); }
        writer.join(); QVERIFY(consistent); QCOMPARE(history.snapshot().entries.size(), std::size_t(1000));
    }
    void capabilityBoundary() {
        AcquisitionConfig c; c.deviceId = "test"; c.inputChannel = "ai";
        DeviceCapabilities d; d.deviceId = "test"; d.inputChannels = {"ai"}; d.minimumSampleRateHz = 1; d.maximumSampleRateHz = 10000; d.inputMinimumV = -10; d.inputMaximumV = 10;
        QVERIFY(validate(c, d).empty()); c.inputChannel = "foreign"; QVERIFY(!validate(c, d).empty()); c.inputChannel = "ai";
        c.outputChannel = "ao"; d.outputChannels = {"ao"}; d.waveforms = {Waveform::Sine}; QVERIFY(!validate(c, d).empty());
        d.outputMinimumV = -2; d.outputMaximumV = 2; d.maximumOutputFrequencyHz = 20; QVERIFY(validate(c, d).empty());
        c.outputOffsetV = 2; QVERIFY(!validate(c, d).empty()); c.outputChannel.clear(); QVERIFY(validate(c, d).empty());
        c.processingWindowSamples = 10000001; QVERIFY(!validate(c).empty());
    }
};
QTEST_GUILESS_MAIN(ResultTest)
#include "result_test.moc"
