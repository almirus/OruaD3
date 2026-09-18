#include "mix_unmix.hpp"

namespace auro3d::encode {
namespace {

std::int32_t add_wrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left)
        + static_cast<std::uint32_t>(right));
}

std::int32_t sub_wrap(std::int32_t left, std::int32_t right) {
    return static_cast<std::int32_t>(
        static_cast<std::uint32_t>(left)
        - static_cast<std::uint32_t>(right));
}

std::int32_t double_delta(
    std::int32_t value,
    std::int32_t previous) {
    const std::int32_t delta = sub_wrap(value, previous);
    return add_wrap(value, add_wrap(delta, delta));
}

bool residual1_at(
    const std::vector<std::int32_t>& residuals,
    std::uint64_t index,
    std::int32_t& residual,
    std::string& error) {
    if (index >= residuals.size()) {
        error = "mix2 unmix residual index is out of range";
        return false;
    }
    residual = residuals[static_cast<std::size_t>(index)];
    return true;
}

bool residual2_at(
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    std::uint64_t index,
    std::array<std::int32_t, 2>& residual,
    std::string& error) {
    if (index >= residuals.size()) {
        error = "mix3 unmix residual index is out of range";
        return false;
    }
    residual = residuals[static_cast<std::size_t>(index)];
    return true;
}

bool validate_sum2(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    std::string& error) {
    for (std::size_t index = 0u; index < carrier.size(); ++index) {
        if (carrier[index] != add_wrap(primary[index], secondary[index])) {
            error = "mix2 unmix reconstruction does not close to carrier";
            return false;
        }
    }
    return true;
}

bool validate_sum3(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::int32_t>& primary,
    const std::vector<std::int32_t>& secondary,
    const std::vector<std::int32_t>& tertiary,
    std::string& error) {
    for (std::size_t index = 0u; index < carrier.size(); ++index) {
        if (carrier[index] != add_wrap(
                primary[index],
                add_wrap(secondary[index], tertiary[index]))) {
            error = "mix3 unmix reconstruction does not close to carrier";
            return false;
        }
    }
    return true;
}

void write_predictor(
    std::vector<std::int32_t>& plane,
    std::size_t cursor,
    std::int32_t value,
    std::uint32_t horizon) {
    if (horizon < 2u)
        return;
    const std::int32_t delta = sub_wrap(value, plane[cursor]);
    plane[cursor + 1u] = value;
    plane[cursor + 2u] =
        add_wrap(value, add_wrap(delta, delta));
    if (horizon >= 3u)
        plane[cursor + 3u] =
            add_wrap(plane[cursor + 2u], delta);
}

bool mix3_proceed(
    std::array<std::vector<std::int32_t>*, 3>& planes,
    std::array<std::size_t, 3>& cursors,
    std::size_t sample,
    std::uint32_t horizon,
    std::int32_t carrier,
    const std::array<std::int32_t, 2>& residual,
    std::string& error) {
    const std::size_t phase = sample % 3u;
    const std::size_t add_plane =
        phase == 0u ? 1u : phase == 1u ? 2u : 0u;
    const std::size_t predict_plane =
        phase == 0u ? 2u : phase == 1u ? 0u : 1u;
    const std::size_t other_plane =
        3u - add_plane - predict_plane;
    const std::int32_t add_residual =
        phase == 1u ? residual[1] : residual[0];
    const std::int32_t predicted_residual =
        phase == 1u ? residual[0] : residual[1];

    ++cursors[add_plane];
    ++cursors[other_plane];
    if (cursors[add_plane] >= planes[add_plane]->size()
        || cursors[other_plane] >= planes[other_plane]->size()
        || cursors[predict_plane] >= planes[predict_plane]->size()) {
        error = "mix3 unmix train cursor is out of range";
        return false;
    }
    std::int32_t& add_value =
        (*planes[add_plane])[cursors[add_plane]];
    add_value = add_wrap(add_value, add_residual);
    const std::int32_t baseline = sub_wrap(
        carrier,
        add_wrap(
            predicted_residual,
            add_wrap(
                add_value,
                (*planes[other_plane])[cursors[other_plane]])));
    if (cursors[predict_plane] + horizon >= planes[predict_plane]->size()) {
        error = "mix3 unmix predictor horizon is out of range";
        return false;
    }
    write_predictor(
        *planes[predict_plane],
        cursors[predict_plane],
        baseline,
        horizon);
    ++cursors[predict_plane];
    (*planes[predict_plane])[cursors[predict_plane]] =
        add_wrap(predicted_residual, baseline);
    return true;
}

} // namespace

