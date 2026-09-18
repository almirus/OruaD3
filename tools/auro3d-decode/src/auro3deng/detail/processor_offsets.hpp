#pragma once

#include <cstdint>

namespace auro3deng {

/// Поля Processor, с которыми сравниваются дескрипторы в Processor_process.
/// Смещения из машинного кода: mov eax, [rdi+imm32].
constexpr std::uint32_t kProcOff_InputLayout12 = 0x25E7C8;
constexpr std::uint32_t kProcOff_InputChannelMask = 0x25E7BC;
constexpr std::uint32_t kProcOff_InputBytesUnit = 0x25E7C0;
constexpr std::uint32_t kProcOff_InputExpectedField4 = 0x25E7C4;
constexpr std::uint32_t kProcOff_InputAuxField8 = 0x25E7CC;

constexpr std::uint32_t kProcOff_OutputLayout12 = 0x25E7DC;
constexpr std::uint32_t kProcOff_OutputChannelMask = 0x25E7D0;
constexpr std::uint32_t kProcOff_OutputBytesUnit = 0x25E7D4;
constexpr std::uint32_t kProcOff_OutputExpectedField4 = 0x25E7D8;
constexpr std::uint32_t kProcOff_OutputAuxField8 = 0x25E7E0;

} // namespace auro3deng
