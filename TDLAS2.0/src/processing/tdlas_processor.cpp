#include "processing/tdlas_processor.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace tdlas {
namespace {

constexpr double kPi = 3.14159265358979323846;
using Clock = std::chrono::steady_clock;

struct Biquad { double b0, b1, b2, a1, a2; };

ProcessingStageDiagnostic diagnostic(const std::string& stage, bool succeeded,
                                     const std::string& message, Clock::time_point started) {
    const auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
    return {stage, succeeded, message, elapsed};
}

Biquad makeLowPass(double normalizedFrequency, double q) {
    const double omega = kPi * normalizedFrequency;
    const double alpha = std::sin(omega) / (2.0 * q);
    const double cosine = std::cos(omega);
    const double denominator = 1.0 + alpha;
    return {(1.0 - cosine) / (2.0 * denominator), (1.0 - cosine) / denominator,
            (1.0 - cosine) / (2.0 * denominator), -2.0 * cosine / denominator,
            (1.0 - alpha) / denominator};
}

bool applyBiquad(std::vector<double>& values, const Biquad& filter) {
    double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    for (double& value : values) {
        const double output = filter.b0 * value + filter.b1 * x1 + filter.b2 * x2
            - filter.a1 * y1 - filter.a2 * y2;
        if (!std::isfinite(output)) return false;
        x2 = x1; x1 = value; y2 = y1; y1 = output; value = output;
    }
    return true;
}

bool applyZeroPhaseLowPass(std::vector<double>& values) {
    const std::array<Biquad, 2> filters = {
        makeLowPass(0.1, 0.541196100146197), makeLowPass(0.1, 1.306562964876377)};
    for (const auto& filter : filters) if (!applyBiquad(values, filter)) return false;
    std::reverse(values.begin(), values.end());
    for (const auto& filter : filters) if (!applyBiquad(values, filter)) return false;
    std::reverse(values.begin(), values.end());
    return true;
}

bool solveLinearSystem(std::array<std::array<double, 6>, 5> matrix,
                       std::array<double, 5>& solution) {
    for (int column = 0; column < 5; ++column) {
        int pivot = column;
        for (int row = column + 1; row < 5; ++row)
            if (std::abs(matrix[row][column]) > std::abs(matrix[pivot][column])) pivot = row;
        if (!std::isfinite(matrix[pivot][column]) || std::abs(matrix[pivot][column]) < 1e-18) return false;
        std::swap(matrix[column], matrix[pivot]);
        const double divisor = matrix[column][column];
        for (int value = column; value <= 5; ++value) matrix[column][value] /= divisor;
        for (int row = 0; row < 5; ++row) {
            if (row == column) continue;
            const double factor = matrix[row][column];
            for (int value = column; value <= 5; ++value)
                matrix[row][value] -= factor * matrix[column][value];
        }
    }
    for (int index = 0; index < 5; ++index) {
        solution[index] = matrix[index][5];
        if (!std::isfinite(solution[index])) return false;
    }
    return true;
}

std::optional<std::array<double, 5>> polynomialFit4(const std::vector<double>& x,
                                                     const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 5) return {};
    std::array<std::array<double, 6>, 5> normal{};
    for (std::size_t index = 0; index < x.size(); ++index) {
        std::array<double, 5> powers{1, 0, 0, 0, 0};
        for (int power = 1; power < 5; ++power) powers[power] = powers[power - 1] * x[index];
        for (int row = 0; row < 5; ++row) {
            for (int column = 0; column < 5; ++column) normal[row][column] += powers[row] * powers[column];
            normal[row][5] += powers[row] * y[index];
        }
    }
    std::array<double, 5> coefficients{};
    if (!solveLinearSystem(normal, coefficients)) return {};
    return coefficients;
}

template<std::size_t N>
double evaluatePolynomial(const std::array<double, N>& coefficients, double x) {
    double value = 0;
    for (std::size_t index = coefficients.size(); index-- > 0;) value = value * x + coefficients[index];
    return value;
}

