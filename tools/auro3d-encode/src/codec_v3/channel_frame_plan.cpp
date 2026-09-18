#include "channel_frame_plan.hpp"

#include "crc.hpp"
#include "golomb_rice.hpp"
#include "metadata_block.hpp"
#include "mix_mix3.hpp"
#include "pcm_metadata.hpp"

#include <limits>
#include <utility>

namespace auro3d::encode {
namespace {

bool locate_projector_mask(
    std::uint64_t position,
    std::uint32_t bitlines,
    std::size_t sample_count,
    PcmMetadataBitLocation& out) {
    out = {};
    if (bitlines < 3u || bitlines > 16u)
        return false;
    if (position < 16u) {
        out.sample_offset = static_cast<std::size_t>(position);
        out.bit_mask = 1u;
    } else if (position < 32u) {
        out.sample_offset = static_cast<std::size_t>(position - 16u);
        out.bit_mask = 2u;
    } else if (position < 48u) {
        out.sample_offset = static_cast<std::size_t>(position - 32u);
        out.bit_mask = 4u;
    } else {
        std::uint64_t tail = position - 48u;
        const std::uint64_t first_region =
            16u * static_cast<std::uint64_t>(bitlines - 3u);
        std::uint32_t line = 0u;
        if (tail < first_region) {
            const std::uint32_t stride = bitlines - 3u;
            out.sample_offset =
                static_cast<std::size_t>(tail / stride);
            line = bitlines - 1u
                - static_cast<std::uint32_t>(tail % stride);
        } else {
            tail -= first_region;
            const std::uint64_t cycle_width =
                16u * static_cast<std::uint64_t>(bitlines) - 1u;
            const std::uint64_t cycle = tail / cycle_width;
            std::uint64_t within = tail % cycle_width;
            if (cycle > (std::numeric_limits<std::size_t>::max() - 16u) / 16u)
                return false;
            const std::size_t first_sample =
                16u + static_cast<std::size_t>(16u * cycle);
            if (within < bitlines - 1u) {
                out.sample_offset = first_sample;
                line = bitlines - 1u
                    - static_cast<std::uint32_t>(within);
            } else {
                within -= bitlines - 1u;
                out.sample_offset = first_sample + 1u
                    + static_cast<std::size_t>(within / bitlines);
                line = bitlines - 1u
                    - static_cast<std::uint32_t>(within % bitlines);
            }
        }
        out.bit_mask = std::uint32_t{1} << line;
    }
    return out.sample_offset < sample_count;
}

bool project_channel_payload(
    const EncodedChannelFrame& frame,
    std::vector<std::int32_t>& plane,
    std::string& error) {
    const std::uint32_t low_mask =
        (std::uint32_t{1} << frame.quantization_shift) - 1u;
    for (const std::int32_t sample : plane) {
        if ((static_cast<std::uint32_t>(sample) & low_mask) != 0u) {
            error = "prepared carrier has occupied codec-v3 mux bits";
            return false;
        }
    }
    if (plane.size() < 16u) {
        error = "codec-v3 channel projector requires sixteen samples";
        return false;
    }
    // Projector serialization reserves masks 0..15 for the fixed sync
    // sentinel and masks 16..31 for the CRC. The false iterator opens at
    // mask 32, where the sixteen-bit fixed A3D header begins.
    for (std::size_t sample = 0u; sample < 16u; ++sample) {
        std::uint32_t raw = static_cast<std::uint32_t>(plane[sample]);
        raw = (raw & ~7u) | 1u;
        plane[sample] = static_cast<std::int32_t>(raw);
    }
    PcmMetadataFalseWriter writer;
    if (!writer.open(
            plane, 0u, plane.size(), frame.quantization_shift)) {
        error = "could not open codec-v3 channel projector";
        return false;
    }
    const std::uint32_t frame_code =
        (static_cast<std::uint32_t>(plane.size()) >> 4u) - 1u;
    const std::uint32_t flags = frame.a3d_config_flag ? 3u : 2u;
    if (!writer.write_unsigned(frame_code, 8u)
        || !writer.write_unsigned(flags, 4u)
        || !writer.write_unsigned(
            14u - frame.quantization_shift, 4u)) {
        error = "could not project codec-v3 fixed A3D header";
        return false;
    }
    std::uint64_t payload_capacity = 0u;
    if (!codec_v3_channel_payload_capacity(
            frame.quantization_shift,
            static_cast<std::uint32_t>(plane.size()),
            payload_capacity)
        || writer.bits_remaining() != payload_capacity) {
        error = "codec-v3 false-projector capacity differs from its mask table";
        return false;
    }
    std::uint64_t written = 0u;
    for (std::size_t sample = 0u;
         sample < frame.words.size() && written < frame.serialized_bits;
         ++sample) {
        const std::uint32_t reserved = sample < 16u
            ? 3u
            : ((sample & 15u) == 0u ? 1u : 0u);
        const std::uint32_t raw =
            static_cast<std::uint32_t>(frame.words[sample]);
        for (std::uint32_t line = frame.quantization_shift;
             line-- > reserved && written < frame.serialized_bits;) {
            if (!writer.write_bool(((raw >> line) & 1u) != 0u)) {
                error = "codec-v3 channel payload exceeds false-projector capacity";
                return false;
            }
            ++written;
        }
    }
    if (written != frame.serialized_bits) {
        error = "codec-v3 channel scratch stream is shorter than its bit count";
        return false;
    }
    return true;
}

bool validate_projected_channel_payload(
    const EncodedChannelFrame& frame,
    const std::vector<std::int32_t>& plane,
    std::string& error) {
    const std::uint32_t frame_code =
        (static_cast<std::uint32_t>(plane.size()) >> 4u) - 1u;
    const std::uint32_t flags = frame.a3d_config_flag ? 3u : 2u;
    const std::uint32_t fixed_header =
        (frame_code << 8u)
        | (flags << 4u)
        | (14u - frame.quantization_shift);
    for (std::uint32_t bit = 0u; bit < 16u; ++bit) {
        PcmMetadataBitLocation location{};
        if (!locate_projector_mask(
                32u + bit,
                frame.quantization_shift,
                plane.size(),
                location)) {
            error = "codec-v3 fixed A3D header has no projector mask";
            return false;
        }
        const bool actual =
            (static_cast<std::uint32_t>(plane[location.sample_offset])
                & location.bit_mask) != 0u;
        const bool expected =
            ((fixed_header >> (15u - bit)) & 1u) != 0u;
        if (actual != expected) {
            error = "codec-v3 fixed A3D header differs after projection";
            return false;
        }
    }

    std::uint64_t position = 48u;
    std::uint64_t compared = 0u;
    for (std::size_t sample = 0u;
         sample < frame.words.size() && compared < frame.serialized_bits;
         ++sample) {
        const std::uint32_t reserved = sample < 16u
            ? 3u
            : ((sample & 15u) == 0u ? 1u : 0u);
        const std::uint32_t raw =
            static_cast<std::uint32_t>(frame.words[sample]);
        for (std::uint32_t line = frame.quantization_shift;
             line-- > reserved && compared < frame.serialized_bits;) {
            PcmMetadataBitLocation projector_location{};
            PcmMetadataBitLocation false_location{};
            if (!locate_projector_mask(
                    position,
                    frame.quantization_shift,
                    plane.size(),
                    projector_location)
                || !locate_pcm_metadata_bit_false(
                    position,
                    frame.quantization_shift,
                    16u * frame.quantization_shift - 1u,
                    plane.size(),
                    false_location)
                || projector_location.sample_offset
                    != false_location.sample_offset
                || projector_location.bit_mask
                    != false_location.bit_mask) {
                error = "codec-v3 false iterator differs from Projector mask order";
                return false;
            }
            const bool actual =
                (static_cast<std::uint32_t>(
                    plane[projector_location.sample_offset])
                    & projector_location.bit_mask) != 0u;
            const bool expected = ((raw >> line) & 1u) != 0u;
            if (actual != expected) {
                error = "codec-v3 Channel stream differs after projection";
                return false;
            }
            ++position;
            ++compared;
        }
    }
    if (compared != frame.serialized_bits) {
        error = "codec-v3 projected Channel stream is shorter than declared";
        return false;
    }
    return true;
}

bool project_channel_crc(
    std::vector<std::int32_t>& plane,
    std::string& error) {
    if (plane.size() < 16u) {
        error = "codec-v3 channel CRC requires sixteen samples";
        return false;
    }
    std::vector<std::uint32_t> words;
    words.reserve(plane.size());
    for (std::size_t sample = 0u; sample < plane.size(); ++sample) {
        std::uint32_t raw = static_cast<std::uint32_t>(plane[sample]);
        if (sample < 16u)
            raw &= ~2u;
        words.push_back(raw);
    }
    std::uint16_t crc = 0u;
    if (!compute_pcm_mux_crc16(words, crc, error))
        return false;
    for (std::size_t sample = 0u; sample < 16u; ++sample) {
        std::uint32_t raw = words[sample];
        if ((crc & (std::uint16_t{1} << (15u - sample))) != 0u)
            raw |= 2u;
        plane[sample] = static_cast<std::int32_t>(raw);
    }
    return true;
}

} // namespace

bool build_direct_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    std::vector<EncodedChannelFrame>& out,
    std::string& error) {
    error.clear();
    out.clear();
    if (frame_count == 0u) {
        error = "direct channel-frame plan has an empty unit span";
        return false;
    }
    out.reserve(groups.size());
    for (const AnalyzedEncodeGroup& group : groups) {
        if (!group.carrier_ready || group.analysis_arity != 1u
            || group.analysis_source_ids.size() != 1u
            || group.carrier.samples.size() != frame_count) {
            error = "direct channel-frame plan contains a mixed or incomplete group";
            out.clear();
            return false;
        }
        EncodedChannelFrame frame{};
        if (!encode_direct_channel_frame(
                group.carrier.carrier_channel_id,
                group.carrier.quantization_shift,
                kCodecV3SerializedChannelHeader,
                group.analysis_source_ids.front(),
                group.has_scaler_ix,
                group.scaler_ix,
                {},
                {},
                {},
                frame,
                error)
            || !finalize_channel_frame_span(frame_count, frame, error)) {
            out.clear();
            return false;
        }
        out.push_back(std::move(frame));
    }
    return true;
}

