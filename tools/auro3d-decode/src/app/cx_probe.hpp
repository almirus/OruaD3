#pragma once
#include <cstdint>
#include <string>
#include <vector>
namespace auro3d {
struct AuroCxBlobSegmentInfo {
    std::uint32_t identifier = 0;
    std::uint32_t payload_bytes = 0;
};
struct AuroCxSchemaChannelInfo {
    std::uint32_t id = 0;
    std::uint32_t audio_stream_index = 0;
};
struct AuroCxSchemaPduInfo {
    std::uint32_t type = 0;
    std::uint32_t first_audio_stream = 0;
    std::uint32_t audio_stream_count = 0;
    std::uint32_t header_flag0 = 0;
    std::uint32_t header_flag1 = 0;
    std::uint32_t header_value = 0;
    std::uint64_t payload_bits = 0;
    std::vector<std::uint8_t> payload_data;
    bool awc_payload_config_decoded = false;
    std::uint32_t awc_common_preamble = 0;
    std::vector<std::uint32_t> awc_stream_parameters;
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
    std::uint32_t schema_codec_version = 0;
    std::uint32_t schema_codec_profile = 0;
    std::uint32_t schema_programs = 0;
    std::uint32_t schema_beds = 0;
    std::uint32_t schema_object_groups = 0;
    std::uint32_t schema_switch_groups = 0;
    std::uint32_t schema_program0_bed_references = 0;
    bool schema_bed_channels_decoded = false;
    std::uint32_t schema_audio_bit_offset = 0;
    std::vector<AuroCxSchemaChannelInfo> schema_bed_channels;
    std::uint32_t schema_pdu_count = 0;
    bool schema_pdu_types_complete = false;
    std::vector<std::uint32_t> schema_pdu_types;
    std::vector<AuroCxSchemaPduInfo> schema_pdus;
    std::string error;
};
bool probe_auro_cx_mp4(const std::string& path, AuroCxProbeInfo& info);
void print_auro_cx_probe(const AuroCxProbeInfo& info);
}