double voigtShape(const std::array<double, 4>& parameters, double frequency) {
    static constexpr std::array<double, 4> a{-1.2150, -1.3509, -1.2150, -1.3509};
    static constexpr std::array<double, 4> b{1.2359, 0.3786, -1.2359, -0.3786};
    static constexpr std::array<double, 4> c{-0.3085, 0.5906, -0.3085, 0.5906};
    static constexpr std::array<double, 4> d{0.0210, -1.1858, -0.0210, 1.1858};
    const double gaussianFwhm = std::max(parameters[2], 1e-9);
    const double ratio = std::max(parameters[3], 0.0) / gaussianFwhm * std::sqrt(std::log(2.0));
    const double y = 2.0 * std::sqrt(std::log(2.0)) * (frequency - parameters[0]);
    const double phiG = 2.0 / gaussianFwhm * std::sqrt(std::log(2.0) / kPi);
    double profile = 0;
    for (int index = 0; index < 4; ++index) {
        const double numerator = c[index] * (ratio - a[index]) + d[index] * (y / gaussianFwhm - b[index]);
        const double denominator = std::pow(ratio - a[index], 2) + std::pow(y / gaussianFwhm - b[index], 2);
        profile += phiG * numerator / denominator;
    }
    return profile * parameters[1];
}

double fitError(const std::array<double, 4>& parameters, const std::vector<double>& x,
                const std::vector<double>& y) {
    double error = 0;
    for (std::size_t index = 0; index < x.size(); ++index) {
        const double difference = voigtShape(parameters, x[index]) - y[index];
        error += difference * difference;
    }
    return error;
}

std::optional<std::array<double, 4>> fitVoigt(const std::vector<double>& x,
                                              const std::vector<double>& y) {
    if (x.size() != y.size() || x.size() < 4 || !std::all_of(x.begin(), x.end(), [](double v) { return std::isfinite(v); }) ||
        !std::all_of(y.begin(), y.end(), [](double v) { return std::isfinite(v); })) return {};
    const auto peak = std::max_element(y.begin(), y.end());
    const auto trough = std::min_element(y.begin(), y.end());
    if (peak == y.end() || *peak <= 0 || *peak - *trough <= 1e-12) return {};
    std::array<double, 4> parameters{0, 0.004, 0.05, 0.05};
    const std::array<double, 4> lower{-0.5, 0, 0.001, 0.001};
    const std::array<double, 4> upper{0.5, 0.2, 0.5, 0.5};
    double error = fitError(parameters, x, y);
    if (!std::isfinite(error)) return {};
    std::array<double, 4> steps{0.02, 0.002, 0.02, 0.02};
    for (int iteration = 0; iteration < 80; ++iteration) {
        bool improved = false;
        for (int parameter = 0; parameter < 4; ++parameter) {
            for (double direction : {-1.0, 1.0}) {
                auto candidate = parameters;
                candidate[parameter] = std::clamp(candidate[parameter] + direction * steps[parameter], lower[parameter], upper[parameter]);
                const double candidateError = fitError(candidate, x, y);
                if (std::isfinite(candidateError) && candidateError < error) {
                    parameters = candidate; error = candidateError; improved = true;
                }
            }
        }
        if (!improved) for (double& step : steps) step *= 0.55;
        if (*std::max_element(steps.begin(), steps.end()) < 1e-6) break;
    }
    return parameters;
}

bool multiplyWouldOverflow(std::size_t a, std::size_t b) {
    return b != 0 && a > std::numeric_limits<std::size_t>::max() / b;
}

} // namespace

