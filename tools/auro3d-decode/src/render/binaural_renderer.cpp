#include "binaural_renderer.hpp"
#include "peak_limiter.hpp"
#include "native_sample_convertor.hpp"
#include "auro3deng/detail/runtime_api.hpp"
#include "headphone_pca_bank.hpp"
#include "headphone_lfe_processing.hpp"
#include "headphone_source_manager.hpp"
#include "am4hp_core_processor.hpp"
#include <array>
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace auro3d {
namespace {
struct IrHeader { char magic[8]; std::uint32_t version, rate, frames, slots, perf, rooms, banks, mask, slot_ids[10]; };
using C = std::complex<double>;

void fft(std::vector<C>& a, bool inverse) {
    const std::size_t n=a.size();
    for(std::size_t i=1,j=0;i<n;i++){std::size_t b=n>>1;for(;j&b;b>>=1)j^=b;j^=b;if(i<j)std::swap(a[i],a[j]);}
    for(std::size_t len=2;len<=n;len<<=1){double ang=2.0*3.14159265358979323846/len*(inverse?-1:1);C wlen(std::cos(ang),std::sin(ang));
        for(std::size_t i=0;i<n;i+=len){C w(1);for(std::size_t j=0;j<len/2;j++){C u=a[i+j],v=a[i+j+len/2]*w;a[i+j]=u+v;a[i+j+len/2]=u-v;w*=wlen;}}}
    if(inverse)for(C& x:a)x/=double(n);
}
double read_sample(const std::uint8_t* p,unsigned bits){
    if(bits==16){std::int16_t v;std::memcpy(&v,p,2);return v/32768.0;}
    if(bits==24){std::int32_t v=(std::int32_t(p[0])|(std::int32_t(p[1])<<8)|(std::int32_t(p[2])<<16));if(v&0x800000)v|=~0xffffff;return v/8388608.0;}
    float v;std::memcpy(&v,p,4);return v;
}
void write_sample(std::vector<std::uint8_t>& out,double x,unsigned bits){
    if(bits==16){
        auto v=static_cast<std::int32_t>(std::llround(x*32768.0));
        if(v>32767)v=32767;
        if(v<-32768)v=-32768;
        const auto s=static_cast<std::int16_t>(v);
        const auto*p=reinterpret_cast<const std::uint8_t*>(&s);
        out.insert(out.end(),p,p+2);
        return;
    }
    if(bits==24){
        const auto v=native_float_to_pcm24(static_cast<float>(x));
        out.push_back(static_cast<std::uint8_t>(v&255));
        out.push_back(static_cast<std::uint8_t>((v>>8)&255));
        out.push_back(static_cast<std::uint8_t>((v>>16)&255));
        return;
    }
    auto v=static_cast<std::int32_t>(std::llround(x*2147483648.0));
    const auto*p=reinterpret_cast<const std::uint8_t*>(&v);
    out.insert(out.end(),p,p+4);
}

// Opt-in differential instrumentation.  The native AM4HP oracle records the
// two Manager output planes per 32-sample quantum; keep the portable side in
// the same raw-float form when explicitly requested by a regression run.
void write_am4hp_manager_trace(const std::array<float, 32>& left,
                               const std::array<float, 32>& right) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path)
            trace.open(path, std::ios::binary | std::ios::trunc);
    }
    if (!trace)
        return;
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    trace.write(reinterpret_cast<const char*>(left.data()),
                static_cast<std::streamsize>(sizeof(left)));
    trace.write(reinterpret_cast<const char*>(right.data()),
                static_cast<std::streamsize>(sizeof(right)));
    ++call_index;
}

void write_am4hp_manager_input_trace(
    const Am4hpCoreProcessor::RendererInput& input) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string input_path(path);
            input_path += ".input";
            trace.open(input_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace)
        return;
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    for (const auto* plane : input)
        trace.write(reinterpret_cast<const char*>(plane->data()),
                    static_cast<std::streamsize>(sizeof(*plane)));
    ++call_index;
}

void write_am4hp_manager_scores_trace(
    const std::vector<std::array<float, 32>>& first_scores,
    const std::vector<std::array<float, 32>>& second_scores) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string score_path(path);
            score_path += ".scores";
            trace.open(score_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace)
        return;
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    for (const auto& score : first_scores)
        trace.write(reinterpret_cast<const char*>(score.data()),
                    static_cast<std::streamsize>(sizeof(score)));
    for (const auto& score : second_scores)
        trace.write(reinterpret_cast<const char*>(score.data()),
                    static_cast<std::streamsize>(sizeof(score)));
    ++call_index;
}

