#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace auro3d {
struct AuroCxBlobSegmentInfo {
    std::uint32_t identifier = 0;
    std::uint32_t payload_bytes = 0;
};
struct AuroCxIntegralGainInfo {
    std::uint32_t selector = 0;
    std::int32_t value = 0;
};
struct AuroCxMonoTopDownmixInfo {
    bool present = false;
    std::uint32_t kind = 0;
    std::vector<AuroCxIntegralGainInfo> gains;
};
// StereoTopDownmix_t: kind 0 → 2 gains; kind 2|3 → 1 gain.
struct AuroCxStereoTopDownmixInfo {
    bool present = false;
    std::uint32_t kind = 0;
    std::vector<AuroCxIntegralGainInfo> gains;
};
struct AuroCxChannelDownmixInfo {
    // Optional gains at ChannelDownmix +128 +140 (native decode).
    std::uint32_t gain_present_mask = 0;
    std::vector<AuroCxIntegralGainInfo> gains;
    // Optional intra_layer_gains at +152 (type#7), not MonoTopDownmix.
    bool intra_layer_present = false;
    std::vector<AuroCxIntegralGainInfo> intra_layer_gains;
    // Channel 12 (T): MonoTop at +0 +40 (types #1/#2).
    std::vector<AuroCxMonoTopDownmixInfo> mono_top;
    // Channels 28/29 (mask 805306368): StereoTop at +80 +104 (types #3/#4).
    std::vector<AuroCxStereoTopDownmixInfo> stereo_top;
};
struct AuroCxSchemaChannelInfo {
    std::uint32_t id = 0;
    std::uint32_t audio_stream_index = 0;
    std::uint32_t layer_index = 0;
    AuroCxIntegralGainInfo gain;
    bool flag0 = false;
    bool downmix_present = false;
    std::uint32_t downmix_bit_offset = 0;
    std::uint32_t downmix_bits = 0;
    AuroCxChannelDownmixInfo downmix;
};
struct AuroCxSchemaBedInfo {
    bool ambisonics = false;
    std::vector<AuroCxSchemaChannelInfo> channels;
};
struct AuroCxSchemaPositionInfo {
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;
};
struct AuroCxSchemaSpreadInfo {
    std::uint32_t x = 0;
    std::uint32_t y = 0;
    std::uint32_t z = 0;
};
struct AuroCxSchemaGainSubblockInfo {
    bool changed = false;
    bool differential = false;
    std::int32_t delta = 0;
    AuroCxIntegralGainInfo absolute;
};
struct AuroCxSchemaPositionSubblockInfo {
    bool changed = false;
    bool differential = false;
    AuroCxSchemaPositionInfo delta;
    AuroCxSchemaPositionInfo absolute;
};
struct AuroCxSchemaSpreadSubblockInfo {
    bool changed = false;
    AuroCxSchemaSpreadInfo absolute;
};
struct AuroCxSchemaObjectInfo {
    bool content_kind_present = false;
    std::uint64_t content_kind = 0;
    std::uint32_t content_kind_classifier = 0;
    std::uint32_t audio_stream_index = 0;
    bool flag0 = false;
    bool flag1 = false;
    AuroCxIntegralGainInfo gain;
    bool gains_use_default = false;
    bool positions_use_default = false;
    bool spreads_use_default = false;
    std::vector<AuroCxSchemaGainSubblockInfo> gains;
    std::vector<AuroCxSchemaPositionSubblockInfo> positions;
    std::vector<AuroCxSchemaSpreadSubblockInfo> spreads;
    bool zone_exclusion_present = false;
    std::uint32_t zone_exclusion_mask = 0;
    std::vector<AuroCxIntegralGainInfo> zone_exclusion_gains;
};
struct AuroCxSchemaObjectGroupInfo {
    bool gains_present = false;
    bool gains_use_default = false;
    bool positions_present = false;
    bool positions_use_default = false;
    bool spreads_present = false;
    bool spreads_use_default = false;
    std::vector<AuroCxSchemaGainSubblockInfo> gains;
    std::vector<AuroCxSchemaPositionSubblockInfo> positions;
    std::vector<AuroCxSchemaSpreadSubblockInfo> spreads;
    std::vector<AuroCxSchemaObjectInfo> objects;
};
struct AuroCxSchemaReferenceInfo {
    std::uint32_t index = 0;
    bool explicit_metadata_flag = false;
    AuroCxIntegralGainInfo gain;
};
struct AuroCxSchemaSwitchGroupElementInfo {
    std::vector<std::uint32_t> object_groups;
    std::vector<std::uint32_t> beds;
    std::vector<AuroCxSchemaReferenceInfo> object_group_references;
    std::vector<AuroCxSchemaReferenceInfo> bed_references;
};
struct AuroCxSchemaSwitchGroupInfo {
    std::vector<AuroCxSchemaSwitchGroupElementInfo> elements;
};
struct AuroCxSchemaProgramInfo {
    std::vector<std::uint32_t> beds;
    std::vector<std::uint32_t> object_groups;
    std::vector<std::uint32_t> switch_groups;
    std::vector<AuroCxSchemaReferenceInfo> bed_references;
    std::vector<AuroCxSchemaReferenceInfo> object_group_references;
};
struct AuroCxSchemaPduInfo {
    std::uint32_t type = 0;
    std::uint32_t first_audio_stream = 0;
    std::uint32_t audio_stream_count = 0;
    std::uint32_t header_flag0 = 0;
    std::uint32_t header_flag1 = 0;
    std::uint32_t header_value = 0;
    std::uint32_t transform_bed_index = 0;
    std::uint32_t transform_source_layer = 0;
    std::uint32_t transform_target_layer = 0;
    std::uint32_t transform_resample_factor = 1;
    std::uint64_t payload_bits = 0;
    std::uint32_t payload_bit_offset = 0;
    std::vector<std::uint8_t> payload_data;
    bool awc_payload_config_decoded = false;
    std::uint32_t awc_common_preamble = 0;
    std::vector<std::uint32_t> awc_stream_parameters;
    bool sasc_channel_bed_decoded = false;
    std::uint32_t sasc_object_group_index = 0;
    std::uint32_t sasc_bed_index = 0;
    bool sasc_mode_flag = false;
    bool sasc_config_flag = false;
    std::uint32_t sasc_resample_factor = 1;
    std::vector<std::vector<bool>> sasc_channel_sets;
    std::vector<std::vector<std::uint32_t>> sasc_channel_set_stream_indices;
    std::vector<std::uint32_t> sasc_audio_stream_indices;
    std::vector<std::uint32_t> sasc_linked_object_groups;
    std::uint32_t sasc_channel_bed_layer = 0;
};
struct CxSchemaDecodeState {
    bool valid = false;
    unsigned audio_stream_bits = 0;
    std::size_t pdu_vector_bit_offset = 0;
    std::uint32_t pdu_count = 0;
    std::vector<std::uint32_t> pdu_types;
    std::vector<AuroCxSchemaPduInfo> pdu_templates;
    std::uint32_t programs = 1, beds = 0, objects = 0, switches = 0;
    std::uint32_t gains_enabled = 0, explicit_metadata = 0;
    bool object_position_16bit = false;
    bool default_object_position_present = false;
    AuroCxSchemaPositionInfo default_object_position;
    std::uint32_t block_size = 0;
    std::uint32_t bed_channel_count = 0;
    std::uint32_t last_awc_stream_count = 0;
};
struct CxSchemaParseResult {
    bool header_decoded = false;
    bool common_config = false;
    bool gains_enabled = false;
    bool explicit_metadata = false;
    bool object_position_16bit = false;
    bool default_object_position_present = false;
    AuroCxSchemaPositionInfo default_object_position;
    std::uint32_t metadata_subblocks = 1;
    std::uint32_t block_size = 0;
    std::uint32_t audio_bit_offset = 0;
    std::uint32_t blob_bits = 0;
    std::uint32_t consumed_bits = 0;
    std::uint32_t config_header_bits = 0;
    std::uint32_t programs_end_bit = 0;
    std::vector<std::uint32_t> channel_end_bits;
    std::uint32_t pdu_count = 0;
    bool pdu_types_complete = false;
    std::vector<std::uint32_t> pdu_types;
    std::vector<AuroCxSchemaPduInfo> pdus;
    std::uint32_t primary_program_index = 0;
    std::uint32_t default_program_index = 0;
    std::vector<AuroCxSchemaProgramInfo> programs;
    std::vector<AuroCxSchemaChannelInfo> bed_channels;
    bool bed_channels_decoded = false;
    std::vector<AuroCxSchemaBedInfo> beds;
    std::vector<AuroCxSchemaObjectGroupInfo> object_groups;
    std::vector<AuroCxSchemaSwitchGroupInfo> switch_groups;
};
struct AuroCxProbeInfo {
    bool found = false;
    std::string sample_entry;
    std::uint16_t container_channels = 0;
    std::uint32_t sample_rate = 0, sample_count = 0, samples_per_access_unit = 0;
    std::uint64_t first_access_unit_offset = 0;
    std::uint32_t first_access_unit_size = 0;
    bool first_access_unit_has_sync = false;
    std::uint64_t second_access_unit_offset = 0;
    std::uint32_t second_access_unit_size = 0;
    bool second_access_unit_has_sync = false;
    std::vector<AuroCxBlobSegmentInfo> second_access_unit_segments;
    std::uint32_t second_schema_pdu_count = 0;
    bool second_schema_pdu_types_complete = false;
    std::vector<std::uint32_t> second_schema_pdu_types;
    std::vector<AuroCxSchemaPduInfo> second_schema_pdus;
    bool has_declared_layout = false;
    std::uint16_t declared_layout = 0;
    std::string declared_layout_name;
    std::vector<std::uint8_t> decoder_config;
    std::vector<AuroCxBlobSegmentInfo> first_access_unit_segments;
    bool schema_header_decoded = false;
    bool schema_common_config = false;
    bool schema_gains_enabled = false;
    bool schema_explicit_metadata = false;
    bool schema_object_position_16bit = false;
    bool schema_default_object_position_present = false;
    AuroCxSchemaPositionInfo schema_default_object_position;
    std::uint32_t schema_metadata_subblocks = 1;
    std::uint32_t schema_block_size = 0;
    std::uint32_t schema_codec_version = 0;
    std::uint32_t schema_codec_profile = 0;
    std::uint32_t schema_programs = 0;
    std::uint32_t schema_beds = 0;
    std::uint32_t schema_object_groups = 0;
    std::uint32_t schema_switch_groups = 0;
    std::uint32_t schema_program0_bed_references = 0;
    std::uint32_t schema_primary_program_index = 0;
    std::uint32_t schema_default_program_index = 0;
    std::vector<AuroCxSchemaProgramInfo> schema_program_data;
    bool schema_bed_channels_decoded = false;
    std::uint32_t schema_audio_bit_offset = 0;
    std::vector<AuroCxSchemaChannelInfo> schema_bed_channels;
    std::vector<AuroCxSchemaBedInfo> schema_bed_data;
    std::vector<AuroCxSchemaObjectGroupInfo> schema_object_group_data;
    std::vector<AuroCxSchemaSwitchGroupInfo> schema_switch_group_data;
    std::uint32_t schema_pdu_count = 0;
    bool schema_pdu_types_complete = false;
    std::uint32_t schema_blob_bits = 0;
    std::uint32_t schema_consumed_bits = 0;
    std::uint32_t schema_config_header_bits = 0;
    std::uint32_t schema_programs_end_bit = 0;
    std::vector<std::uint32_t> schema_channel_end_bits;
    std::vector<std::uint32_t> schema_pdu_types;
    std::vector<AuroCxSchemaPduInfo> schema_pdus;
    std::uint32_t second_schema_consumed_bits = 0;
    std::uint32_t second_schema_blob_bits = 0;
    std::uint32_t second_schema_audio_bit_offset = 0;
    std::uint32_t second_schema_config_header_bits = 0;
    std::uint32_t second_schema_programs_end_bit = 0;
    std::string error;
};
bool probe_auro_cx_mp4(const std::string& path, AuroCxProbeInfo& info);
bool auro_cx_declared_layout_from_acxd(
    const std::vector<std::uint8_t>& acxd,
    std::uint16_t& layout);
const char* auro_cx_awc_coding(
    const std::vector<AuroCxSchemaPduInfo>& pdus,
    std::size_t& awc_pdus);
void print_auro_cx_probe(const AuroCxProbeInfo& info);
bool cx_parse_schema_blob(
    const std::vector<std::uint8_t>& blob,
    CxSchemaParseResult& out,
    CxSchemaDecodeState* state,
    bool delta);
bool cx_decode_xor_segment1(
    const std::uint8_t* begin,
    const std::uint8_t* end,
    std::vector<std::uint8_t>& decoded,
    std::string& error);
bool cx_read_config_block_size(
    const std::vector<std::uint8_t>& blob,
    bool delta,
    const CxSchemaDecodeState* state,
    std::uint32_t& block_size);
}
