#include "mix_mix2.hpp"

#include "mix_deltas.hpp"

namespace auro3d::encode {
namespace {

std::int32_t avg_even_pair(std::int32_t left, std::int32_t right) {
    const std::uint32_t sum = static_cast<std::uint32_t>(left)
        + static_cast<std::uint32_t>(right);
    return codec_v3_arithmetic_shift_right(static_cast<std::int32_t>(sum), 1u);
}

std::int32_t add_wrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right));
}

std::int32_t sub_wrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) - static_cast<std::uint32_t>(right));
}

bool residual_at(
    const std::vector<std::int32_t>& residuals,
    std::uint64_t index,
    std::int32_t& value,
    std::string& error) {
    if (index >= residuals.size()) {
        error = "mix2 mixer residual index out of range";
        return false;
    }
    value = residuals[static_cast<std::size_t>(index)];
    return true;
}

} // namespace

bool mix2_mixer_reconstruct(
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::int32_t>& residuals,
    std::vector<std::int32_t>& carrier,
    Mix2MixerSeeds& seeds,
    std::string& error) {
    error.clear();
    carrier.clear();
    seeds = {};
    // Native gate: equal lengths >= 4, indices length == n, seeds capacity 2.
    if (primary.size() < 4u || primary.size() != secondary.size()
        || indices.size() != primary.size()) {
        error = "mix2 mixer requires matching primary/secondary/indices of length >= 4";
        return false;
    }
    const std::size_t n = primary.size();
    carrier.assign(n, 0);

    // First loop: odd residual slots + even-rounded primary evens into carrier[1..].
    {
        const std::size_t pairs = (n - 1u) >> 1u;
        std::int32_t prev_even = mix_even_round(primary[0]);
        std::size_t index_cursor = 1u;
        std::size_t src = 0u;
        for (std::size_t i = 0; i < pairs; ++i) {
            const std::int32_t next_even = mix_even_round(primary[src + 2u]);
            std::int32_t residual = 0;
            if (!residual_at(residuals, indices[index_cursor], residual, error))
                return false;
            carrier[1u + 2u * i] = add_wrap(residual, avg_even_pair(next_even, prev_even));
            carrier[2u + 2u * i] = next_even;
            prev_even = next_even;
            index_cursor += 2u;
            src += 2u;
        }
        if (n > ((n - 1u) | 1u))
            carrier[n - 1u] = primary[n - 1u];
    }

    // mix2::mix<1> @ 0x4EEB20: fold secondary into carrier[2..].
    {
        const std::size_t limit = n >= 2u ? n - 2u : 0u;
        const std::size_t pairs = limit >> 1u;
        std::int32_t prev_even = mix_even_round(secondary[1]);
        std::size_t index_cursor = 2u;
        std::size_t src = 0u;
        for (std::size_t i = 0; i < pairs; ++i) {
            const std::int32_t next_even = mix_even_round(secondary[src + 3u]);
            std::int32_t residual = 0;
            if (!residual_at(residuals, indices[index_cursor], residual, error))
                return false;
            carrier[2u + 2u * i] = add_wrap(
                carrier[2u + 2u * i], add_wrap(avg_even_pair(next_even, prev_even), residual));
            carrier[3u + 2u * i] = add_wrap(carrier[3u + 2u * i], next_even);
            prev_even = next_even;
            index_cursor += 2u;
            src += 2u;
        }
        if (n > (limit & ~std::size_t{1}) + 2u) {
            // Trailing secondary sample when native sets the final odd slot.
            carrier[n - 1u] = add_wrap(carrier[n - 1u], secondary[n - 1u]);
        }
    }

    // Trailing fixups from auro::mix::mix mix2 @ 0x4EE540 LABEL_20.
    {
        std::int32_t residual0 = 0;
        std::int32_t residual1 = 0;
        if (!residual_at(residuals, indices[0], residual0, error)
            || !residual_at(residuals, indices[1], residual1, error)) {
            return false;
        }
        carrier[0] = mix_even_round(primary[0]);
        carrier[0] = add_wrap(carrier[0], secondary[0]);
        carrier[1] = add_wrap(carrier[1], mix_even_round(secondary[1]));
        seeds.seed0 = sub_wrap(secondary[0], residual0);
        seeds.seed1 = sub_wrap(
            sub_wrap(carrier[1], residual1), mix_even_round(secondary[1]));
    }
    return true;
}

} // namespace auro3d::encode