void write_am4hp_manager_early_scores_trace(
    const std::vector<std::array<float, 32>>& first_scores,
    const std::vector<std::array<float, 32>>& second_scores) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string early_path(path);
            early_path += ".early_scores";
            trace.open(early_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace)
        return;
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    for (const auto& score : first_scores)
        trace.write(reinterpret_cast<const char*>(score.data()),
                    static_cast<std::streamsize>(sizeof(score)));
    for (const auto& score : second_scores)
        trace.write(reinterpret_cast<const char*>(score.data()),
                    static_cast<std::streamsize>(sizeof(score)));
    ++call_index;
}

void write_am4hp_manager_explicit_scores_trace(
    const std::vector<std::array<float, 32>>& first_scores,
    const std::vector<std::array<float, 32>>& second_scores) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string explicit_path(path);
            explicit_path += ".explicit_scores";
            trace.open(explicit_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace)
        return;
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    for (const auto& score : first_scores)
        trace.write(reinterpret_cast<const char*>(score.data()),
                    static_cast<std::streamsize>(sizeof(score)));
    for (const auto& score : second_scores)
        trace.write(reinterpret_cast<const char*>(score.data()),
                    static_cast<std::streamsize>(sizeof(score)));
    ++call_index;
}

void write_am4hp_manager_dynamic_trace(float late_feedback_rt60,
                                        float early_distance_db,
                                        float late_output_gain_scale,
                                        float late_shelf_coeff,
                                        float late_shelf_hz) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string dynamic_path(path);
            dynamic_path += ".dynamic";
            trace.open(dynamic_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace)
        return;
    const float early_distance_linear =
        am4hp_output_gain_linear(early_distance_db);
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    const float values[]{late_feedback_rt60, early_distance_db,
                         early_distance_linear, late_output_gain_scale,
                         late_shelf_coeff, late_shelf_hz};
    trace.write(reinterpret_cast<const char*>(values), sizeof(values));
    ++call_index;
}

void write_am4hp_core_center_trace(
    const Am4hpCoreProcessor::DebugSnapshot& snapshot) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string center_path(path);
            center_path += ".core_center";
            trace.open(center_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace)
        return;
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    const Am4hpCoreProcessor::Block* blocks[]{
        &snapshot.center_input_left, &snapshot.center_input_right,
        &snapshot.center_mutated_left, &snapshot.center_mutated_right,
        &snapshot.center_generated, &snapshot.center_scaled,
        &snapshot.center_filtered};
    for (const auto* block : blocks)
        trace.write(reinterpret_cast<const char*>(block->data()), sizeof(*block));
    trace.write(reinterpret_cast<const char*>(snapshot.center_state_before_bits.data()),
                sizeof(snapshot.center_state_before_bits));
    trace.write(reinterpret_cast<const char*>(snapshot.center_state_bits.data()),
                sizeof(snapshot.center_state_bits));
    trace.write(reinterpret_cast<const char*>(snapshot.center_control_before_bits.data()),
                sizeof(snapshot.center_control_before_bits));
    trace.write(reinterpret_cast<const char*>(snapshot.center_control_bits.data()),
                sizeof(snapshot.center_control_bits));
    trace.write(reinterpret_cast<const char*>(snapshot.center_processor_bits.data()),
                sizeof(snapshot.center_processor_bits));
    trace.write(reinterpret_cast<const char*>(snapshot.center_energy_sums.data()),
                sizeof(snapshot.center_energy_sums));
    ++call_index;
}

void write_am4hp_manager_early_gains_trace(const std::vector<float>& gains) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string gain_path(path);
            gain_path += ".early_gains";
            trace.open(gain_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace)
        return;
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    trace.write(reinterpret_cast<const char*>(gains.data()),
                static_cast<std::streamsize>(gains.size() * sizeof(float)));
    ++call_index;
}

void write_am4hp_manager_early_contributions_trace(
    const std::vector<HeadphoneSourceManager::DebugEarlyContribution>& contributions) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string contribution_path(path);
            contribution_path += ".early_contributions";
            trace.open(contribution_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace || contributions.empty())
        return;
    const std::uint32_t count = static_cast<std::uint32_t>(contributions.size());
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    trace.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& item : contributions) {
        trace.write(reinterpret_cast<const char*>(&item.native_slot), sizeof(item.native_slot));
        for (const auto& score : item.first_scores)
            trace.write(reinterpret_cast<const char*>(score.data()), sizeof(score));
        for (const auto& score : item.second_scores)
            trace.write(reinterpret_cast<const char*>(score.data()), sizeof(score));
    }
    ++call_index;
}

