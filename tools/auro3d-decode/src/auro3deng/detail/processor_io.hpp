#pragma once

#include <cstddef>
#include <cstdint>

namespace auro3deng {

// Descriptor used by auro_a3deng_v3_Processor_process.
// AuroDecoderImpl places input at +40 and output at +272, so the descriptor
// span is 232 bytes: 16 bytes of header plus 27 channel pointers.
struct ProcessorIOBufferDesc {
    std::uint32_t total_size_bytes;
    std::uint32_t field_4;
    std::uint32_t field_8;
    std::uint32_t layout_or_kind;
    std::uint64_t channel_ptr[27];
};

static_assert(sizeof(ProcessorIOBufferDesc) == 16 + 27 * 8, "layout");

constexpr std::size_t kProcessorProcessErrNull = 2;
constexpr std::size_t kProcessorProcessErrOutMask = 135;
constexpr std::size_t kProcessorProcessErrInMask = 136;

} // namespace auro3deng
