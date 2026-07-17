#include "lfe_decode.hpp"
#include "lfe_coefficients.hpp"

#include <algorithm>

namespace auro3d {
namespace lfe {
namespace {

std::int32_t sign_extend_lfe(std::uint32_t raw, unsigned width) {
    if (!width || width > 32)
        return 0;
    if (width == 32)
        return static_cast<std::int32_t>(raw);
    const unsigned high = width - 1u;
    const std::uint32_t sign = 1u << high;
    const std::uint32_t mask = (sign << 1) - 1u;
    raw &= mask;
    return static_cast<std::int32_t>(raw) - static_cast<std::int32_t>((raw & sign) ? (sign << 1) : 0u);
}

template <std::size_t N>
coefficients::View view(const float (&values)[N], unsigned factor) {
    return {values, N, factor};
}

bool interpolation_stages(std::uint32_t factor, std::vector<coefficients::View>& stages) {
    using namespace coefficients;
    stages.clear();
    if (factor == 1)
        return true;
    if (factor == 2 || factor == 4) {
        stages = {view(if2_s0_f2, 2)};
        if (factor == 4)
            stages.push_back(view(if4_s1_f2, 2));
        return true;
    }
    if (factor == 40 || factor == 80) {
        stages = {
            view(if40_s0_f2, 2), view(if40_s1_f2, 2),
            view(if40_s2_f2, 2), view(if40_s3_f5, 5)
        };
        if (factor == 80)
            stages.push_back(view(if80_s4_f2, 2));
        return true;
    }
    if (factor == 160 || factor == 320 || factor == 640) {
        stages = {
            view(if160_s0_f2, 2), view(if160_s1_f2, 2),
            view(if160_s2_f2, 2), view(if160_s3_f5, 5),
            view(if160_s4_f2, 2), view(if160_s5_f2, 2)
        };
        if (factor >= 320)
            stages.push_back(view(if320_s6_f2, 2));
        if (factor >= 640)
            stages.push_back(view(if640_s7_f2, 2));
        return true;
    }
    return false;
}

bool initialize_interpolation(InterpolationState& state, std::uint32_t factor) {
    std::vector<coefficients::View> stages;
    if (!interpolation_stages(factor, stages))
        return false;

    std::uint32_t product = 1;
    state.stages.clear();
    state.stages.reserve(stages.size());
    for (const auto& stage : stages) {
        if (!stage.factor || !stage.size)
            return false;
        product *= stage.factor;
        InterpolationStageState stage_state{};
        stage_state.factor = stage.factor;
        stage_state.coefficients.assign(stage.data, stage.data + stage.size);
        const std::size_t taps_per_phase =
            (stage.size + stage.factor - 1u) / stage.factor;
        stage_state.history.assign(taps_per_phase - 1u, 0.0f);
        state.stages.push_back(std::move(stage_state));
    }

    if (product != factor)
        return false;

    state.factor = factor;
    state.next_phase = 0;
    state.phase_valid = false;
    state.pending.clear();
    return true;
}

std::int32_t float_to_pcm24(float sample) {
    const float scaled = sample * 8388608.0f;
    if (scaled >= 8388607.0f)
        return 8388607;
    if (scaled <= -8388608.0f)
        return -8388608;
    return static_cast<std::int32_t>(scaled);
}

void interpolate_stage_sample(
    InterpolationStageState& stage,
    float input,
    std::vector<float>& output) {
    output.resize(stage.factor);
    for (std::uint32_t phase = 0; phase < stage.factor; ++phase) {
        float sum = 0.0f;
        std::size_t lag = 0;
        for (std::size_t coefficient = phase;
             coefficient < stage.coefficients.size();
             coefficient += stage.factor, ++lag) {
            const float sample = lag == 0u
                ? input
                : lag <= stage.history.size()
                    ? stage.history[stage.history.size() - lag]
                    : 0.0f;
            const float product = sample * stage.coefficients[coefficient];
            sum = sum + product;
        }
        output[phase] = sum;
    }
    if (!stage.history.empty()) {
        std::move(stage.history.begin() + 1, stage.history.end(), stage.history.begin());
        stage.history.back() = input;
    }
}

void interpolate_sample(
    InterpolationState& state,
    float input,
    std::vector<float>& output) {
    std::vector<float> current(1u, input);
    std::vector<float> expanded;
    std::vector<float> stage_output;
    for (auto& stage : state.stages) {
        expanded.clear();
        expanded.reserve(current.size() * stage.factor);
        for (const float sample : current) {
            interpolate_stage_sample(stage, sample, stage_output);
            expanded.insert(expanded.end(), stage_output.begin(), stage_output.end());
        }
        current.swap(expanded);
    }
    output = std::move(current);
}

bool interpolate_block(
    InterpolationState& state,
    std::uint32_t block_size,
    std::uint32_t phase,
    const std::vector<std::int32_t>& residuals,
    std::vector<std::int32_t>& output,
    std::string& error) {
    if (!state.phase_valid) {
        state.pending.assign(phase, 0.0f);
        state.phase_valid = true;
    } else if (state.pending.size() != phase) {
        error = "LFE interpolation phase discontinuity";
        return false;
    }

    std::vector<float> expanded;
    for (const std::int32_t residual : residuals) {
        const float input = static_cast<float>(residual) * (1.0f / 32768.0f);
        interpolate_sample(state, input, expanded);
        state.pending.insert(state.pending.end(), expanded.begin(), expanded.end());
    }

    if (state.pending.size() < block_size) {
        error = "LFE interpolation output truncated";
        return false;
    }

    output.resize(block_size);
    std::transform(
        state.pending.begin(),
        state.pending.begin() + static_cast<std::ptrdiff_t>(block_size),
        output.begin(),
        float_to_pcm24);
    state.pending.erase(
        state.pending.begin(),
        state.pending.begin() + static_cast<std::ptrdiff_t>(block_size));
    return true;
}

} // namespace

std::uint32_t default_resample_factor(std::uint32_t sample_rate) {
    // asc_1E9160, selected by lfe::resample_factor/Processor::initialize.
    switch (sample_rate) {
    case 16000: return 40;
    case 24000:
    case 32000: return 80;
    case 44100:
    case 48000: return 160;
    case 88200:
    case 96000: return 320;
    case 176400:
    case 192000: return 640;
    default: return 1;
    }
}

bool decode_stream_residuals(
    cx::Bits& bits,
    std::size_t sample_count,
    std::vector<std::int32_t>& out,
    std::string& error) {
    if (!sample_count) {
        out.clear();
        return true;
    }

    std::uint32_t width_m1 = 0;
    if (!bits.get(4, width_m1)) {
        error = "LFE residual width read failed";
        return false;
    }
    const unsigned width = (width_m1 & 0xFu) + 1u;
    if (!width || width > 16u) {
        error = "LFE residual width out of range";
        return false;
    }

    out.resize(sample_count);
    for (std::size_t i = 0; i < sample_count; ++i) {
        if (bits.remaining_bits() < width) {
            error = "LFE residual truncated";
            return false;
        }
        std::uint32_t raw = 0;
        if (!bits.get(width, raw)) {
            error = "LFE residual read failed";
            return false;
        }
        out[i] = sign_extend_lfe(raw, width);
    }
    return true;
}

bool decode_lfe_payload(
    cx::Bits& bits,
    std::uint32_t block_size,
    std::uint32_t resample_factor,
    std::uint32_t first_stream_index,
    std::uint32_t pdu_stream_count,
    std::vector<InterpolationState>& interpolation_states,
    std::vector<std::vector<std::int32_t>>& stream_samples,
    std::string& error) {
    if (!block_size || !resample_factor || !pdu_stream_count) {
        error = "LFE invalid stream geometry";
        return false;
    }

    unsigned phase_bits = 0;
    for (std::uint32_t maximum = resample_factor - 1u; maximum; maximum >>= 1u)
        ++phase_bits;
    std::uint32_t phase = 0;
    if (phase_bits && !bits.get(phase_bits, phase)) {
        error = "LFE phase read failed";
        return false;
    }
    if (phase >= resample_factor) {
        error = "LFE phase out of interpolation range";
        return false;
    }

    const std::uint32_t residual_count = phase < block_size
        ? (block_size - phase + resample_factor - 1u) / resample_factor
        : 0u;

    if (interpolation_states.size() < first_stream_index + pdu_stream_count)
        interpolation_states.resize(first_stream_index + pdu_stream_count);
    stream_samples.assign(pdu_stream_count, {});
    for (std::uint32_t s = 0; s < pdu_stream_count; ++s) {
        std::vector<std::int32_t> residuals;
        if (!decode_stream_residuals(bits, residual_count, residuals, error)) {
            error += " factor=" + std::to_string(resample_factor) +
                     " phase=" + std::to_string(phase) +
                     " residuals=" + std::to_string(residual_count) +
                     " stream=" + std::to_string(s) +
                     " remaining=" + std::to_string(bits.remaining_bits());
            return false;
        }
        auto& interpolation = interpolation_states[first_stream_index + s];
        if (interpolation.factor != resample_factor &&
            !initialize_interpolation(interpolation, resample_factor)) {
            error = "LFE interpolation initialization failed";
            return false;
        }
        if (!interpolate_block(
                interpolation,
                block_size,
                phase,
                residuals,
                stream_samples[s],
                error))
            return false;
        const std::uint64_t next_phase =
            static_cast<std::uint64_t>(phase) +
            static_cast<std::uint64_t>(residual_count) * resample_factor - block_size;
        if (next_phase >= resample_factor) {
            error = "LFE interpolation next phase out of range";
            return false;
        }
        interpolation.next_phase = static_cast<std::uint32_t>(next_phase);
        interpolation.phase_valid = true;
    }

    return true;
}

} // namespace lfe
} // namespace auro3d