void write_am4hp_manager_early_ears_trace(
    const std::vector<HeadphoneSourceManager::DebugEarlyContribution>& contributions) {
    static std::ofstream trace;
    static std::uint32_t call_index = 0u;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        const char* path = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        if (path && *path) {
            std::string ear_path(path);
            ear_path += ".early_ears";
            trace.open(ear_path, std::ios::binary | std::ios::trunc);
        }
    }
    if (!trace || contributions.empty()) return;
    const std::uint32_t count = static_cast<std::uint32_t>(contributions.size());
    trace.write(reinterpret_cast<const char*>(&call_index), sizeof(call_index));
    trace.write(reinterpret_cast<const char*>(&count), sizeof(count));
    for (const auto& item : contributions) {
        trace.write(reinterpret_cast<const char*>(&item.native_slot), sizeof(item.native_slot));
        trace.write(reinterpret_cast<const char*>(item.first_ear.data()), sizeof(item.first_ear));
        trace.write(reinterpret_cast<const char*>(item.second_ear.data()), sizeof(item.second_ear));
        trace.write(reinterpret_cast<const char*>(item.first_pre_gain.data()), sizeof(item.first_pre_gain));
        trace.write(reinterpret_cast<const char*>(item.second_pre_gain.data()), sizeof(item.second_pre_gain));
    }
    ++call_index;
}
}

struct BinauralStreamRenderer::Impl {
    static constexpr std::size_t kNativeAm4hpLatencyFrames = 1056u;
    unsigned bits = 0;
    unsigned out_bits = 0;
    unsigned channels = 0;
    unsigned bytes_per_sample = 0;
    unsigned out_bytes_per_sample = 0;
    std::size_t maximum_block_frames = 0;
    std::size_t ir_frames = 0;
    std::size_t nfft = 0;
    std::vector<std::vector<C>> ir_left;
    std::vector<std::vector<C>> ir_right;
    std::vector<double> overlap_left;
    std::vector<double> overlap_right;
    NativePeakLimiter peak_limiter;
    bool stateful_ahp = false;
    bool am4hp_core = false;
    std::uint32_t am4hp_core_layout = 0u;
    float stateful_ahp_output_gain = 1.0f;
    std::array<unsigned, 16> source_slot_for_channel{};
    std::array<bool, 16> source_channel_active{};
    std::array<std::array<float, 11>, 16> folded_source_gains{};
    std::array<bool, 16> is_lfe_channel{};
    bool has_folded_ahp_inputs = false;
    std::array<unsigned, 16> am4hp_core_slot_for_channel{};
    std::array<bool, 16> am4hp_core_channel_active{};
    unsigned lfe_channel = 3u;
    HeadphoneLfeProcessing lfe_processing;
    HeadphonePcaBank pca_bank;
    HeadphoneSourceManager source_manager;
    std::unique_ptr<HeadphonePcaBank> pending_pca_bank;
    std::unique_ptr<HeadphoneSourceManager> pending_source_manager;
    std::uint32_t pending_am4hp_preset = 2u;
    bool manager_trace_after_preset = false;
    Am4hpCoreProcessor core;
    std::array<std::array<float, kNativeAm4hpLatencyFrames>, 6>
        am4hp_input_delay{};
    std::size_t am4hp_input_delay_pos = 0u;
    unsigned configured_rate = 0u;
    unsigned configured_room = 0u;
    unsigned configured_hrtf = 0u;
    std::vector<std::uint32_t> configured_slots;
    bool reference_ir = false;
};

namespace {
constexpr std::uint32_t kNativeAhpLayout = 0x67BFu;
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
    impl.configured_slots = channel_slots;
    return true;
}
}

BinauralStreamRenderer::BinauralStreamRenderer() = default;
BinauralStreamRenderer::~BinauralStreamRenderer() = default;
BinauralStreamRenderer::BinauralStreamRenderer(BinauralStreamRenderer&&) noexcept = default;
BinauralStreamRenderer& BinauralStreamRenderer::operator=(BinauralStreamRenderer&&) noexcept = default;