bool build_analyzed_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    std::vector<EncodedChannelFrame>& out,
    std::string& error) {
    return build_analyzed_channel_frames(
        groups, frame_count, nullptr, {}, kCodecV3ChannelCount, {}, out, error);
}

bool build_analyzed_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    std::vector<EncodedChannelFrame>& out,
    std::string& error) {
    return build_analyzed_channel_frames(
        groups,
        frame_count,
        input_scaler_indices,
        {},
        kCodecV3ChannelCount,
        {},
        out,
        error);
}

bool build_analyzed_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    const std::vector<AdolInstruction>& common_adol,
    std::uint32_t optional_adol_channel,
    const std::vector<AdolInstruction>& optional_adol,
    std::vector<EncodedChannelFrame>& out,
    std::string& error) {
    std::array<std::vector<AdolInstruction>, 31> optional_by_channel{};
    if (optional_adol_channel < optional_by_channel.size())
        optional_by_channel[optional_adol_channel] = optional_adol;
    else if (!optional_adol.empty()) {
        error = "optional ADOL channel is outside codec-v3 range";
        return false;
    }
    return build_analyzed_channel_frames(
        groups,
        frame_count,
        input_scaler_indices,
        common_adol,
        optional_by_channel,
        out,
        error);
}

bool build_analyzed_channel_frames(
    const std::vector<AnalyzedEncodeGroup>& groups,
    std::uint32_t frame_count,
    const std::array<std::uint8_t, 31>* input_scaler_indices,
    const std::vector<AdolInstruction>& common_adol,
    const std::array<std::vector<AdolInstruction>, 31>& optional_adol_by_channel,
    std::vector<EncodedChannelFrame>& out,
    std::string& error) {
    error.clear();
    out.clear();
    if (frame_count == 0u) {
        error = "analyzed channel-frame plan has an empty unit span";
        return false;
    }
    out.reserve(groups.size());
    for (const AnalyzedEncodeGroup& group : groups) {
        if (!group.carrier_ready
            || group.carrier.carrier_channel_id >= kCodecV3ChannelCount
            || group.carrier.samples.size() != frame_count
            || group.analysis_source_ids.size() != group.analysis_arity
            || group.analysis_arity == 0u
            || group.analysis_arity > 3u) {
            error = "analyzed channel-frame plan contains an incomplete group";
            out.clear();
            return false;
        }
        std::vector<PrimaryDownmixGain> primary_downmix;
        if (!collect_primary_downmix_gains(
                group.analysis_source_ids,
                input_scaler_indices,
                primary_downmix,
                error)) {
            out.clear();
            return false;
        }
        EncodedChannelFrame frame{};
        const std::vector<AdolInstruction>& channel_optional_adol =
            optional_adol_by_channel[group.carrier.carrier_channel_id];
        if (group.analysis_arity == 1u) {
            if (!encode_direct_channel_frame(
                    group.carrier.carrier_channel_id,
                    group.carrier.quantization_shift,
                    kCodecV3SerializedChannelHeader,
                    group.analysis_source_ids.front(),
                    group.has_scaler_ix,
                    group.scaler_ix,
                    primary_downmix,
                    common_adol,
                    channel_optional_adol,
                    frame,
                    error)) {
                out.clear();
                return false;
            }
        } else {
            std::vector<std::uint32_t> indices;
            if (group.analysis_arity == 2u) {
                if (group.mix2_indices.size() != frame_count
                    || group.mix2_residuals.empty()) {
                    error = "mix2 channel-frame tables are incomplete";
                    out.clear();
                    return false;
                }
                indices.reserve(group.mix2_indices.size());
                for (const std::uint64_t index : group.mix2_indices) {
                    if (index > std::numeric_limits<std::uint32_t>::max()) {
                        error = "mix2 channel-frame index exceeds 32-bit storage";
                        out.clear();
                        return false;
                    }
                    if (index >= group.mix2_residuals.size()) {
                        error = "mix2 channel-frame index exceeds its residual table";
                        out.clear();
                        return false;
                    }
                    indices.push_back(static_cast<std::uint32_t>(index));
                }
                const std::array<std::uint32_t, 3> channel_ids = {
                    group.analysis_source_ids[0],
                    group.analysis_source_ids[1],
                    255u};
                const std::vector<std::int32_t> seeds = {
                    group.mix2_seeds.seed0,
                    group.mix2_seeds.seed1};
                const std::uint32_t golomb_parameter =
                    group.mix2_level_pack_mode;
                std::uint64_t golomb_bits = 0u;
                if (golomb_parameter == 0u
                    || golomb_parameter > 7u
                    || !golomb_rice_bit_count(
                        indices, golomb_parameter, golomb_bits)
                    || golomb_bits != group.golomb_index_bit_cost) {
                    error = "mix2 channel-frame Rice cost differs from native BitSize";
                    out.clear();
                    return false;
                }
                if (!encode_codebook_channel_frame_golomb(
                        group.carrier.carrier_channel_id,
                        group.carrier.quantization_shift,
                        kCodecV3SerializedChannelHeader,
                        2u,
                        group.residual_bit_width,
                        group.mix2_residuals,
                        {},
                        channel_ids,
                        0u,
                        golomb_parameter,
                        group.has_scaler_ix,
                        group.scaler_ix,
                        primary_downmix,
                        common_adol,
                        channel_optional_adol,
                        seeds,
                        indices,
                        frame,
                        error)) {
                    out.clear();
                    return false;
                }
            } else {
                if (group.mix3_indices.size() != frame_count
                    || group.mix3_residuals.empty()) {
                    error = "mix3 channel-frame tables are incomplete";
                    out.clear();
                    return false;
                }
                std::vector<std::array<std::int32_t, 2>> pairs;
                if (!unpack_mix3_residual_table(group.mix3_residuals, pairs, error)
                    || pairs.empty()) {
                    out.clear();
                    return false;
                }
                std::vector<std::int32_t> values0;
                std::vector<std::int32_t> values1;
                values0.reserve(pairs.size());
                values1.reserve(pairs.size());
                for (const auto& pair : pairs) {
                    values0.push_back(pair[0]);
                    values1.push_back(pair[1]);
                }
                indices.reserve(group.mix3_indices.size());
                for (const std::uint64_t index : group.mix3_indices) {
                    if (index > std::numeric_limits<std::uint32_t>::max()) {
                        error = "mix3 channel-frame index exceeds 32-bit storage";
                        out.clear();
                        return false;
                    }
                    if (index >= pairs.size()) {
                        error = "mix3 channel-frame index exceeds its residual table";
                        out.clear();
                        return false;
                    }
                    indices.push_back(static_cast<std::uint32_t>(index));
                }
                const std::array<std::uint32_t, 3> channel_ids = {
                    group.analysis_source_ids[0],
                    group.analysis_source_ids[1],
                    group.analysis_source_ids[2]};
                const std::vector<std::int32_t> seeds = {
                    group.mix3_seeds[0], group.mix3_seeds[1], group.mix3_seeds[2],
                    group.mix3_seeds[3], group.mix3_seeds[4]};
                const std::uint32_t golomb_parameter =
                    group.mix3_level_pack_mode;
                std::uint64_t golomb_bits = 0u;
                if (golomb_parameter == 0u
                    || golomb_parameter > 7u
                    || !golomb_rice_bit_count(
                        indices, golomb_parameter, golomb_bits)
                    || golomb_bits != group.golomb_index_bit_cost) {
                    error = "mix3 channel-frame Rice cost differs from native BitSize";
                    out.clear();
                    return false;
                }
                if (!encode_codebook_channel_frame_golomb(
                        group.carrier.carrier_channel_id,
                        group.carrier.quantization_shift,
                        kCodecV3SerializedChannelHeader,
                        3u,
                        group.residual_bit_width,
                        values0,
                        values1,
                        channel_ids,
                        0u,
                        golomb_parameter,
                        group.has_scaler_ix,
                        group.scaler_ix,
                        primary_downmix,
                        common_adol,
                        channel_optional_adol,
                        seeds,
                        indices,
                        frame,
                        error)) {
                    out.clear();
                    return false;
                }
            }
        }
        if (!finalize_channel_frame_span(frame_count, frame, error)) {
            out.clear();
            return false;
        }
        out.push_back(std::move(frame));
    }
    return true;
}