std::string validate(const LegacyTdlasConfig& c) {
    if (!c.windowSamples || !c.samplesPerCycle || !c.averagedCyclesPerSpectrum || !c.averagedSpectra)
        return "TDLAS 窗口、周期点数和平均次数必须大于零";
    if (!std::isfinite(c.voltageOffsetV)) return "旧版电压偏置必须为有限数";
    if (multiplyWouldOverflow(c.samplesPerCycle, c.averagedCyclesPerSpectrum) ||
        multiplyWouldOverflow(c.samplesPerCycle * c.averagedCyclesPerSpectrum, c.averagedSpectra))
        return "TDLAS 周期与平均次数乘积溢出";
    const auto required = c.samplesPerCycle * c.averagedCyclesPerSpectrum * c.averagedSpectra;
    if (c.windowSamples < required) return "TDLAS 窗口小于完整平均批次";
    if (c.windowSamples > 10000000) return "TDLAS 窗口超过基础版内存预算";
    return {};
}

LegacyTdlasProcessor::LegacyTdlasProcessor(LegacyTdlasConfig config) : config_(config) {
    if (const auto error = validate(config_); !error.empty()) throw std::invalid_argument(error);
}

LegacyTdlasSpectrum LegacyTdlasProcessor::processWindow(const std::vector<double>& samples) const {
    LegacyTdlasSpectrum result;
    result.cycleSamples = config_.samplesPerCycle;
    const auto fail = [&](const std::string& stage, const std::string& message, Clock::time_point started) {
        result.failureStage = stage; result.message = message;
        result.diagnostics.push_back(diagnostic(stage, false, message, started));
        return result;
    };
    const auto inputStarted = Clock::now();
    const auto perSpectrum = config_.samplesPerCycle * config_.averagedCyclesPerSpectrum;
    const auto usable = perSpectrum * config_.averagedSpectra;
    if (samples.size() < config_.windowSamples || config_.windowSamples < usable)
        return fail("input", "完整窗口样本不足", inputStarted);
    auto filtered = std::vector<double>(samples.end() - static_cast<std::ptrdiff_t>(usable), samples.end());
    if (!std::all_of(filtered.begin(), filtered.end(), [](double value) { return std::isfinite(value); }))
        return fail("low_pass", "低通输入含非有限值", inputStarted);
    result.diagnostics.push_back(diagnostic("input", true, "完整窗口与平均边界有效", inputStarted));

    const auto filterStarted = Clock::now();
    if (!applyZeroPhaseLowPass(filtered)) return fail("low_pass", "低通滤波产生非有限值", filterStarted);
    result.diagnostics.push_back(diagnostic("low_pass", true, "旧版四阶零相位低通完成", filterStarted));

    const auto averageStarted = Clock::now();
    const auto processedBegin = std::min(config_.samplesPerCycle - 1,
        static_cast<std::size_t>(std::max(0.0, std::floor(0.3 * static_cast<double>(config_.samplesPerCycle)) - 1)));
    const auto processedEnd = std::min(config_.samplesPerCycle - 1,
        static_cast<std::size_t>(std::max(0.0, std::floor(0.9 * static_cast<double>(config_.samplesPerCycle)))));
    if (processedEnd < processedBegin) return fail("cycle_average", "周期截取边界无效", averageStarted);
    result.sampleOffset = processedBegin;
    std::vector<double> accumulatedAbsorbance, accumulatedFrequency, accumulatedVoltage, accumulatedBaseline;
    bool averageDiagnosticAdded = false;
    double baselineDurationMs = 0;
    double calibrationDurationMs = 0;
    for (std::size_t spectrumIndex = 0; spectrumIndex < config_.averagedSpectra; ++spectrumIndex) {
        std::vector<double> cycleAverage(config_.samplesPerCycle, 0);
        const auto spectrumOffset = spectrumIndex * perSpectrum;
        for (std::size_t cycle = 0; cycle < config_.averagedCyclesPerSpectrum; ++cycle) {
            const auto cycleOffset = spectrumOffset + cycle * config_.samplesPerCycle;
            for (std::size_t sample = 0; sample < config_.samplesPerCycle; ++sample)
                cycleAverage[sample] += filtered[cycleOffset + sample] / static_cast<double>(config_.averagedCyclesPerSpectrum);
        }
        const auto maximum = std::max_element(cycleAverage.begin(), cycleAverage.end());
        const auto peakIndex = static_cast<std::size_t>(maximum - cycleAverage.begin());
        std::vector<double> aligned(config_.samplesPerCycle);
        for (std::size_t sample = 0; sample < config_.samplesPerCycle; ++sample)
            aligned[sample] = cycleAverage[(peakIndex + sample) % config_.samplesPerCycle] + config_.voltageOffsetV;
        std::vector<double> y(aligned.begin() + static_cast<std::ptrdiff_t>(processedBegin),
                              aligned.begin() + static_cast<std::ptrdiff_t>(processedEnd + 1));
        std::vector<double> x(y.size());
        for (std::size_t index = 0; index < x.size(); ++index) x[index] = static_cast<double>(index + 1);
        if (!averageDiagnosticAdded) {
            result.diagnostics.push_back(diagnostic("cycle_average", true, "周期平均与峰值对齐完成", averageStarted));
            averageDiagnosticAdded = true;
        }

        const auto baselineStarted = Clock::now();
        std::vector<double> baselineX, baselineY;
        const auto firstEnd = std::min(y.size(), static_cast<std::size_t>(std::floor(0.35 * y.size())));
        const auto tail = static_cast<std::size_t>(std::floor(0.4 * y.size()));
        const auto secondBegin = y.size() > tail + 1 ? y.size() - tail - 1 : 0;
        for (std::size_t index = 0; index < firstEnd; ++index) { baselineX.push_back(x[index]); baselineY.push_back(y[index]); }
        for (std::size_t index = secondBegin; index < y.size(); ++index) { baselineX.push_back(x[index]); baselineY.push_back(y[index]); }
        const auto polynomial = polynomialFit4(baselineX, baselineY);
        if (!polynomial) return fail("baseline", "四阶基线拟合失败：有效边缘点不足或矩阵奇异", baselineStarted);
        std::vector<double> absorbance(y.size()), baselineValues(y.size());
        for (std::size_t index = 0; index < y.size(); ++index) {
            const double baseline = evaluatePolynomial(*polynomial, x[index]);
            baselineValues[index] = baseline;
            absorbance[index] = std::log(std::max(baseline, 1e-12) / std::max(y[index], 1e-12));
            if (!std::isfinite(absorbance[index])) return fail("baseline", "吸光度计算产生非有限值", baselineStarted);
        }
        baselineDurationMs += std::chrono::duration<double, std::milli>(Clock::now() - baselineStarted).count();

        const auto calibrationStarted = Clock::now();
        constexpr std::array<double, 4> frequencyPolynomial{0.609383083614659, 0.0241628513601265,
                                                            5.82968569053455e-06, -3.02716372873655e-09};
        std::vector<double> frequency(y.size());
        const auto absorptionPeak = std::max_element(absorbance.begin(), absorbance.end());
        const auto absorptionPeakIndex = static_cast<std::size_t>(absorptionPeak - absorbance.begin());
        const double peakFrequency = evaluatePolynomial(frequencyPolynomial, x[absorptionPeakIndex]);
        for (std::size_t index = 0; index < frequency.size(); ++index)
            frequency[index] = (evaluatePolynomial(frequencyPolynomial, x[index]) - peakFrequency) * 0.8761 / 30.0;
        calibrationDurationMs += std::chrono::duration<double, std::milli>(Clock::now() - calibrationStarted).count();

        if (spectrumIndex == 0) {
            accumulatedAbsorbance = std::move(absorbance); accumulatedFrequency = std::move(frequency);
            accumulatedVoltage = std::move(y); accumulatedBaseline = std::move(baselineValues);
            const auto referenceIndex = std::min<std::size_t>(49, aligned.size() - 1);
            result.legacyIntensityPercent = (aligned.back() - aligned[referenceIndex]) / 2.0 * 100.0;
        } else {
            for (std::size_t index = 0; index < absorbance.size(); ++index) {
                accumulatedAbsorbance[index] += absorbance[index]; accumulatedVoltage[index] += y[index];
                accumulatedBaseline[index] += baselineValues[index];
            }
        }
    }
    for (std::size_t index = 0; index < accumulatedAbsorbance.size(); ++index) {
        accumulatedAbsorbance[index] /= static_cast<double>(config_.averagedSpectra);
        accumulatedVoltage[index] /= static_cast<double>(config_.averagedSpectra);
        accumulatedBaseline[index] /= static_cast<double>(config_.averagedSpectra);
    }
    result.diagnostics.push_back({"baseline", true, "旧版边缘区四阶基线与 log(I0/I) 完成", baselineDurationMs});
    result.diagnostics.push_back({"frequency_calibration", true,
        "已复现旧版频率偏移常数；来源和物理标定尚未确认", calibrationDurationMs});

    const auto fitStarted = Clock::now();
    const auto fit = fitVoigt(accumulatedFrequency, accumulatedAbsorbance);
    if (!fit) return fail("voigt_fit", "Voigt 拟合失败：输入无可辨识正吸收峰或数值无效", fitStarted);
    result.legacyFrequencyOffset = std::move(accumulatedFrequency);
    result.absorbance = std::move(accumulatedAbsorbance);
    result.voltage = std::move(accumulatedVoltage);
    result.fittedAbsorbance.resize(result.legacyFrequencyOffset.size());
    result.fittedVoltage.resize(result.legacyFrequencyOffset.size());
    for (std::size_t index = 0; index < result.legacyFrequencyOffset.size(); ++index) {
        result.fittedAbsorbance[index] = voigtShape(*fit, result.legacyFrequencyOffset[index]);
        result.fittedVoltage[index] = accumulatedBaseline[index] * std::exp(-result.fittedAbsorbance[index]);
    }
    result.legacyFittedPeak = *std::max_element(result.fittedAbsorbance.begin(), result.fittedAbsorbance.end());
    result.diagnostics.push_back(diagnostic("voigt_fit", true, "旧版有界坐标搜索拟合完成", fitStarted));
    result.succeeded = true;
    result.message = "旧版 TDLAS 数值链完成；物理标定仍未确认";
    return result;
}