void BinauralStreamRenderer::reset_audio_state() noexcept {
    if (!impl_)
        return;
    if (impl_->stateful_ahp) {
        if (impl_->am4hp_core) {
            impl_->source_manager.reset_audio_state();
            impl_->core.reset_audio_state();
            impl_->pending_pca_bank.reset();
            impl_->pending_source_manager.reset();
            for (auto& delay : impl_->am4hp_input_delay)
                delay.fill(0.0f);
            impl_->am4hp_input_delay_pos = 0u;
            return;
        }
        impl_->lfe_processing.reset_audio_state();
        impl_->source_manager.reset_audio_state();
        impl_->peak_limiter.reset();
        return;
    }
    std::fill(impl_->overlap_left.begin(), impl_->overlap_left.end(), 0.0);
    std::fill(impl_->overlap_right.begin(), impl_->overlap_right.end(), 0.0);
    impl_->peak_limiter.reset();
}

bool BinauralStreamRenderer::set_lfe_dynamic_gain(float gain) noexcept {
    if (!impl_ || !impl_->stateful_ahp || !std::isfinite(gain))
        return false;
    // The captured AM4HP stereo Core has no active LFE plane.  Do not report
    // success for a parameter that this production graph cannot consume.
    if (impl_->am4hp_core)
        return false;
    impl_->lfe_processing.set_dynamic_gain(gain);
    return true;
}

