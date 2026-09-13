#include "headphone_pca_score_state.hpp"

#include <algorithm>

namespace auro3d {

bool HeadphonePcaScoreState::construct(std::size_t score_count) {
    try {
        first_ear_gains_.assign(score_count, 0.0f);
        second_ear_gains_.assign(score_count, 0.0f);
    } catch (...) {
        first_ear_gains_.clear();
        second_ear_gains_.clear();
        itd_samples_ = 0;
        return false;
    }
    itd_samples_ = 0;
    return true;
}

void HeadphonePcaScoreState::reset() noexcept {
    itd_samples_ = 0;
    std::fill(first_ear_gains_.begin(), first_ear_gains_.end(), 0.0f);
    std::fill(second_ear_gains_.begin(), second_ear_gains_.end(), 0.0f);
}

void HeadphonePcaScoreState::reset_audio_state() noexcept {
    // Native source reset clears delay/filter audio state, not the PCA gains
    // populated by PCABankV2_get_score_degrees.
}

bool HeadphonePcaScoreState::set_gains(
    const std::vector<float>& first_ear,
    const std::vector<float>& second_ear) noexcept {
    if (first_ear.size() != first_ear_gains_.size()
        || second_ear.size() != second_ear_gains_.size())
        return false;
    first_ear_gains_ = first_ear;
    second_ear_gains_ = second_ear;
    return true;
}

bool HeadphonePcaScoreState::accumulate(
    const std::array<float, 32>& first_ear_block,
    const std::array<float, 32>& second_ear_block,
    std::vector<std::array<float, 32>>& first_ear_outputs,
    std::vector<std::array<float, 32>>& second_ear_outputs) const noexcept {
    if (first_ear_outputs.size() != score_count()
        || second_ear_outputs.size() != score_count())
        return false;
    for (std::size_t score = 0u; score < score_count(); ++score) {
        HeadphonePcaAccumulator::add(first_ear_block, first_ear_gains_[score],
                                     first_ear_outputs[score]);
        HeadphonePcaAccumulator::add(second_ear_block, second_ear_gains_[score],
                                     second_ear_outputs[score]);
    }
    return true;
}

bool HeadphonePcaScoreState::accumulate_mono(
    const std::array<float, 32>& block,
    std::vector<std::array<float, 32>>& outputs) const noexcept {
    if (outputs.size() != score_count())
        return false;
    for (std::size_t score = 0u; score < score_count(); ++score)
        HeadphonePcaAccumulator::add(block, first_ear_gains_[score],
                                     outputs[score]);
    return true;
}

}