LegacyTdlasAdapter::LegacyTdlasAdapter(LegacyTdlasConfig config) : processor_(config) {
    samples_.reserve(config.windowSamples);
}

std::vector<AcquisitionFrame> LegacyTdlasAdapter::process(const SampleBlock& block) {
    if (block.runId.empty() || block.deviceId.empty() || block.channelId.empty() || !block.configRevision ||
        !std::isfinite(block.sampleRateHz) || block.sampleRateHz <= 0 || block.samples.empty())
        throw std::invalid_argument("TDLAS 适配器收到无效输入块身份或采样率");
    if (!initialized_) {
        runId_ = block.runId; deviceId_ = block.deviceId; channelId_ = block.channelId;
        configRevision_ = block.configRevision; sampleRateHz_ = block.sampleRateHz;
        nextBlockSequence_ = block.sequence; initialized_ = true;
    }
    if (block.runId != runId_ || block.deviceId != deviceId_ || block.channelId != channelId_ ||
        block.configRevision != configRevision_ || block.sampleRateHz != sampleRateHz_ || block.sequence != nextBlockSequence_)
        throw std::invalid_argument("TDLAS 批次边界不连续或输入身份发生变化");
    ++nextBlockSequence_;
    samples_.insert(samples_.end(), block.samples.begin(), block.samples.end());
    parts_.push_back({block.sequence, block.acquiredAtUtcMs, block.elapsedNs, 0, block.samples.size()});

    std::vector<AcquisitionFrame> frames;
    while (samples_.size() >= processor_.config().windowSamples) {
        const std::vector<double> window(samples_.begin(), samples_.begin() + static_cast<std::ptrdiff_t>(processor_.config().windowSamples));
        frames.push_back(makeFrame(block, processor_.processWindow(window)));
        consume(processor_.config().windowSamples);
    }
    return frames;
}

