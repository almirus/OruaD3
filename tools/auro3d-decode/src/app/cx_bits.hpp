#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <vector>

namespace auro3d {
namespace cx {

class Bits {
public:
    explicit Bits(const std::vector<std::uint8_t>& data, std::size_t start_bit = 0, std::size_t end_bit = kNoLimit)
        : d_(data), p_(start_bit), end_(end_bit) {
        const std::size_t total = d_.size() * 8u;
        if (end_ == kNoLimit || end_ > total)
            end_ = total;
        if (p_ > end_)
            p_ = end_;
    }

    const std::vector<std::uint8_t>& data() const { return d_; }
    std::size_t position() const { return p_; }
    void set_position(std::size_t bit) { p_ = bit; }
    std::size_t end_position() const { return end_; }

    std::uint64_t remaining_bits() const {
        return p_ >= end_ ? 0u : static_cast<std::uint64_t>(end_ - p_);
    }

    bool get(unsigned n, std::uint32_t& v) {
        if (n > 32 || p_ + n > end_)
            return false;
        v = 0;
        for (unsigned i = 0; i < n; ++i)
            v |= ((d_[(p_ + i) / 8] >> ((p_ + i) & 7)) & 1u) << i;
        p_ += n;
        return true;
    }

    bool unary(std::uint32_t& v) {
        v = 0;
        std::uint32_t x = 0;
        while (get(1, x)) {
            if (!x)
                return true;
            ++v;
        }
        return false;
    }

    bool skip(std::uint64_t count) {
        std::uint32_t ignored = 0;
        while (count) {
            const unsigned chunk = static_cast<unsigned>(count > 32 ? 32 : count);
            if (!get(chunk, ignored))
                return false;
            count -= chunk;
        }
        return true;
    }

    std::uint64_t pop_all_ones() {
        std::uint64_t total = 0;
        while (p_ < end_) {
            const std::uint8_t byte = d_[p_ >> 3u];
            if (((byte >> (p_ & 7u)) & 1u) == 0)
                break;
            ++p_;
            ++total;
        }
        return total;
    }

    bool read_golomb_rice(unsigned param, std::uint64_t& value) {
        if (param > 31)
            return false;
        const std::uint64_t quotient = pop_all_ones();
        if (param + 1u > remaining_bits())
            return false;
        std::uint32_t zero = 0;
        if (!get(1, zero) || zero != 0u)
            return false;
        std::uint32_t remainder = 0;
        if (!get(param, remainder))
            return false;
        const std::uint32_t mask = param ? ((1u << param) - 1u) : 0u;
        value = (quotient << param) | (remainder & mask);
        return true;
    }

    bool read_vlq(unsigned param, std::uint64_t& value) {
        if (param < 2 || param > 63)
            return false;
        const std::uint64_t continuation = std::uint64_t(1) << (param - 1u);
        const std::uint64_t payload_mask = continuation - 1u;
        value = 0;
        while (true) {
            if (param > remaining_bits())
                return false;
            const unsigned low_width = std::min(param, 32u);
            const unsigned high_width = param - low_width;
            std::uint32_t low = 0;
            std::uint32_t high = 0;
            if (!get(low_width, low) || (high_width && !get(high_width, high)))
                return false;
            const std::uint64_t field = std::uint64_t(low) | (std::uint64_t(high) << 32u);
            value = (value << (param - 1u)) | (field & payload_mask);
            if ((field & continuation) == 0)
                return true;
        }
    }

private:
    static constexpr std::size_t kNoLimit = static_cast<std::size_t>(-1);
    const std::vector<std::uint8_t>& d_;
    std::size_t p_;
    std::size_t end_;
};

} // namespace cx
} // namespace auro3d
