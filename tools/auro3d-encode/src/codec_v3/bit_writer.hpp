#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace auro3d::encode {

/// Native bitstream::Writer<VectorStorage>::write_ stores the least-significant
/// requested bits first, starting at bit 0 of each output byte. This class is
/// a direct storage-level counterpart; it deliberately contains no codec field
/// syntax or automatic byte alignment.
class LsbBitWriter {
public:
    bool write_bits(std::uint64_t value, std::uint32_t bit_count);
    bool write_zeroes(std::uint64_t bit_count);
    bool write_one();

    std::uint64_t bit_count() const { return bit_count_; }
    std::size_t byte_count() const { return bytes_.size(); }
    const std::vector<std::uint8_t>& bytes() const { return bytes_; }
    void clear();

private:
    bool reserve_bits(std::uint64_t additional_bits);

    std::vector<std::uint8_t> bytes_;
    std::uint64_t bit_count_ = 0;
};

} // namespace auro3d::encode
