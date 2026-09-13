#include "headphone_wiir.hpp"
#include "headphone_pca_bank.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace auro3d {
namespace {

float read_filter_float(const std::vector<std::uint8_t>& bytes,
                        std::size_t offset) noexcept {
    float value = 0.0f;
    if (offset + sizeof(value) <= bytes.size())
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}

float float_from_bits(std::uint32_t bits) noexcept {
    float value = 0.0f;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

}

bool HeadphoneWiir::construct(float lambda,
                              const std::vector<Section>& sections) {
    if (!std::isfinite(lambda) || sections.empty())
        return false;
    for (const auto& section : sections) {
        if (!std::isfinite(section.scale)
            || !std::isfinite(section.output0)
            || !std::isfinite(section.output1)
            || !std::isfinite(section.output2)
            || !std::isfinite(section.feedback0)
            || !std::isfinite(section.feedback1)
            || !std::isfinite(section.feedback2))
            return false;
    }
    lambda_ = lambda;
    has_first_order_ = false;
    sections_ = sections;
    state_.assign(sections_.size(), {0.0f, 0.0f, 0.0f});
    return true;
}

bool HeadphoneWiir::construct_with_first_order(
    float lambda, const FirstOrder& first_order,
    const std::vector<Section>& sections) {
    if (!std::isfinite(lambda)
        || (!std::isfinite(first_order.scale))
        || (!std::isfinite(first_order.output0))
        || (!std::isfinite(first_order.output1))
        || (!std::isfinite(first_order.feedback0))
        || (!std::isfinite(first_order.feedback1)))
        return false;
    for (const auto& section : sections) {
        if (!std::isfinite(section.scale)
            || !std::isfinite(section.output0)
            || !std::isfinite(section.output1)
            || !std::isfinite(section.output2)
            || !std::isfinite(section.feedback0)
            || !std::isfinite(section.feedback1)
            || !std::isfinite(section.feedback2))
            return false;
    }
    lambda_ = lambda;
    has_first_order_ = true;
    first_order_ = first_order;
    first_state_ = {0.0f, 0.0f};
    sections_ = sections;
    state_.assign(sections_.size(), {0.0f, 0.0f, 0.0f});
    return true;
}

bool HeadphoneWiir::construct_from_filter(
    const HeadphonePcaFilterRecord& filter) {
    if (!filter.order || filter.coefficients.size()
            != 24u * ((static_cast<std::size_t>(filter.order) + 1u) / 2u))
        return false;
    const float lambda = float_from_bits(filter.gain_f32_bits);
    const bool odd = (filter.order & 1u) != 0;
    FirstOrder first;
    std::size_t coefficient_offset = 0u;
    if (odd) {
        first.output0 = read_filter_float(filter.coefficients, 0u);
        first.output1 = read_filter_float(filter.coefficients, 4u);
        const float raw_feedback = read_filter_float(filter.coefficients, 16u);
        first.feedback0 = raw_feedback;
        first.feedback1 = raw_feedback * lambda;
        first.scale = 1.0f / (1.0f - lambda * raw_feedback);
        coefficient_offset = 24u;
    }
    std::vector<Section> sections;
    try {
        sections.reserve(filter.order / 2u);
        for (std::uint32_t index = 0; index < filter.order / 2u; ++index) {
            Section section;
            section.output0 = read_filter_float(filter.coefficients,
                                                coefficient_offset + 0u);
            section.output1 = read_filter_float(filter.coefficients,
                                                coefficient_offset + 4u);
            section.output2 = read_filter_float(filter.coefficients,
                                                coefficient_offset + 8u);
            const float raw_feedback0 = read_filter_float(
                filter.coefficients, coefficient_offset + 16u);
            const float raw_feedback1 = read_filter_float(
                filter.coefficients, coefficient_offset + 20u);
            section.feedback0 = raw_feedback0 - lambda * raw_feedback1;
            section.feedback1 = raw_feedback1 + lambda * section.feedback0;
            section.feedback2 = raw_feedback1 * lambda;
            section.scale = 1.0f / (1.0f - lambda * section.feedback0);
            sections.push_back(section);
            coefficient_offset += 24u;
        }
    } catch (...) {
        return false;
    }
    return odd ? construct_with_first_order(lambda, first, sections)
               : construct(lambda, sections);
}

void HeadphoneWiir::reset_audio_state() noexcept {
    first_state_ = {0.0f, 0.0f};
    for (auto& state : state_)
        state = {0.0f, 0.0f, 0.0f};
}

bool HeadphoneWiir::process(const std::array<float, 32>& input,
                            std::array<float, 32>& output) noexcept {
    if (sections_.empty() && !has_first_order_) {
        output.fill(0.0f);
        return false;
    }
    output = input;
    if (has_first_order_) {
        for (std::size_t i = 0u; i < output.size(); ++i) {
            const float old0 = first_state_[0];
            const float old1 = first_state_[1];
            const float next0 = first_order_.scale *
                (output[i] - first_order_.feedback0 * old0
                   - first_order_.feedback1 * old1);
            const float next1 = old0 + lambda_ * (old1 - next0);
            output[i] = first_order_.output0 * next0
                       + first_order_.output1 * next1;
            first_state_ = {next0, next1};
        }
    }
    for (std::size_t section_index = 0u;
         section_index < sections_.size(); ++section_index) {
        const auto& section = sections_[section_index];
        auto& state = state_[section_index];
        for (std::size_t i = 0u; i < output.size(); ++i) {
            const float old0 = state[0];
            const float old1 = state[1];
            const float old2 = state[2];
            const float x = output[i];
            const float next0 = section.scale *
                (x - section.feedback0 * old0
                   - section.feedback1 * old1
                   - section.feedback2 * old2);
            const float next1 = old0 + lambda_ * (old1 - next0);
            const float next2 = old1 + lambda_ * (old2 - next1);
            output[i] = section.output0 * next0
                       + section.output1 * next1
                       + section.output2 * next2;
            state = {next0, next1, next2};
        }
    }
    return true;
}

bool HeadphoneWiir::process_pair(
    const std::array<float, 32>& first_input,
    const std::array<float, 32>& second_input,
    std::array<float, 32>& first_output,
    std::array<float, 32>& second_output,
    HeadphoneWiir& second_filter) noexcept {
    if (sections_.empty() && !has_first_order_
        || second_filter.sections_.empty() && !second_filter.has_first_order_)
        return false;

    first_output = first_input;
    second_output = second_input;
    if (has_first_order_ || second_filter.has_first_order_) {
        if (!has_first_order_ || !second_filter.has_first_order_)
            return false;
        for (std::size_t i = 0u; i < first_output.size(); ++i) {
            const float first_old0 = first_state_[0];
            const float first_old1 = first_state_[1];
            const float second_old0 = second_filter.first_state_[0];
            const float second_old1 = second_filter.first_state_[1];
            const float first_next0 = first_order_.scale *
                (first_output[i] - first_order_.feedback0 * first_old0
                 - first_order_.feedback1 * first_old1);
            const float second_next0 = second_filter.first_order_.scale *
                (second_output[i] - second_filter.first_order_.feedback0
                    * second_old0 - second_filter.first_order_.feedback1
                    * second_old1);
            const float first_next1 = first_old0
                + lambda_ * (first_old1 - first_next0);
            const float second_next1 = second_old0
                + second_filter.lambda_ * (second_old1 - second_next0);
            first_output[i] = first_order_.output0 * first_next0
                + first_order_.output1 * first_next1;
            second_output[i] = second_filter.first_order_.output0
                    * second_next0
                + second_filter.first_order_.output1 * second_next1;
            first_state_ = {first_next0, first_next1};
            second_filter.first_state_ = {second_next0, second_next1};
        }
    }
    if (sections_.size() != second_filter.sections_.size())
        return false;
    for (std::size_t section_index = 0u;
         section_index < sections_.size(); ++section_index) {
        const auto& first_section = sections_[section_index];
        const auto& second_section = second_filter.sections_[section_index];
        auto& first_state = state_[section_index];
        auto& second_state = second_filter.state_[section_index];
        for (std::size_t i = 0u; i < first_output.size(); ++i) {
            const float first_old0 = first_state[0];
            const float first_old1 = first_state[1];
            const float first_old2 = first_state[2];
            const float second_old0 = second_state[0];
            const float second_old1 = second_state[1];
            const float second_old2 = second_state[2];
            const float first_x = first_output[i];
            const float second_x = second_output[i];
            const float first_next0 = first_section.scale *
                (first_x - first_section.feedback0 * first_old0
                 - first_section.feedback1 * first_old1
                 - first_section.feedback2 * first_old2);
            const float second_next0 = second_section.scale *
                (second_x - second_section.feedback0 * second_old0
                 - second_section.feedback1 * second_old1
                 - second_section.feedback2 * second_old2);
            const float first_next1 = first_old0
                + lambda_ * (first_old1 - first_next0);
            const float second_next1 = second_old0
                + second_filter.lambda_ * (second_old1 - second_next0);
            const float first_next2 = first_old1
                + lambda_ * (first_old2 - first_next1);
            const float second_next2 = second_old1
                + second_filter.lambda_ * (second_old2 - second_next1);
            first_output[i] = first_section.output2 * first_next2
                + (first_section.output0 * first_next0
                   + first_section.output1 * first_next1);
            second_output[i] = second_section.output2 * second_next2
                + (second_section.output0 * second_next0
                   + second_section.output1 * second_next1);
            first_state = {first_next0, first_next1, first_next2};
            second_state = {second_next0, second_next1, second_next2};
        }
    }
    return true;
}

}