bool merge_channel_frames_into_carrier(
    CarrierUnit& carrier,
    std::vector<EncodedChannelFrame>& frames,
    std::string& error) {
    error.clear();
    if (carrier.descriptor.frame_count == 0u) {
        error = "carrier frame plan has an empty unit span";
        return false;
    }
    std::vector<bool> seen(kCodecV3ChannelCount, false);
    std::uint32_t seen_mask = 0u;
    for (EncodedChannelFrame& frame : frames) {
        if (frame.channel_id >= kCodecV3ChannelCount
            || (carrier.descriptor.layout & (std::uint32_t{1} << frame.channel_id)) == 0u
            || frame.word_count != frame.words.size()
            || frame.word_count > carrier.descriptor.frame_count) {
            error = "channel frame does not match the carrier descriptor";
            return false;
        }
        if (seen[frame.channel_id]) {
            error = "carrier frame plan contains a duplicate channel";
            return false;
        }
        seen[frame.channel_id] = true;
        seen_mask |= std::uint32_t{1} << frame.channel_id;
        std::vector<std::int32_t>& plane = carrier.planes[frame.channel_id];
        if (plane.size() != carrier.descriptor.frame_count) {
            error = "prepared carrier plane has the wrong unit span";
            return false;
        }
        const std::vector<std::int32_t> preamble_unsealed = plane;
        // compose:Channel:mux projects the A3D stream and closes the
        // per-channel sync/CRC prefix in the same pass. Every carrier plane
        // is framed independently with its group's bit_line; optional ADOL
        // placement does not create a second metadata overlay.
        if (!project_channel_payload(frame, plane, error)
            || !validate_projected_channel_payload(
                frame, plane, error)
            || !project_channel_crc(plane, error)
            || !validate_pcm_metadata_block(
                plane,
                0u,
                carrier.descriptor.frame_count,
                frame.quantization_shift)) {
            if (error.empty())
                error = "could not close codec-v3 channel mux sync/CRC";
            return false;
        }
        const std::uint32_t low_mask =
            (std::uint32_t{1} << frame.quantization_shift) - 1u;
        for (std::size_t sample = 0; sample < plane.size(); ++sample) {
            const std::uint32_t before =
                static_cast<std::uint32_t>(preamble_unsealed[sample]);
            const std::uint32_t after =
                static_cast<std::uint32_t>(plane[sample]);
            if (((before ^ after) & ~low_mask) != 0u) {
                error = "codec-v3 projection modified carrier audio bits";
                return false;
            }
        }
        frame.words = plane;
        frame.word_count = carrier.descriptor.frame_count;
        Crc16 crc;
        for (const std::int32_t word : frame.words) {
            const std::uint32_t raw = static_cast<std::uint32_t>(word);
            if ((raw >> 24u) != 0u && (raw >> 24u) != 0xFFu) {
                error = "merged channel frame exceeds signed PCM24 range";
                return false;
            }
            crc.process_words(&raw, 1u);
        }
        frame.crc_word = crc.stored_word();
    }
    if (seen_mask != carrier.descriptor.layout) {
        error = "carrier frame plan does not cover the complete carrier layout";
        return false;
    }
    return true;
}

} // namespace auro3d:encode