bool BinauralStreamRenderer::set_am4hp_core_preset(
    unsigned preset, std::string& err) {
    if (!impl_ || !impl_->stateful_ahp || !impl_->am4hp_core) {
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
    // Keep the opt-in Manager trace scoped to a real preset update.  The
    // regression executable creates several unrelated AHP/AM4HP renderers;
    // mixing them into one raw trace makes native-call alignment ambiguous.
    if (const char* trace = std::getenv("ORUA_AM4HP_MANAGER_TRACE_PATH");
        trace && *trace) {
        impl_->manager_trace_after_preset = true;
        impl_->source_manager.set_debug_capture_early_scores(true);
        manager->set_debug_capture_early_scores(true);
    }
    impl_->pending_pca_bank = std::move(bank);
    impl_->pending_source_manager = std::move(manager);
    impl_->pending_am4hp_preset = preset;
    return true;
}

bool BinauralStreamRenderer::reconfigure(
    unsigned sample_rate,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    bool reset_audio,
    std::string& err) {
    if (!impl_) {
        err = "binaural renderer is not initialized";
        return false;
    }
    if (impl_->reference_ir) {
        err = "reference IR path has no native AHP configure/reconfigure owner";
        return false;
    }
    if (!impl_->stateful_ahp) {
        err = "stateful AHP/AM4HP reconfigure requires a stateful renderer";
        return false;
    }
    if (room != impl_->configured_room || hrtf != impl_->configured_hrtf) {
        err = "AHP configure requires the captured HPV2 room-0 graph; "
              "alternate rooms and HRTF banks are not captured";
        return false;
    }
    if (impl_->am4hp_core) {
        // AM4HP selection and its Core layout are owned above the AHP
        // Renderer::configure path. Do not silently turn an existing AM4HP
        // instance into the fixed 0x67BF AHP graph.
        if (sample_rate != impl_->configured_rate) {
            err = "AM4HP rate reconfigure has no captured Core graph";
            return false;
        }
        if (channel_slots != impl_->configured_slots) {
            err = "AM4HP reconfigure cannot change the selected Core layout";
            return false;
        }
    } else {
        const unsigned resource = ahp_bank_resource_for_rate(sample_rate);
        if (resource == 0u) {
            err = "AHP rate is rejected by the captured native v4 graph set";
            return false;
        }
        if (sample_rate != impl_->configured_rate) {
            HeadphonePcaBank bank;
            HeadphoneSourceManager manager;
            if (!bank.load_embedded_resource(resource, err)
                || !manager.construct(bank)) {
                if (err.empty())
                    err = "embedded rate-specific AHP graph is unavailable";
                return false;
            }
            if (!configure_ahp_input_routing(*impl_, channel_slots, err))
                return false;
            impl_->pca_bank = std::move(bank);
            impl_->source_manager = std::move(manager);
            impl_->configured_rate = sample_rate;
            impl_->peak_limiter.set_sample_rate(sample_rate);
        } else if (channel_slots != impl_->configured_slots
                   && !configure_ahp_input_routing(*impl_, channel_slots, err)) {
            return false;
        }
    }
    if (reset_audio)
        reset_audio_state();
    return true;
}

bool BinauralStreamRenderer::set_headphone_user_preset(
    unsigned preset, std::string& err) {
    if (!impl_ || !impl_->stateful_ahp || impl_->reference_ir) {
        err = "headphone user preset configure requires stateful AHP/AM4HP";
        return false;
    }
    if (impl_->am4hp_core) {
        err = "AM4HP user-preset configure is the Core preset path; "
              "use set_am4hp_core_preset";
        return false;
    }
    // Captured AHP production graph is DEFAULT (0). Native AHP::update is a
    // no-op; other user presets belong to Configurator/reconfigure tables
    // that are not in the current HPV2 room-0 resource.
    if (preset != 0u) {
        err = "AHP headphone user preset configure is not captured for this profile";
        return false;
    }
    return true;
}

bool BinauralStreamRenderer::initialize(
    unsigned bits,
    unsigned rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    std::size_t maximum_block_frames,
    std::string& err) {
    return initialize_mode(bits, rate, channels, channel_slots, room, hrtf,
                           maximum_block_frames, err, false);
}

bool BinauralStreamRenderer::initialize_reference_ir(
    unsigned bits,
    unsigned rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    std::size_t maximum_block_frames,
    std::string& err) {
    return initialize_mode(bits, rate, channels, channel_slots, room, hrtf,
                           maximum_block_frames, err, true);
}

bool BinauralStreamRenderer::initialize_mode(
    unsigned bits,
    unsigned rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    std::size_t maximum_block_frames,
    std::string& err,
    bool reference_ir) {
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
    // Native Headphones::get_remix_layout selects AM4HP for direct stereo
    // and for the dimensional 5.0.2H layout. The latter discards the two
    // height inputs and feeds the five lower planes to Core as mask 0x37.
    const std::uint32_t am4hp_core_layout = input_mask == 3u ? 3u
        : (input_mask == 0x637u ? 0x37u : 0u);
    const bool am4hp_shape = !reference_ir && am4hp_core_layout != 0u
        && unique_input_slots && rate == 48000u && hrtf == 0u && room == 0u
        && maximum_block_frames && (maximum_block_frames % 32u) == 0u;
    const unsigned ahp_bank_resource = ahp_bank_resource_for_rate(rate);
    if (!reference_ir && !am4hp_shape && (room != 0u || hrtf != 0u)) {
        err = "stateful AHP configure requires the captured HPV2 room-0 graph; "
              "alternate rooms and HRTF banks are not captured";
        return false;
    }
    if (!reference_ir && !am4hp_shape && ((bits != 16u && bits != 24u && bits != 32u)
        || ahp_bank_resource == 0u || channels == 0u || channels > 14u
        || (channels == 2u && !am4hp_shape) || hrtf != 0u
        || room != 0u
        || channel_slots.size() != channels || !maximum_block_frames
        || (maximum_block_frames % 32u) != 0u)) {
        err = "stateful AHP supports captured 32/44.1/48/88.2/96 kHz HPV2 graphs, "
              "physical channel slots from mask 0x67BF (plus explicit HC/Top "
              "height folds) with slot 3 as LFE and "
              "32-sample alignment; use "
              "--binaural-reference-ir for the finite reference path";
        return false;
    }
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
        am4hp->stateful_ahp = true;
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
        am4hp->configured_rate = rate;
        am4hp->configured_room = room;
        am4hp->configured_hrtf = hrtf;
        am4hp->configured_slots = channel_slots;
        am4hp->reference_ir = false;
        impl_ = std::move(am4hp);
        return true;
    }
    if (!reference_ir) {
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
        // Artist Connection 1.21.31, room-0/HPV2/48-kHz AHP Renderer+3680.
        // Native Renderer_process_float_32_ multiplies both output planes by
        // this exact float32 value after LFE and SourceManager accumulation.
        std::uint32_t output_gain_bits = 0x400001abu;
        std::memcpy(&stateful->stateful_ahp_output_gain,
                    &output_gain_bits, sizeof(output_gain_bits));
        stateful->stateful_ahp = true;
        stateful->bits = bits;
        stateful->out_bits = bits == 32u ? 24u : bits;
        stateful->bytes_per_sample = bits / 8u;
        stateful->out_bytes_per_sample = stateful->out_bits / 8u;
        stateful->maximum_block_frames = maximum_block_frames;
        stateful->configured_rate = rate;
        stateful->configured_room = room;
        stateful->configured_hrtf = hrtf;
        stateful->reference_ir = false;
        impl_ = std::move(stateful);
        return true;
    }

    HRSRC rs = FindResourceW(nullptr, MAKEINTRESOURCEW(102), MAKEINTRESOURCEW(10));
    if (!rs) { err = "binaural IR resource not found"; return false; }
    HGLOBAL hg = LoadResource(nullptr, rs);
    const auto* data = static_cast<const std::uint8_t*>(LockResource(hg));
    const std::size_t size = SizeofResource(nullptr, rs);
    if (!data || size < sizeof(IrHeader)) { err = "invalid binaural IR resource"; return false; }
    const auto* h = reinterpret_cast<const IrHeader*>(data);
    if (std::memcmp(h->magic, "AUROHPIR", 8) || h->version != 1 ||
        h->rate != 48000 || rate != 48000) {
        err = "binaural mode currently requires 48000 Hz";
        return false;
    }
    if (room >= h->rooms) { err = "invalid room preset"; return false; }
    const std::size_t expected_floats = static_cast<std::size_t>(h->banks)
        * h->rooms * h->perf * h->slots * h->frames * 2u;
    if (!h->frames || h->frames % 32u || !h->perf || h->slots > 10u
        || expected_floats > (size - sizeof(IrHeader)) / sizeof(float)) {
        err = "invalid binaural IR dimensions"; return false;
    }
    if ((bits != 16 && bits != 24 && bits != 32) || !channels ||
        channel_slots.size() < channels || !maximum_block_frames) {
        err = "unsupported PCM format";
        return false;
    }
    if (hrtf != 0u && hrtf != 2u) {
        err = "bundled binaural IR supports only HRTF preset 0=HPV2 or 2=GENERIC_2";
        return false;
    }
    const unsigned bank = hrtf == 0u ? 0u : 1u;
    if (bank >= h->banks) {
        err = "requested HRTF bank is absent from the binaural IR resource";
        return false;
    }

    auto impl = std::make_unique<Impl>();
    impl->bits = bits;
    impl->out_bits = bits == 32u ? 24u : bits;
    impl->channels = channels;
    impl->bytes_per_sample = bits / 8;
    impl->out_bytes_per_sample = impl->out_bits / 8;
    impl->maximum_block_frames = maximum_block_frames;
    impl->configured_rate = rate;
    impl->configured_room = room;
    impl->configured_hrtf = hrtf;
    impl->configured_slots = channel_slots;
    impl->reference_ir = true;
    impl->ir_frames = h->frames;
    impl->nfft = 1;
    while (impl->nfft < maximum_block_frames + h->frames - 1u)
        impl->nfft <<= 1u;
    impl->ir_left.resize(channels);
    impl->ir_right.resize(channels);
    impl->overlap_left.assign(h->frames ? h->frames - 1u : 0u, 0.0);
    impl->overlap_right.assign(h->frames ? h->frames - 1u : 0u, 0.0);

    const float* base = reinterpret_cast<const float*>(data + sizeof(IrHeader));
    std::uint32_t reference_input_mask = 0u;
    for (unsigned ch = 0; ch < channels; ++ch) {
        if (channel_slots[ch] >= 31u
            || (reference_input_mask & (1u << channel_slots[ch]))) {
            err = "invalid binaural channel layout"; return false;
        }
        reference_input_mask |= 1u << channel_slots[ch];
    }
    alignas(8) std::array<std::int32_t, 160> plan{};
    alignas(8) std::array<std::int32_t, 160> routes{};
    alignas(8) std::array<std::int32_t, 16> engine{};
    auro3deng::auro_matic_v3_downmix_engine_t_construct(
        reinterpret_cast<std::uint8_t*>(engine.data()));
    if (!auro3deng::auro_matic_v3_downmix_engine_calculate(
            engine.data(), plan.data(), reference_input_mask, h->mask)
        || !auro3deng::auro_matic_v3_downmix_plan_construct(
            plan.data(), routes.data(), auro3deng::auro_downmix_v1_gain_table_default())) {
        err = "unsupported binaural remix layout"; return false;
    }
    float matrix[31][31]{};
    std::uint64_t route_count = 0;
    std::memcpy(&route_count, routes.data(), sizeof(route_count));
    for (std::uint64_t row = 0; row < route_count; ++row) {
        const auto* route = routes.data() + 2u + row * 3u;
        float gain;
        std::memcpy(&gain, route + 2u, sizeof(gain));
        if (route[0] < 0 || route[0] >= 31 || route[1] < 0 || route[1] >= 31) {
            err = "invalid binaural remix route"; return false;
        }
        matrix[route[0]][route[1]] += gain;
    }
    const std::size_t ir_stride = static_cast<std::size_t>(h->frames) * 2u;
    constexpr std::size_t perf_index = 0;
    std::vector<C> ir(impl->nfft);
    for (unsigned ch = 0; ch < channels; ++ch) {
        for (unsigned ear = 0; ear < 2; ++ear) {
            std::fill(ir.begin(), ir.end(), C{});
            bool routed = false;
            for (unsigned s = 0; s < h->slots; ++s) {
                if (h->slot_ids[s] >= 31u) { err = "invalid IR slot"; return false; }
                const float gain = matrix[channel_slots[ch]][h->slot_ids[s]];
                if (gain == 0.0f) continue;
                routed = true;
                const std::size_t index = (((static_cast<std::size_t>(bank) * h->rooms + room)
                    * h->perf + perf_index) * h->slots + s) * ir_stride;
                // Preserve the captured response; do not apply an additional
                // window. Extending this diagnostic tail requires a longer
                // reference capture; this path is not the stateful AHP model.
                for (unsigned frame = 0; frame < h->frames; ++frame) {
                    const unsigned block = frame / 32u;
                    const unsigned position = frame % 32u;
                    ir[frame] += double(base[index + block * 64u + ear * 32u + position]) * gain;
                }
            }
            if (!routed) { err = "binaural remix leaves an input channel unrouted"; return false; }
            fft(ir, false);
            (ear ? impl->ir_right[ch] : impl->ir_left[ch]) = ir;
        }
    }
    impl_ = std::move(impl);
    return true;
}

