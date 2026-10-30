#include "core/contracts.h"
#include "core/sample_router.h"
#include <QtTest>
#include <limits>
#include <stdexcept>
using namespace tdlas;
class CoreTest : public QObject {
    Q_OBJECT
private slots:
    void configuration() {
        AcquisitionConfig c;
        QVERIFY(!validate(c).empty()); c.deviceId = "fake"; c.inputChannel = "fake/ai";
        QVERIFY(validate(c).empty());
        c.outputChannel = "fake/ao"; c.outputAmplitudeV = 9; c.outputOffsetV = 2;
        QVERIFY(!validate(c).empty()); c.outputChannel.clear(); QVERIFY(validate(c).empty());
        c.sampleRateHz = std::numeric_limits<double>::quiet_NaN(); QVERIFY(!validate(c).empty());
        c.sampleRateHz = 10000; c.inputMinimumV = c.inputMaximumV; QVERIFY(!validate(c).empty());
        c.inputMinimumV = -10; c.queueCapacitySamples = 10; QVERIFY(!validate(c).empty());
        c.queueCapacitySamples = 100000; c.scanPeriodSamples = 0; QVERIFY(!validate(c).empty());
    }
    void peakSemantics() {
        AcquisitionFrame f; f.quality = Quality::Valid; f.fv.x = {0, 1, 2};
        for (const auto& row : std::vector<std::vector<double>>{{0,2,1}, {1,5,3}, {-3,-1,-2}}) {
            f.fv.y = row; const auto p = extractPeak(f); QVERIFY(p.peakValue); QCOMPARE(*p.peakValue, row[1]); QCOMPARE(*p.peakX, 1.0);
        }
        f.fv.y = {1, std::numeric_limits<double>::quiet_NaN(), 3}; QCOMPARE(*extractPeak(f).peakValue, 3.0);
        f.fv.y = {3,3,1}; QCOMPARE(*extractPeak(f).peakX, 0.0);
        f.fv.pointValid = {false, true, true}; QCOMPARE(*extractPeak(f).peakX, 1.0);
        f.fv.pointValid = {false}; QVERIFY(!extractPeak(f).peakValue); f.fv.pointValid.clear();
        f.fv.y = {NAN, INFINITY, NAN}; QVERIFY(!extractPeak(f).peakValue);
        f.fv.y.clear(); QVERIFY(!extractPeak(f).peakValue);
        f.fv.y = {1,2,3}; f.quality = Quality::Invalid; QVERIFY(!extractPeak(f).peakValue);
    }
    void boundedQueueAndDrain() {
        SampleRouter router(6, 2);
        auto b = std::make_shared<SampleBlock>(); b->samples = {1,2,3}; b->sampleRateHz = 10000;
        QCOMPARE(router.submit(b), SubmitResult::Accepted); QCOMPARE(router.submit(b), SubmitResult::Accepted);
        QCOMPARE(router.submit(b), SubmitResult::Overflow);
        const auto s = router.snapshot(); QCOMPARE(s.samples.size(), std::size_t(2)); QCOMPARE(s.samples[0], 2.0);
        QCOMPARE(s.stats.acceptedBlocks, std::uint64_t(2)); QCOMPARE(s.stats.acceptedSamples, std::uint64_t(6));
        QCOMPARE(s.stats.queuedBlocks, std::size_t(2)); QCOMPARE(s.stats.queuedSamples, std::size_t(6));
        QCOMPARE(s.stats.highWaterBlocks, std::size_t(2)); QCOMPARE(s.stats.highWaterSamples, std::size_t(6));
        QCOMPARE(s.stats.rejectedBlocks, std::uint64_t(1)); QVERIFY(s.stats.accountingConsistent()); QVERIFY(!s.stats.terminalComplete());
        QVERIFY(router.take());
        auto processing = router.snapshot().stats;
        QCOMPARE(processing.queuedBlocks, std::size_t(1)); QCOMPARE(processing.processingBlocks, std::size_t(1));
        QCOMPARE(processing.processingSamples, std::size_t(3)); QVERIFY(processing.accountingConsistent());
        router.complete(true); QVERIFY(router.take()); router.complete(true); QVERIFY(!router.take());
        const auto done = router.snapshot().stats;
        QCOMPARE(done.processedBlocks, std::uint64_t(2)); QCOMPARE(done.processedSamples, std::uint64_t(6));
        QCOMPARE(done.terminalBlocks(), done.acceptedBlocks); QCOMPARE(done.terminalSamples(), done.acceptedSamples);
        QVERIFY(done.terminalComplete());
        QVERIFY_THROWS_EXCEPTION(std::logic_error, router.complete(true));
        QCOMPARE(router.submit(b), SubmitResult::Closed);
    }
};
QTEST_GUILESS_MAIN(CoreTest)
#include "core_test.moc"