bool mix2_unmix_reconstruct(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::int32_t>& residuals,
    const Mix2MixerSeeds& seeds,
    std::vector<std::int32_t>& primary,
    std::vector<std::int32_t>& secondary,
    std::string& error) {
    error.clear();
    primary.clear();
    secondary.clear();
    if (carrier.size() < 3u || indices.size() != carrier.size()) {
        error = "mix2 unmix requires matching carrier/indices of length >= 3";
        return false;
    }
    primary.assign(carrier.size(), 0);
    secondary.assign(carrier.size(), 0);
    std::int32_t residual0 = 0;
    std::int32_t residual1 = 0;
    if (!residual1_at(residuals, indices[0], residual0, error)
        || !residual1_at(residuals, indices[1], residual1, error)) {
        return false;
    }
    secondary[0] = add_wrap(residual0, seeds.seed0);
    primary[0] = sub_wrap(carrier[0], secondary[0]);
    primary[2] = double_delta(seeds.seed1, primary[0]);
    primary[1] = add_wrap(residual1, seeds.seed1);
    secondary[1] = sub_wrap(carrier[1], primary[1]);

    for (std::size_t sample = 2u; sample + 1u < carrier.size(); ++sample) {
        std::int32_t residual = 0;
        if (!residual1_at(residuals, indices[sample], residual, error))
            return false;
        if ((sample & 1u) == 0u) {
            const std::int32_t baseline = sub_wrap(
                carrier[sample],
                add_wrap(residual, primary[sample]));
            secondary[sample] = add_wrap(residual, baseline);
            secondary[sample + 1u] =
                double_delta(baseline, secondary[sample - 1u]);
        } else {
            const std::int32_t baseline = sub_wrap(
                carrier[sample],
                add_wrap(residual, secondary[sample]));
            primary[sample] = add_wrap(residual, baseline);
            primary[sample + 1u] =
                double_delta(baseline, primary[sample - 1u]);
        }
    }
    const std::size_t last = carrier.size() - 1u;
    if ((last & 1u) == 0u)
        secondary[last] = sub_wrap(carrier[last], primary[last]);
    else
        primary[last] = sub_wrap(carrier[last], secondary[last]);
    return validate_sum2(carrier, primary, secondary, error);
}

bool mix3_unmix_reconstruct(
    const std::vector<std::int32_t>& carrier,
    const std::vector<std::uint64_t>& indices,
    const std::vector<std::array<std::int32_t, 2>>& residuals,
    const Mix3MixerSeeds& seeds,
    std::vector<std::int32_t>& primary,
    std::vector<std::int32_t>& secondary,
    std::vector<std::int32_t>& tertiary,
    std::string& error) {
    error.clear();
    primary.clear();
    secondary.clear();
    tertiary.clear();
    if (carrier.size() < 5u || indices.size() != carrier.size()) {
        error = "mix3 unmix requires matching carrier/indices of length >= 5";
        return false;
    }
    primary.assign(carrier.size(), 0);
    secondary.assign(carrier.size(), 0);
    tertiary.assign(carrier.size(), 0);
    std::array<std::int32_t, 2> r0{};
    std::array<std::int32_t, 2> r1{};
    std::array<std::int32_t, 2> r2{};
    if (!residual2_at(residuals, indices[0], r0, error)
        || !residual2_at(residuals, indices[1], r1, error)
        || !residual2_at(residuals, indices[2], r2, error)) {
        return false;
    }

    secondary[0] = add_wrap(seeds.values[0], r0[0]);
    tertiary[0] = add_wrap(seeds.values[1], r0[1]);
    primary[0] = sub_wrap(
        carrier[0], add_wrap(secondary[0], tertiary[0]));

    primary[1] = add_wrap(seeds.values[2], r1[0]);
    tertiary[1] = add_wrap(seeds.values[3], r1[1]);
    secondary[1] = sub_wrap(
        carrier[1], add_wrap(primary[1], tertiary[1]));
    const std::int32_t primary_delta =
        sub_wrap(seeds.values[2], primary[0]);
    primary[2] = add_wrap(
        seeds.values[2],
        add_wrap(primary_delta, primary_delta));
    primary[3] = add_wrap(primary[2], primary_delta);

    primary[2] = add_wrap(primary[2], r2[0]);
    secondary[2] = add_wrap(seeds.values[4], r2[1]);
    tertiary[2] = sub_wrap(
        carrier[2], add_wrap(primary[2], secondary[2]));
    const std::int32_t secondary_delta =
        sub_wrap(seeds.values[4], secondary[1]);
    secondary[3] = add_wrap(
        seeds.values[4],
        add_wrap(secondary_delta, secondary_delta));
    secondary[4] = add_wrap(secondary[3], secondary_delta);

    std::array<std::vector<std::int32_t>*, 3> planes{
        &primary, &secondary, &tertiary,
    };
    std::array<std::size_t, 3> cursors{2u, 2u, 2u};
    for (std::size_t sample = 3u; sample < carrier.size(); ++sample) {
        std::array<std::int32_t, 2> residual{};
        if (!residual2_at(
                residuals, indices[sample], residual, error)) {
            return false;
        }
        const std::uint32_t horizon =
            sample + 2u < carrier.size() ? 3u
            : sample + 1u < carrier.size() ? 2u
            : 1u;
        if (!mix3_proceed(
                planes,
                cursors,
                sample,
                horizon,
                carrier[sample],
                residual,
                error)) {
            return false;
        }
    }
    return validate_sum3(
        carrier, primary, secondary, tertiary, error);
}

} // namespace auro3d:encode
