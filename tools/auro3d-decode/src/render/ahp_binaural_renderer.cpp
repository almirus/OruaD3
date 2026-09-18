#include "ahp_binaural_renderer.hpp"
#include "peak_limiter.hpp"
#include "native_sample_convertor.hpp"
#include "headphone_pca_bank.hpp"
#include "headphone_lfe_processing.hpp"
#include "headphone_source_manager.hpp"
#include "am4hp_core_processor.hpp"
#include <array>
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace auro3d {
namespace {

double read_sample(const std::uint8_t* p, unsigned bits) {
    if (bits == 16) {
        std::int16_t v;
        std::memcpy(&v, p, 2);
        return v / 32768.0;
    }
    if (bits == 24) {
        std::int32_t v = (std::int32_t(p[0]) | (std::int32_t(p[1]) << 8)
            | (std::int32_t(p[2]) << 16));
        if (v & 0x800000)
            v |= ~0xffffff;
        return v / 8388608.0;
    }
    float v;
    std::memcpy(&v, p, 4);
    return v;
}

void write_sample(std::vector<std::uint8_t>& out, double x, unsigned bits) {
    if (bits == 16) {
        auto v = static_cast<std::int32_t>(std::llround(x * 32768.0));
        if (v > 32767)
            v = 32767;
        if (v < -32768)
            v = -32768;
        const auto s = static_cast<std::int16_t>(v);
        const auto* p = reinterpret_cast<const std::uint8_t*>(&s);
        out.insert(out.end(), p, p + 2);
        return;
    }
    if (bits == 24) {
        const auto v = native_float_to_pcm24(static_cast<float>(x));
        out.push_back(static_cast<std::uint8_t>(v & 255));
        out.push_back(static_cast<std::uint8_t>((v >> 8) & 255));
        out.push_back(static_cast<std::uint8_t>((v >> 16) & 255));
        return;
    }
    auto v = static_cast<std::int32_t>(std::llround(x * 2147483648.0));
    const auto* p = reinterpret_cast<const std::uint8_t*>(&v);
    out.insert(out.end(), p, p + 4);
}

constexpr std::array<std::uint32_t, 11> kNativeAhpSourceSlots{
    0u, 1u, 2u, 4u, 5u, 7u, 8u, 9u, 10u, 13u, 14u};

unsigned ahp_bank_resource_for_rate(unsigned rate) noexcept {
    switch (rate) {
    case 32000u: return 113u;
    case 44100u: return 114u;
    case 48000u: return 103u;
    case 88200u: return 115u;
    case 96000u: return 116u;
    default: return 0u;
    }
}

template <typename ImplT>
bool configure_ahp_input_routing(ImplT& impl,
                                 const std::vector<std::uint32_t>& channel_slots,
                                 std::string& err) {
    if (channel_slots.empty() || channel_slots.size() > 14u) {
        err = "AHP channel layout must contain 1..14 physical slots";
        return false;
    }
    std::uint32_t requested_layout = 0u;
    std::array<unsigned, 16> source_slot_for_channel{};
    std::array<bool, 16> source_channel_active{};
    std::array<std::array<float, 11>, 16> folded_source_gains{};
    std::array<bool, 16> is_lfe_channel{};
    bool has_folded_inputs = false;
    for (std::size_t channel = 0u; channel < channel_slots.size(); ++channel) {
        const auto slot = channel_slots[channel];
        if (slot >= 31u) {
            err = "AHP layout contains an invalid physical slot";
            return false;
        }
        if ((requested_layout & (1u << slot)) != 0u) {
            err = "AHP channel layout contains a duplicate physical slot";
            return false;
        }
        requested_layout |= 1u << slot;
        if (slot == 3u) {
            is_lfe_channel[channel] = true;
            continue;
        }
        const auto source = std::find(kNativeAhpSourceSlots.begin(),
                                      kNativeAhpSourceSlots.end(), slot);
        if (source != kNativeAhpSourceSlots.end()) {
            source_channel_active[channel] = true;
            source_slot_for_channel[channel] =
                static_cast<unsigned>(source - kNativeAhpSourceSlots.begin());
            continue;
        }
        // Native room-0/HPV2 has no HC (slot 11) or Top (slot 12) source.
        // Preserve these Auro 13.1 channels with constant-power phantom
        // mappings into the captured height HRTFs instead of dropping them:
        // HC -> HL/HR, Top -> HL/HR/HLS/HRS.
        if (slot == 11u) {
            constexpr float kPairGain = 0.7071067811865475244f;
            folded_source_gains[channel][7] = kPairGain;
            folded_source_gains[channel][8] = kPairGain;
            has_folded_inputs = true;
            continue;
        }
        if (slot == 12u) {
            for (const unsigned height_source : {7u, 8u, 9u, 10u})
                folded_source_gains[channel][height_source] = 0.5f;
            has_folded_inputs = true;
            continue;
        }
        {
            err = "AHP layout slot has no native fixed-graph input route";
            return false;
        }
    }
    impl.channels = static_cast<unsigned>(channel_slots.size());
    impl.source_slot_for_channel = source_slot_for_channel;
    impl.source_channel_active = source_channel_active;
    impl.folded_source_gains = folded_source_gains;
    impl.is_lfe_channel = is_lfe_channel;
    impl.has_folded_ahp_inputs = has_folded_inputs;
    return true;
}

} // namespace

struct AhpBinauralRenderer::Impl {
    static constexpr std::size_t kNativeAm4hpLatencyFrames = 1056u;
    unsigned bits = 0;
    unsigned out_bits = 0;
    unsigned channels = 0;
    unsigned bytes_per_sample = 0;
    unsigned out_bytes_per_sample = 0;
    std::size_t maximum_block_frames = 0;
    float stateful_ahp_output_gain = 1.0f;
    std::array<unsigned, 16> source_slot_for_channel{};
    std::array<bool, 16> source_channel_active{};
    std::array<std::array<float, 11>, 16> folded_source_gains{};
    std::array<bool, 16> is_lfe_channel{};
    bool has_folded_ahp_inputs = false;
    HeadphoneLfeProcessing lfe_processing;
    HeadphonePcaBank pca_bank;
    HeadphoneSourceManager source_manager;
    NativePeakLimiter peak_limiter;
    // AM4HP direct-stereo dimensional 5.0.2H Core. It owns its own dynamics
    // and does not use the AHP peak limiter.
    bool am4hp_core = false;
    std::uint32_t am4hp_core_layout = 0u;
    std::array<unsigned, 16> am4hp_core_slot_for_channel{};
    std::array<bool, 16> am4hp_core_channel_active{};
    Am4hpCoreProcessor core;
    std::array<std::array<float, kNativeAm4hpLatencyFrames>, 6> am4hp_input_delay{};
    std::size_t am4hp_input_delay_pos = 0u;
    std::unique_ptr<HeadphonePcaBank> pending_pca_bank;
    std::unique_ptr<HeadphoneSourceManager> pending_source_manager;
    std::uint32_t pending_am4hp_preset = 2u;
};

AhpBinauralRenderer::AhpBinauralRenderer() = default;
AhpBinauralRenderer::~AhpBinauralRenderer() = default;
AhpBinauralRenderer::AhpBinauralRenderer(AhpBinauralRenderer&&) noexcept = default;
AhpBinauralRenderer& AhpBinauralRenderer::operator=(AhpBinauralRenderer&&) noexcept = default;

bool AhpBinauralRenderer::initialize(
    unsigned bits,
    unsigned rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    std::size_t maximum_block_frames,
    std::string& err) {
    if (channel_slots.size() != channels) {
        err = "binaural channel-slot count must exactly match PCM channels";
        return false;
    }
    std::uint32_t input_mask = 0u;
    bool unique_input_slots = true;
    for (const auto slot : channel_slots) {
        if (slot >= 31u || (input_mask & (1u << slot)) != 0u) {
            unique_input_slots = false;
            break;
        }
        input_mask |= 1u << slot;
    }
    if (room != 0u || hrtf != 0u) {
        err = "stateful AHP configure requires the captured HPV2 room-0 graph; "
              "alternate rooms and HRTF banks are not captured";
        return false;
    }
    if (bits != 16u && bits != 24u && bits != 32u) {
        err = "unsupported PCM format";
        return false;
    }
    // Native Headphones:get_remix_layout selects AM4HP for direct stereo and
    // for the dimensional 5.0.2H layout. The latter discards the two height
    // inputs and feeds the five lower planes to Core as mask 0x37.
    const std::uint32_t am4hp_core_layout = input_mask == 3u ? 3u
        : (input_mask == 0x637u ? 0x37u : 0u);
    const bool am4hp_shape = am4hp_core_layout != 0u && unique_input_slots
        && rate == 48000u && maximum_block_frames
        && (maximum_block_frames % 32u) == 0u;

    if (am4hp_shape) {
        auto am4hp = std::make_unique<Impl>();
        const unsigned bank_resource = am4hp_core_layout == 0x37u ? 106u : 104u;
        if (!am4hp->pca_bank.load_embedded_resource(bank_resource, err)
            || !am4hp->source_manager.construct(am4hp->pca_bank)) {
            if (err.empty())
                err = "embedded AM4HP source-manager graph is unavailable";
            return false;
        }
        if (!am4hp->core.construct(48000u, am4hp_core_layout, 1.0f, &err))
            return false;
        am4hp->am4hp_core = true;
        am4hp->am4hp_core_layout = am4hp_core_layout;
        am4hp->bits = bits;
        am4hp->out_bits = bits == 32u ? 24u : bits;
        am4hp->channels = channels;
        am4hp->bytes_per_sample = bits / 8u;
        am4hp->out_bytes_per_sample = am4hp->out_bits / 8u;
        am4hp->maximum_block_frames = maximum_block_frames;
        for (unsigned ch = 0u; ch != channels; ++ch) {
            const auto slot = channel_slots[ch];
            if (slot < 6u && (am4hp_core_layout & (1u << slot)) != 0u) {
                am4hp->am4hp_core_channel_active[ch] = true;
                am4hp->am4hp_core_slot_for_channel[ch] = slot;
            }
        }
        for (auto& delay : am4hp->am4hp_input_delay)
            delay.fill(0.0f);
        am4hp->am4hp_input_delay_pos = 0u;
        impl_ = std::move(am4hp);
        return true;
    }

    const unsigned ahp_bank_resource = ahp_bank_resource_for_rate(rate);
    if (ahp_bank_resource == 0u || channels < 3u || channels > 14u
        || !unique_input_slots || channel_slots.size() != channels
        || !maximum_block_frames || (maximum_block_frames % 32u) != 0u) {
        err = "stateful AHP supports captured 32/44.1/48/88.2/96 kHz HPV2 graphs, "
              "physical channel slots from mask 0x67BF (plus explicit HC/Top "
              "height folds) with slot 3 as LFE and 32-sample alignment for "
              "multichannel input; direct stereo 2.0 and dimensional 5.0.2H "
              "require 48000 Hz";
        return false;
    }

    auto stateful = std::make_unique<Impl>();
    if (!configure_ahp_input_routing(*stateful, channel_slots, err))
        return false;
    if (!stateful->pca_bank.load_embedded_resource(ahp_bank_resource, err)
        || !stateful->source_manager.construct(stateful->pca_bank)) {
        if (err.empty())
            err = "embedded stateful AHP source-manager graph is unavailable";
        return false;
    }
    std::uint32_t lfe_scale_bits = 0x3eaaae9bu;
    float lfe_scale = 0.0f;
    std::memcpy(&lfe_scale, &lfe_scale_bits, sizeof(lfe_scale));
    stateful->lfe_processing.construct(lfe_scale, true);
    stateful->lfe_processing.set_dynamic_gain(5.0f);
    stateful->peak_limiter.set_sample_rate(rate);
    // Artist Connection 1.21.31, room-0/HPV2 AHP Renderer+3680. Native
    // Renderer_process_float_32_ multiplies both output planes by this exact
    // float32 value after LFE and SourceManager accumulation.
    std::uint32_t output_gain_bits = 0x400001abu;
    std::memcpy(&stateful->stateful_ahp_output_gain,
                &output_gain_bits, sizeof(output_gain_bits));
    stateful->bits = bits;
    stateful->out_bits = bits == 32u ? 24u : bits;
    stateful->bytes_per_sample = bits / 8u;
    stateful->out_bytes_per_sample = stateful->out_bits / 8u;
    stateful->maximum_block_frames = maximum_block_frames;
    impl_ = std::move(stateful);
    return true;
}

bool AhpBinauralRenderer::process(
    const std::vector<std::uint8_t>& pcm,
    std::vector<std::uint8_t>& out,
    std::string& err) {
    if (!impl_) { err = "binaural renderer is not initialized"; return false; }
    const std::size_t frame_bytes = static_cast<std::size_t>(impl_->channels) * impl_->bytes_per_sample;
    if (!frame_bytes || pcm.size() % frame_bytes) { err = "unsupported PCM format"; return false; }
    const std::size_t frames = pcm.size() / frame_bytes;
    if (frames > impl_->maximum_block_frames) { err = "binaural block is too large"; return false; }
    if ((frames % 32u) != 0u) {
        err = "stateful AHP requires a 32-sample-aligned block";
        return false;
    }

    if (impl_->am4hp_core) {
        out.clear();
        out.reserve(frames * 2u * impl_->out_bytes_per_sample);
        for (std::size_t base = 0u; base < frames; base += 32u) {
            std::array<Am4hpCoreProcessor::Block, 6> core_input{};
            for (std::size_t frame = 0u; frame < 32u; ++frame) {
                const std::size_t delay_pos = impl_->am4hp_input_delay_pos;
                for (unsigned ch = 0u; ch != impl_->channels; ++ch) {
                    if (!impl_->am4hp_core_channel_active[ch])
                        continue;
                    const auto slot = impl_->am4hp_core_slot_for_channel[ch];
                    const auto offset = ((base + frame) * impl_->channels + ch)
                        * impl_->bytes_per_sample;
                    float current = static_cast<float>(read_sample(
                        pcm.data() + offset, impl_->bits));
                    if (impl_->bits == 32u) {
                        constexpr float q23 = 8388608.0f;
                        current = std::trunc(current * q23) / q23;
                    }
                    core_input[slot][frame] =
                        impl_->am4hp_input_delay[slot][delay_pos];
                    impl_->am4hp_input_delay[slot][delay_pos] = current;
                }
                impl_->am4hp_input_delay_pos =
                    (delay_pos + 1u) % Impl::kNativeAm4hpLatencyFrames;
            }
            Am4hpCoreProcessor::PhysicalBlocks physical{};
            for (unsigned slot = 0u; slot != core_input.size(); ++slot)
                if ((impl_->am4hp_core_layout & (1u << slot)) != 0u)
                    physical[slot] = &core_input[slot];
            Am4hpCoreProcessor::RendererInput renderer_input{};
            if (!impl_->core.prepare_renderer_input(physical, renderer_input)) {
                err = "stateful AM4HP core preparation failed";
                return false;
            }
            std::array<float, 32> renderer_left{};
            std::array<float, 32> renderer_right{};
            if (!impl_->source_manager.set_late_feedback_rt60(
                    impl_->core.hp_late_feedback_rt60())
                || !impl_->source_manager.set_early_distance_gain_db(
                    impl_->core.hp_early_distance_db())
                || !impl_->source_manager.set_late_output_gain_scale(
                    impl_->core.hp_late_output_gain_scale())
                || !impl_->source_manager.set_late_dynamic_shelf(
                    impl_->core.hp_late_shelf_coeff(),
                    impl_->core.hp_late_shelf_hz())
                || !impl_->source_manager.process_renderer_input(
                    renderer_input, renderer_left, renderer_right)) {
                err = "stateful AM4HP source-manager processing failed";
                return false;
            }
            std::array<float, 32> output_left{};
            std::array<float, 32> output_right{};
            if (!impl_->core.finish_renderer_output(
                    renderer_left, renderer_right, output_left, output_right)) {
                err = "stateful AM4HP core completion failed";
                return false;
            }
            std::uint32_t reconfigure_preset = 0u;
            if (impl_->core.take_renderer_reconfigure(reconfigure_preset)) {
                if (!impl_->pending_pca_bank
                    || !impl_->pending_source_manager
                    || impl_->pending_am4hp_preset != reconfigure_preset) {
                    err = "AM4HP preset graph was not prepared before reconfigure";
                    return false;
                }
                impl_->pca_bank = std::move(*impl_->pending_pca_bank);
                impl_->source_manager = std::move(*impl_->pending_source_manager);
                impl_->pending_pca_bank.reset();
                impl_->pending_source_manager.reset();
            }
            for (std::size_t frame = 0u; frame < 32u; ++frame) {
                write_sample(out, output_left[frame], impl_->out_bits);
                write_sample(out, output_right[frame], impl_->out_bits);
            }
        }
        return true;
    }

    out.clear();
    out.reserve(frames * 2u * impl_->out_bytes_per_sample);
    for (std::size_t base = 0u; base < frames; base += 32u) {
        std::vector<std::array<float, 32>> inputs(11u);
        std::array<float, 32> lfe_input{};
        for (std::size_t frame = 0u; frame < 32u; ++frame) {
            for (unsigned ch = 0u; ch < impl_->channels; ++ch) {
                const auto* sample = &pcm[((base + frame) * impl_->channels + ch)
                    * impl_->bytes_per_sample];
                const float value = static_cast<float>(read_sample(sample, impl_->bits));
                if (impl_->is_lfe_channel[ch]) {
                    lfe_input[frame] = value;
                } else if (!impl_->has_folded_ahp_inputs) {
                    inputs[impl_->source_slot_for_channel[ch]][frame] = value;
                } else if (impl_->source_channel_active[ch]) {
                    inputs[impl_->source_slot_for_channel[ch]][frame] += value;
                } else {
                    for (unsigned source = 0u; source < inputs.size(); ++source) {
                        const float gain = impl_->folded_source_gains[ch][source];
                        if (gain != 0.0f)
                            inputs[source][frame] += value * gain;
                    }
                }
            }
        }
        std::array<float, 32> left{};
        std::array<float, 32> right{};
        const float* lfe_planes[] = {lfe_input.data()};
        std::array<float, 32> lfe_output{};
        if (!impl_->lfe_processing.process(lfe_planes, 1u, lfe_output.data())) {
            err = "stateful AHP LFE processing failed";
            return false;
        }
        // accumulates into output +0x10, then copies
        // that complete mono block to output +0x18. Manager adds its
        // ear-specific result to both existing planes afterward.
        left = lfe_output;
        right = lfe_output;
        if (!impl_->source_manager.process(inputs, left, right)) {
            err = "stateful AHP source-manager processing failed";
            return false;
        }
        for (std::size_t frame = 0u; frame < 32u; ++frame) {
            left[frame] *= impl_->stateful_ahp_output_gain;
            right[frame] *= impl_->stateful_ahp_output_gain;
        }
        // Native Builder pipeline order is AHP Renderer -> linked stereo
        // PeakLimiter -> integer output converter.
        float* limiter_planes[] = {left.data(), right.data()};
        impl_->peak_limiter.process_planar(
            limiter_planes, 2u, 32u, false);
        for (std::size_t frame = 0u; frame < 32u; ++frame) {
            write_sample(out, left[frame], impl_->out_bits);
            write_sample(out, right[frame], impl_->out_bits);
        }
    }
    return true;
}

bool AhpBinauralRenderer::set_am4hp_core_preset(
    unsigned preset, std::string& err) {
    if (!impl_ || !impl_->am4hp_core) {
        err = "AM4HP Core preset requires an initialized AM4HP renderer";
        return false;
    }
    if (preset > 3u) {
        err = "AM4HP Core preset must be in range 0..3";
        return false;
    }
    if (preset == impl_->core.requested_preset())
        return impl_->core.set_preset(preset, &err);

    unsigned resource = 0u;
    if (preset == 0u)
        resource = impl_->am4hp_core_layout == 0x37u ? 111u : 108u;
    else if (preset == 1u)
        resource = impl_->am4hp_core_layout == 0x37u ? 112u : 109u;
    else if (preset == 2u)
        resource = impl_->am4hp_core_layout == 0x37u ? 106u : 104u;
    else if (preset == 3u && impl_->am4hp_core_layout == 3u)
        resource = 107u;
    else if (preset == 3u && impl_->am4hp_core_layout == 0x37u)
        resource = 110u;
    if (resource == 0u) {
        err = "requested AM4HP Core preset graph is not captured for this layout";
        return false;
    }

    auto bank = std::make_unique<HeadphonePcaBank>();
    auto manager = std::make_unique<HeadphoneSourceManager>();
    if (!bank->load_embedded_resource(resource, err)
        || !bank->apply_am4hp_static_source_distances(
            preset, impl_->am4hp_core_layout)
        || !manager->construct(*bank)) {
        if (err.empty())
            err = "embedded AM4HP preset source-manager graph is unavailable";
        return false;
    }
    if (!impl_->core.set_preset(preset, &err))
        return false;
    impl_->pending_pca_bank = std::move(bank);
    impl_->pending_source_manager = std::move(manager);
    impl_->pending_am4hp_preset = preset;
    return true;
}

bool render_binaural_ahp(
    const std::vector<std::uint8_t>& pcm,
    unsigned bits,
    unsigned rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    std::vector<std::uint8_t>& out,
    std::string& err,
    unsigned am4hp_core_preset,
    const ProgressFn& progress) {
    const unsigned bytes_per_sample = bits / 8u;
    const unsigned out_bytes = bits == 32u ? 3u : bytes_per_sample;
    if (!bytes_per_sample || !channels || pcm.size() % (channels * bytes_per_sample)) {
        err = "unsupported PCM format";
        return false;
    }
    const std::size_t frames = pcm.size() / (channels * bytes_per_sample);
    constexpr std::size_t kBlockFrames = 4096u;
    AhpBinauralRenderer renderer;
    if (!renderer.initialize(bits, rate, channels, channel_slots, room, hrtf,
                             kBlockFrames, err))
        return false;
    if (am4hp_core_preset != 2u
        && !renderer.set_am4hp_core_preset(am4hp_core_preset, err))
        return false;
    out.clear();
    out.reserve(frames * 2u * out_bytes);
    const std::size_t in_frame_bytes = static_cast<std::size_t>(channels) * bytes_per_sample;
    std::vector<std::uint8_t> block_in;
    std::vector<std::uint8_t> block_out;
    for (std::size_t frame = 0; frame < frames; frame += kBlockFrames) {
        const std::size_t count = std::min(kBlockFrames, frames - frame);
        const std::size_t padded = (count + 31u) & ~static_cast<std::size_t>(31u);
        block_in.assign(padded * in_frame_bytes, 0u);
        std::memcpy(block_in.data(), pcm.data() + frame * in_frame_bytes,
                    count * in_frame_bytes);
        if (!renderer.process(block_in, block_out, err))
            return false;
        const std::size_t keep = count * 2u * out_bytes;
        out.insert(out.end(), block_out.begin(),
                   block_out.begin() + static_cast<std::ptrdiff_t>(keep));
        if (progress)
            progress("encode binaural", progress_percent(frame + count, frames));
    }
    if (progress)
        progress("encode binaural", 100);
    return true;
}
} // namespace auro3d