bool BinauralStreamRenderer::process(
    const std::vector<std::uint8_t>& pcm,
    std::vector<std::uint8_t>& out,
    std::string& err) {
    if (!impl_) { err = "binaural renderer is not initialized"; return false; }
    const std::size_t frame_bytes = static_cast<std::size_t>(impl_->channels) * impl_->bytes_per_sample;
    if (!frame_bytes || pcm.size() % frame_bytes) { err = "unsupported PCM format"; return false; }
    const std::size_t frames = pcm.size() / frame_bytes;
    if (frames > impl_->maximum_block_frames) { err = "binaural block is too large"; return false; }

    if (impl_->stateful_ahp) {
        if ((frames % 32u) != 0u) {
            err = "stateful AHP requires a 32-sample-aligned block";
            return false;
        }
        out.clear();
        out.reserve(frames * 2u * impl_->out_bytes_per_sample);
        if (impl_->am4hp_core) {
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
                if (impl_->manager_trace_after_preset) {
                    write_am4hp_core_center_trace(impl_->core.debug_snapshot());
                    write_am4hp_manager_input_trace(renderer_input);
                    write_am4hp_manager_trace(renderer_left, renderer_right);
                    write_am4hp_manager_scores_trace(
                        impl_->source_manager.first_scores(),
                        impl_->source_manager.second_scores());
                    write_am4hp_manager_early_scores_trace(
                        impl_->source_manager.debug_early_first_scores(),
                        impl_->source_manager.debug_early_second_scores());
                    write_am4hp_manager_explicit_scores_trace(
                        impl_->source_manager.debug_explicit_first_scores(),
                        impl_->source_manager.debug_explicit_second_scores());
                    write_am4hp_manager_dynamic_trace(
                        impl_->core.hp_late_feedback_rt60(),
                        impl_->core.hp_early_distance_db(),
                        impl_->core.hp_late_output_gain_scale(),
                        impl_->core.hp_late_shelf_coeff(),
                        impl_->core.hp_late_shelf_hz());
                    write_am4hp_manager_early_gains_trace(
                        impl_->source_manager.debug_early_distance_gains());
                    write_am4hp_manager_early_contributions_trace(
                        impl_->source_manager.debug_early_contributions());
                    write_am4hp_manager_early_ears_trace(
                        impl_->source_manager.debug_early_contributions());
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
                    impl_->source_manager =
                        std::move(*impl_->pending_source_manager);
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
                err = "stateful AHP source-manager processing failed";
                return false;
            }
            // IDA 0x56F320..0x56F561 accumulates into output +0x10, then
            // copies that complete mono block to output +0x18. Manager adds
            // its ear-specific result to both existing planes afterward.
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
            // PeakLimiter -> integer output converter. AM4HP is intentionally
            // excluded above because its Core owns the corresponding dynamics.
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

    std::vector<C> sum_left(impl_->nfft), sum_right(impl_->nfft), input(impl_->nfft);
    for (unsigned ch = 0; ch < impl_->channels; ++ch) {
        if (impl_->ir_left[ch].empty())
            continue;
        std::fill(input.begin(), input.end(), C{});
        for (std::size_t frame = 0; frame < frames; ++frame)
            input[frame] = read_sample(&pcm[(frame * impl_->channels + ch) * impl_->bytes_per_sample], impl_->bits);
        fft(input, false);
        for (std::size_t k = 0; k < impl_->nfft; ++k) {
            sum_left[k] += input[k] * impl_->ir_left[ch][k];
            sum_right[k] += input[k] * impl_->ir_right[ch][k];
        }
    }
    fft(sum_left, true);
    fft(sum_right, true);

    // Native pipeline: AHP then PeakLimiter then output_convertor. Keep
    // compressor state across FFT blocks.
    impl_->peak_limiter.set_sample_rate(48000u);

    out.clear();
    out.reserve(frames * 2u * impl_->out_bytes_per_sample);
    std::vector<double> left_td(frames);
    std::vector<double> right_td(frames);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        const double old_left = frame < impl_->overlap_left.size() ? impl_->overlap_left[frame] : 0.0;
        const double old_right = frame < impl_->overlap_right.size() ? impl_->overlap_right[frame] : 0.0;
        left_td[frame] = sum_left[frame].real() + old_left;
        right_td[frame] = sum_right[frame].real() + old_right;
    }
    impl_->peak_limiter.process_stereo(left_td.data(), right_td.data(), frames);
    for (std::size_t frame = 0; frame < frames; ++frame) {
        write_sample(out, left_td[frame], impl_->out_bits);
        write_sample(out, right_td[frame], impl_->out_bits);
    }
    std::vector<double> next_left(impl_->overlap_left.size(), 0.0);
    std::vector<double> next_right(impl_->overlap_right.size(), 0.0);
    for (std::size_t k = 0; k < next_left.size(); ++k) {
        const std::size_t position = frames + k;
        if (position < impl_->overlap_left.size()) {
            next_left[k] += impl_->overlap_left[position];
            next_right[k] += impl_->overlap_right[position];
        }
        if (position < impl_->nfft) {
            next_left[k] += sum_left[position].real();
            next_right[k] += sum_right[position].real();
        }
    }
    impl_->overlap_left = std::move(next_left);
    impl_->overlap_right = std::move(next_right);
    return true;
}

bool render_binaural_from_embedded_ir(
    const std::vector<std::uint8_t>& pcm,
    unsigned bits,
    unsigned rate,
    unsigned channels,
    const std::vector<std::uint32_t>& channel_slots,
    unsigned room,
    unsigned hrtf,
    std::vector<std::uint8_t>& out,
    std::string& err,
    const ProgressFn& progress) {
    const unsigned bytes_per_sample = bits / 8u;
    const unsigned out_bytes = bits == 32u ? 3u : bytes_per_sample;
    if (!bytes_per_sample || !channels || pcm.size() % (channels * bytes_per_sample)) {
        err = "unsupported PCM format";
        return false;
    }
    const std::size_t frames = pcm.size() / (channels * bytes_per_sample);
    constexpr std::size_t kBlockFrames = 4096u;
    const std::size_t block_frames = std::min(kBlockFrames, frames ? frames : kBlockFrames);
    BinauralStreamRenderer renderer;
    if (!renderer.initialize_reference_ir(
            bits, rate, channels, channel_slots, room, hrtf,
            block_frames, err))
        return false;
    out.clear();
    out.reserve(frames * 2u * out_bytes);
    const std::size_t in_frame_bytes = static_cast<std::size_t>(channels) * bytes_per_sample;
    std::vector<std::uint8_t> block_out;
    for (std::size_t frame = 0; frame < frames; frame += block_frames) {
        const std::size_t count = std::min(block_frames, frames - frame);
        const std::uint8_t* begin = pcm.data() + frame * in_frame_bytes;
        const std::vector<std::uint8_t> block_in(begin, begin + count * in_frame_bytes);
        if (!renderer.process(block_in, block_out, err))
            return false;
        out.insert(out.end(), block_out.begin(), block_out.end());
        if (progress)
            progress("encode binaural", progress_percent(frame + count, frames));
    }
    if (progress)
        progress("encode binaural", 100);
    return true;
}
}