AcquisitionFrame LegacyTdlasAdapter::makeFrame(const SampleBlock& identity, const LegacyTdlasSpectrum& spectrum) {
    if (parts_.empty()) throw std::logic_error("TDLAS 适配器批次元数据为空");
    const auto& first = parts_.front();
    auto remaining = processor_.config().windowSamples;
    auto lastBlock = first.blockSequence;
    for (const auto& part : parts_) {
        lastBlock = part.blockSequence;
        if (part.remaining >= remaining) break;
        remaining -= part.remaining;
    }
    AcquisitionFrame frame;
    frame.runId = identity.runId; frame.acquisitionId = identity.runId + ":legacy:" + std::to_string(nextBatchSequence_);
    frame.sequence = nextBatchSequence_++; frame.deviceId = identity.deviceId; frame.channelId = identity.channelId;
    frame.configRevision = identity.configRevision;
    const double offsetSeconds = static_cast<double>(first.offset) / sampleRateHz_;
    frame.acquiredAtUtcMs = first.acquiredAtUtcMs + static_cast<std::int64_t>(std::llround(offsetSeconds * 1000));
    frame.elapsedSeconds = static_cast<double>(first.elapsedNs) / 1e9 + offsetSeconds;
    frame.batch = {firstSampleIndex_, processor_.config().windowSamples, first.blockSequence, lastBlock};
    frame.processingVersion = "legacy-tdlas-reviewed-v1";
    frame.baselineMethod = "旧版边缘区四阶多项式（参数来源未确认）";
    frame.polarityTransform = "log(I0/I)；FV 方向未定义";
    frame.modelId = "v1.6-rational-voigt-coordinate-search";
    frame.processingDiagnostics = spectrum.diagnostics;
    if (!spectrum.succeeded) {
        frame.quality = Quality::Invalid; frame.qualityReason = spectrum.message;
        frame.fitStatus = spectrum.failureStage + "_failed"; frame.metrics.quality = Quality::Invalid;
        return frame;
    }
    std::vector<double> sampleIndex(spectrum.absorbance.size());
    for (std::size_t index = 0; index < sampleIndex.size(); ++index)
        sampleIndex[index] = static_cast<double>(spectrum.sampleOffset + index);
    frame.observed = {sampleIndex, spectrum.absorbance, "周期内样本索引", "1", "旧版吸光度", "1"};
    frame.fitted = {sampleIndex, spectrum.fittedAbsorbance, "周期内样本索引", "1", "旧版吸光度", "1"};
    // FV is intentionally empty: the supplied material does not establish
    // that fitted absorbance is the product's FV physical quantity.
    frame.fitStatus = "success_uncalibrated";
    frame.quality = Quality::Uncalibrated;
    frame.qualityReason = "旧版数值链已完成；FV 定义、频率标定、光强参考和浓度公式未确认";
    frame.metrics.quality = Quality::Uncalibrated;
    return frame;
}

void LegacyTdlasAdapter::consume(std::size_t count) {
    samples_.erase(samples_.begin(), samples_.begin() + static_cast<std::ptrdiff_t>(count));
    firstSampleIndex_ += count;
    while (count && !parts_.empty()) {
        auto& part = parts_.front();
        const auto take = std::min(count, part.remaining);
        part.offset += take; part.remaining -= take; count -= take;
        if (!part.remaining) parts_.pop_front();
    }
}

} // namespace tdlas
