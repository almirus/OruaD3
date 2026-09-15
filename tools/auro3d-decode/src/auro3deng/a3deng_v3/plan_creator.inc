// Plan_Creator_make / Configurator_make_plan / sub_4CD6C0 activation port.
// Included from a3deng_v3/implementation.inc inside namespace auro3deng.

void xinn_retarget_processed_runtime_preset_backing(
    std::uint8_t* processed_preset);

namespace {

// These helpers are defined by the XinN port later in implementation.inc;
// the native Configurator consumes the same preset pointers before that
// implementation section is reached by this included translation unit.
bool xinn_initialize_processed_preset_from_portable_spreset(
    std::uint8_t* processed_preset,
    std::uint64_t spreset_u64);

// Native Plan_Creator_make memset()s 0xA10 bytes.  The newer decompile's
// Scene callback also addresses the tail fields at +2492..+2519.
constexpr std::size_t kPlanBufferBytes = 0xA10u;
constexpr std::size_t kRemixPlanWorkspaceBytes = 0x8F0u;
constexpr std::uintptr_t kManagerOffPlanCreator = 568u;
constexpr std::uintptr_t kManagerOffRemapOutput = 136u;
constexpr std::uintptr_t kManagerOffPlanScratch = 608u;
constexpr std::uintptr_t kManagerOffBlockRecord = 8u;
constexpr std::uintptr_t kManagerOffConfigureResult = 488u;
constexpr std::uintptr_t kManagerOffLayoutReady = 480u;
constexpr std::uintptr_t kManagerOffLayoutEqual = 484u;

// libauro(1).so.c:386043.  The native helper returns 0/1/2 for the
// soundbar generation selected from the configured layout.
std::uint32_t soundbar_category_from_layout_portable(std::uint32_t layout) {
    if ((layout & 0x1B0u) == 0u)
        return 0u;
    return ((~layout & 0x600030u) == 0u
        || (~layout & 0x30600u) == 0u) ? 2u : 1u;
}

// PresetManager_configure @ 0x474A00: a6 selects the preset family.  The
// value stored at Configurator+24 is a private portable bridge field; native
// Configurator does not expose it, but the dispatch itself is copied exactly.
std::uint32_t preset_family_from_native_fields(
    std::uint32_t preset_kind,
    std::uint32_t configured_layout) {
    if (preset_kind == 2u)
        return 0u; // SS
    if (preset_kind == 1u)
        return 1u + soundbar_category_from_layout_portable(configured_layout);
    return 4u; // a6 == 0 or 3: multichannel
}

struct PlanActivationEntry {
    std::uint32_t plan_index;
    std::uintptr_t step_base;
};

constexpr PlanActivationEntry kPlanActivationOrder[] = {
    {0u, 0u},
    {4u, 64u},
    {5u, 1800u},
    {6u, 3536u},
    {10u, 3608u},
    {28u, 4456u},
    {196u, 5416u},
    {362u, 6248u},
    {364u, 7944u},
    {416u, 13072u},
    {420u, 13160u},
    {586u, 13992u},
    {589u, 14184u},
    {596u, 14560u},
    {598u, 14648u},
    {601u, 14848u},
    {602u, 15800u},
    {603u, 16176u},
    {604u, 16248u},
    {605u, 61784u},
};

std::int64_t speaker_layout_bit_popcount_helper(
    std::uint32_t layout,
    std::uint32_t allowed_mask) {
    if ((layout & ~allowed_mask & 0xFFFF8000u) != 0u)
        return 0;
    const auto adjacent_pair =
        (static_cast<std::uint8_t>(layout ^ (layout >> 1))
            | static_cast<std::uint8_t>((layout >> 4) ^ (layout >> 5))
            | static_cast<std::uint8_t>((layout >> 7) ^ (layout >> 8))
            | static_cast<std::uint8_t>((layout >> 9) ^ (layout >> 10))
            | static_cast<std::uint8_t>((layout >> 13) ^ (layout >> 14))
            | static_cast<std::uint8_t>((layout >> 16) ^ (layout >> 17))
            | static_cast<std::uint8_t>((layout >> 18) ^ (layout >> 19)))
        & 1u;
    if (adjacent_pair != 0u)
        return 0;
    if ((((layout >> 22) ^ (layout >> 21)) & 1u) != 0u)
        return 0;
    const std::uint32_t front = layout & 3u;
    if ((layout & 0xFFFFFFF7u) != 4u && front != 3u)
        return 0;
    const std::uint32_t inverted = ~layout;
    if ((inverted & 0x1C0u) == 0u
        || ((inverted & 0x180u) == 0u && (inverted & 0x30u) != 0u))
        return 0;
    const bool not_stereo = front != 3u;
    const std::uint32_t surround = layout & 0x600u;
    if ((surround == 0x600u && not_stereo)
        || (((surround != 0x600u) ? 1u : 0u)
            & static_cast<std::uint8_t>(layout >> 11u)) != 0u
        || ((inverted & 0x630u) != 0u && (inverted & 0x6000u) == 0u))
        return 0;
    return static_cast<std::int64_t>(
        surround == 0x600u
        || ((layout & 0x1000u) == 0u && (inverted & 0x30000000u) != 0u));
}

} // namespace

bool auro_speaker_layout_is_consistent(
    std::uint32_t layout,
    std::int32_t schema) {
    switch (schema) {
    case 12:
    case 13: {
        if ((layout & 0x804u) == 0x800u)
            return false;
        return speaker_layout_bit_popcount_helper(layout, 0u) != 0;
    }
    case 2:
    case 14:
        if ((layout & 3u) == 0u)
            return false;
        if (!speaker_layout_bit_popcount_helper(layout, 0u))
            return false;
        if ((layout & 0x40u) != 0u && (layout & 0x30u) != 0u)
            return false;
        return true;
    default:
        return layout != 0u;
    }
}

std::int64_t auro_remix_v1_Legacy_t_transform(
    std::uint32_t* output_layout,
    std::uint32_t input_layout) {
    if (!output_layout)
        return 1;
    std::array<std::uint32_t, 3u> legacy_plan{};
    if (auro_remix_v1_plan_Legacy_t_construct(legacy_plan.data(), input_layout) != 0)
        return 1;
    *output_layout =
        auro_remix_v1_plan_LegacyFlags_remap_layout(legacy_plan.data() + 2u, input_layout);
    return 0;
}

void auro_remix_v1_Configurator_set_centergen_preset(
    std::uint8_t* state,
    const std::uint8_t* preset_76) {
    if (!state || !preset_76)
        return;
    std::memcpy(state + 72u, preset_76, 76u);
}

void auro_remix_v1_Configurator_set_XinN_stereo_preset(
    std::uint8_t* state,
    std::uint64_t preset) {
    if (state)
        *reinterpret_cast<std::uint64_t*>(state + 152u) = preset;
}

void auro_remix_v1_Configurator_set_XinN_surround_preset(
    std::uint8_t* state,
    std::uint64_t preset) {
    if (state)
        *reinterpret_cast<std::uint64_t*>(state + 160u) = preset;
}

void auro_remix_v1_Configurator_set_XinN_stereo_parameters(
    std::uint8_t* state,
    std::uint64_t parameters) {
    if (state)
        *reinterpret_cast<std::uint64_t*>(state + 168u) = parameters;
}

void auro_remix_v1_Configurator_set_XinN_surround_parameters(
    std::uint8_t* state,
    std::uint64_t parameters) {
    if (state)
        *reinterpret_cast<std::uint64_t*>(state + 176u) = parameters;
}

void auro_remix_v1_Configurator_set_3d3d_preset(
    std::uint8_t* state,
    std::uint64_t preset) {
    if (state)
        *reinterpret_cast<std::uint64_t*>(state + 184u) = preset;
}

void auro_remix_v1_Configurator_set_force_front_manager_reset(
    std::uint8_t* state,
    std::uint32_t value) {
    if (state)
        *reinterpret_cast<std::uint32_t*>(state + 192u) = value;
}

namespace {

std::int64_t remix_configurator_get_bass_config(
    std::uint8_t* state,
    std::uint8_t* out_24) {
    if (!state || !out_24)
        return 1;
    *reinterpret_cast<std::uint64_t*>(out_24 + 0u) =
        *reinterpret_cast<std::uint64_t*>(state + 212u);
    *reinterpret_cast<std::uint64_t*>(out_24 + 8u) =
        *reinterpret_cast<std::uint64_t*>(state + 220u);
    *reinterpret_cast<std::uint64_t*>(out_24 + 16u) =
        *reinterpret_cast<std::uint64_t*>(state + 228u);
    return 0;
}

std::int64_t remix_configurator_get_alt3d_config(
    std::uint8_t* state,
    std::uint8_t* out_24) {
    if (!state || !out_24)
        return 1;
    *reinterpret_cast<std::uint64_t*>(out_24 + 0u) =
        *reinterpret_cast<std::uint64_t*>(state + 200u);
    *reinterpret_cast<std::uint32_t*>(out_24 + 8u) =
        *reinterpret_cast<std::uint32_t*>(state + 208u);
    return 0;
}

std::int64_t manager_link_plan_metadata(
    std::uint8_t* step,
    std::uint8_t* predecessor_metadata) {
    if (!step || !predecessor_metadata)
        return 1;
    *reinterpret_cast<std::uint64_t*>(step + 32u) =
        reinterpret_cast<std::uint64_t>(predecessor_metadata);
    std::memcpy(step + 40u, predecessor_metadata, 16u);
    *reinterpret_cast<std::uint32_t*>(step + 56u) =
        *reinterpret_cast<const std::uint32_t*>(predecessor_metadata + 16u);
    return 0;
}

// Headroom::configure @ 0x4D1440. Rate comes from the predecessor metadata at
// step+32 (+8); the fourth activation argument is unused. Leave step+32 set.
std::int64_t manager_configure_headroom_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state) {
    (void)first_time;
    (void)plan_entry;
    (void)block_state;
    if (!step)
        return 1;
    const auto* bound = reinterpret_cast<const std::uint8_t*>(
        static_cast<std::uintptr_t>(
            *reinterpret_cast<const std::uint64_t*>(step + 32u)));
    if (!bound)
        return 1;
    if (*reinterpret_cast<const std::uint32_t*>(bound + 12u) != 0u
        || *reinterpret_cast<const std::uint32_t*>(step + 52u) != 0u)
        return 1;
    *reinterpret_cast<std::uint32_t*>(step + 372u) =
        *reinterpret_cast<const std::uint32_t*>(bound + 8u);
    return 0;
}

// Gate::configure @ 0x4D1BB0. Unity ramp endpoints at +64; success when
// predecessor+12 and step+52 are both clear. Does not clear step+32.
std::int64_t manager_configure_gate_step(
    std::uint8_t* step,
    const std::uint32_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state) {
    (void)first_time;
    (void)plan_entry;
    (void)block_state;
    if (!step)
        return 1;
    *reinterpret_cast<std::uint64_t*>(step + 64u) = 0x3F8000003F800000ull;
    const auto* bound = reinterpret_cast<const std::uint8_t*>(
        static_cast<std::uintptr_t>(
            *reinterpret_cast<const std::uint64_t*>(step + 32u)));
    if (!bound || *reinterpret_cast<const std::uint32_t*>(bound + 12u) != 0u)
        return 1;
    return *reinterpret_cast<const std::uint32_t*>(step + 52u) != 0u ? 1 : 0;
}

std::int64_t manager_configure_upmix_step(
    std::uint8_t* step,
    const std::uint8_t* plan_entry,
    bool first_time,
    std::uint8_t* block_state) {
    (void)first_time;
    if (!step || !plan_entry || !block_state)
        return 1;
    // Native Upmix::configure @ 0x4CF440 reads the bound record from step+32
    // (already linked by activation) and copies engine params from plan+8.
    *reinterpret_cast<std::uint32_t*>(block_state + 12u) = 0u;
    *reinterpret_cast<std::uint32_t*>(step + 52u) = 0u;
    std::array<std::uint8_t, 0xD0u> config_blob{};
    std::memcpy(config_blob.data() + 8u, plan_entry + 8u, 0xC8u);
    return auro_a3deng_v3_pipeline_Upmix_configure(
        step, block_state, config_blob.data());
}

bool frames_meta_equal_plan_output(
    const std::uint8_t* manager,
    const std::uint8_t* /*metadata*/) {
    if (!manager)
        return false;
    const std::uint32_t requested =
        *reinterpret_cast<const std::uint32_t*>(manager + kManagerOffRemapOutput + 4u);
    const std::uint32_t produced =
        *reinterpret_cast<const std::uint32_t*>(manager + 28u);
    return requested != 0u && (requested & produced) == requested;
}

// auro_a3deng_v3_FramesMeta_t_equal — five consecutive u32 fields (20 bytes).
// Confirmed by libauro(1).so.c; current libauro.so.c decompile is mangled.
bool frames_meta_t_equal(
    const std::uint32_t* a,
    const std::uint32_t* b) {
    if (!a || !b)
        return false;
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3]
        && a[4] == b[4];
}

std::int64_t plan_creator_mode_build_default(
    std::uint32_t* creator,
    std::uint32_t* plan,
    std::uint8_t* meta_info,
    std::uint64_t /*remap_context*/) {
    if (!creator || !plan || !meta_info)
        return 1;
    plan[364] = 1u;
    // Native sub_4D6510 is part of the Plan_Creator mode callback.  Keep its
    // fields in the plan even when the current host does not instantiate the
    // optional Scene step yet; Plan_configure_remixer and later activation
    // consume the same offsets.
    const auto* input_record =
        *reinterpret_cast<const std::uint8_t* const*>(meta_info + 0u);
    const auto* output_record =
        *reinterpret_cast<const std::uint8_t* const*>(meta_info + 8u);
    if (!input_record || !output_record)
        return 1;
    const auto scene_rc = auro_a3deng_v3_scene_get_remix_layout(
        reinterpret_cast<std::uint32_t*>(meta_info + 32u),
        reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(plan) + 2492u),
        reinterpret_cast<std::uint32_t*>(meta_info + 24u),
        static_cast<std::int32_t>(
            *reinterpret_cast<const std::uint32_t*>(meta_info + 16u)),
        *reinterpret_cast<const std::uint32_t*>(input_record + 0u),
        *reinterpret_cast<const std::uint32_t*>(output_record + 0u),
        creator[0]);
    if (scene_rc != 0)
        return scene_rc;
    *reinterpret_cast<std::uint32_t*>(plan + 2496u / sizeof(std::uint32_t)) =
        *reinterpret_cast<const std::uint32_t*>(meta_info + 24u);
    if (*reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const std::uint8_t*>(plan) + 2492u) != 0u) {
        std::memcpy(
            reinterpret_cast<std::uint8_t*>(plan) + 2504u,
            meta_info + 44u,
            16u);
        if (*reinterpret_cast<const std::uint32_t*>(meta_info + 24u) == 0u) {
            const auto input_rate =
                *reinterpret_cast<const std::uint32_t*>(input_record + 8u);
            if (input_rate > 0x17700u)
                return 1;
            auto rate = *reinterpret_cast<std::uint32_t*>(meta_info + 84u);
            if (rate >= 0xBB81u) {
                do {
                    const auto previous = rate;
                    rate >>= 1u;
                    if (previous <= 0x17701u)
                        break;
                } while (rate != 0u);
                *reinterpret_cast<std::uint32_t*>(meta_info + 84u) = rate;
            }
            return rate >= 0x7D00u ? 0 : 204;
        }
    }
    return 0;
}

// Plan_Creator callback sub_4D64D0: native creator type 6 (headphone
// virtualization) uses the ChannelTest record as its mode-specific plan
// payload.  Keep this callback separate from the Scene callback; the native
// constructor selects it from creator[1].
std::int64_t plan_creator_mode_build_headphones(
    std::uint32_t* plan,
    std::uint8_t* meta_info,
    std::uint64_t remap_context) {
    if (!plan || !meta_info)
        return 1;
    const auto* input_record =
        *reinterpret_cast<const std::uint8_t* const*>(meta_info + 0u);
    if (!input_record)
        return 1;
    plan[598u] = 1u;
    plan[599u] = *reinterpret_cast<const std::uint32_t*>(input_record + 0u);
    plan[600u] = static_cast<std::uint32_t>(remap_context);
    return 0;
}

std::int64_t plan_creator_post_process_default(
    std::uint32_t* /*creator*/,
    std::uint32_t* /*plan*/,
    std::int64_t /*preset_manager*/,
    std::uint64_t /*remap_context*/) {
    return 0;
}

// The tail of Plan_configure_remixer @ 0x4D6A00.  The native code chooses
// the accepted core rate from the remixer flags, then repeatedly halves the
// MetaInfo rate until one of the exact native rate predicates is satisfied.
std::int64_t normalize_remixer_core_rate_native(
    std::uint8_t* meta_info,
    std::uint32_t native_rate_flags,
    std::uint32_t upmix_active) {
    if (!meta_info)
        return 1;
    const bool no_upmix_or_no_remixer =
        native_rate_flags == 0u || upmix_active == 0u;
    std::uint32_t accepted_flags = no_upmix_or_no_remixer ? 1023u : 48u;
    std::uint32_t rate =
        *reinterpret_cast<std::uint32_t*>(meta_info + 84u);
    if ((accepted_flags & 0x40u) != 0u) {
        if ((accepted_flags & 0x80u) != 0u) {
            const std::uint32_t high_flags = accepted_flags >> 8u;
            for (;;) {
                if (rate > 88199u) {
                    if (rate == 88200u || rate == 96000u)
                        return 0;
                } else if (rate == 44100u || rate == 48000u) {
                    return 0;
                }
                if (((rate == 176400u) && (high_flags & 1u) != 0u)
                    || (no_upmix_or_no_remixer && rate == 192000u)
                    || rate == 32000u)
                    return 0;
                *reinterpret_cast<std::uint32_t*>(meta_info + 84u) = rate >> 1u;
                if (rate <= 0xF9FFu)
                    return 205;
                rate >>= 1u;
            }
        }
        const std::uint32_t high_flags = accepted_flags >> 8u;
        while (rate != 44100u && rate != 48000u && rate != 88200u
            && (!((rate == 176400u) && (high_flags & 1u) != 0u))
            && (!(no_upmix_or_no_remixer && rate == 192000u))) {
            *reinterpret_cast<std::uint32_t*>(meta_info + 84u) = rate >> 1u;
            if (rate <= 0xF9FFu)
                return 205;
            rate >>= 1u;
        }
        return 0;
    }
    if ((accepted_flags & 0x80u) != 0u) {
        const std::uint32_t high_flags = accepted_flags >> 8u;
        while (rate != 44100u && rate != 48000u && rate != 96000u
            && (!((rate == 176400u) && (high_flags & 1u) != 0u))
            && (!(no_upmix_or_no_remixer && rate == 192000u))
            && rate != 32000u) {
            *reinterpret_cast<std::uint32_t*>(meta_info + 84u) = rate >> 1u;
            if (rate <= 0xF9FFu)
                return 205;
            rate >>= 1u;
        }
        return 0;
    }
    const std::uint32_t high_flags = accepted_flags >> 8u;
    while (rate != 44100u && rate != 48000u
        && (!((rate == 176400u) && (high_flags & 1u) != 0u))
        && (!(no_upmix_or_no_remixer && rate == 192000u))) {
        *reinterpret_cast<std::uint32_t*>(meta_info + 84u) = rate >> 1u;
        if (rate <= 0xF9FFu)
            return 205;
        rate >>= 1u;
    }
    return 0;
}

} // namespace

std::int64_t auro_matic_v3_Configurator_process(
    std::uint8_t* configurator,
    std::uint8_t* plan_region,
    std::int32_t* layout_io,
    std::uint32_t* upmix_active,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t preset_kind,
    std::uint32_t room_preset) {
    if (!plan_region || !layout_io || !upmix_active)
        return 1;
    // Upmix::configure copies plan_entry+8 (= plan byte 1464 = this region)
    // for 0xC8 bytes straight into Engine params. Keep that blob identical to
    // the portable Engine configure contract (target/source/preset/dynamic).
    std::memset(plan_region, 0, 200u);
    *upmix_active = 0u;
    const std::uint32_t listening_mode = configurator
        ? *reinterpret_cast<const std::uint32_t*>(configurator + 16u) : 0u;
    const bool surround_mode = listening_mode >= 6u;
    // +24/+32 are the native wrapper callbacks (sub_4DB940/sub_4DB970),
    // not configuration words.  Native make_plan obtains the preset kind and
    // configured layout from MetaInfo before calling this Matic operation;
    // carry those values as explicit portable arguments.
    const std::uint32_t preset_family = preset_family_from_native_fields(
        preset_kind, target_layout);
    // Native Plan_configure_remixer passes MetaInfo+20 as PresetManager's
    // room argument.  The wrapper's +32 is the native get-bass-config
    // callback, never a room index.
    const std::uint32_t native_room_index = std::min<std::uint32_t>(
        room_preset, 4u);
    std::array<std::uint32_t, 64u> engine_params{};
    if (build_engine_params_for_upmix_plan_family(
            engine_params.data(),
            source_layout,
            target_layout,
            0u,
            surround_mode,
            native_room_index,
            preset_family) != 0)
        return 1;
    // Native set_centergen_preset stores a 76-byte block at wrapper+72 and
    // Configurator_process forwards it to the FrontManager plan at +12.
    // The common builder supplies the native defaults; replace that region
    // only when the native override pointer is present.
    if (configurator) {
        const auto centergen_override = *reinterpret_cast<const std::uint64_t*>(
            configurator + 72u);
        if (centergen_override != 0u
            && (source_layout & 0x3u) == 0x3u
            && (source_layout & 0x4u) == 0u
            && (target_layout & 0x4u) != 0u) {
            std::memcpy(
                reinterpret_cast<std::uint8_t*>(engine_params.data()) + 12u,
                reinterpret_cast<const void*>(static_cast<std::uintptr_t>(
                    centergen_override)),
                76u);
        }
    }
    // Native sub_54F890 returns the selected XinN preset/parameter pair from
    // the embedded Matic Configurator.  The wrapper setter offsets alias the
    // corresponding inner fields: stereo (+152/+168), surround (+160/+176).
    // Preserve the resource-backed default above, but honor an explicitly
    // supplied native preset pointer exactly when one is present.
    if (configurator) {
        const auto preset_offset = surround_mode ? 160u : 152u;
        const auto parameters_offset = surround_mode ? 176u : 168u;
        const auto selected_preset = *reinterpret_cast<const std::uint64_t*>(
            configurator + preset_offset);
        if (selected_preset != 0u) {
            const auto processed_u64 = static_cast<std::uint64_t>(engine_params[30u])
                | (static_cast<std::uint64_t>(engine_params[31u]) << 32u);
            auto* processed = reinterpret_cast<std::uint8_t*>(
                static_cast<std::uintptr_t>(processed_u64));
            if (!processed
                || !xinn_initialize_processed_preset_from_portable_spreset(
                    processed, selected_preset))
                return 1;
            xinn_retarget_processed_runtime_preset_backing(processed);
            engine_params[30u] = static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(processed));
            engine_params[31u] = static_cast<std::uint32_t>(
                reinterpret_cast<std::uintptr_t>(processed) >> 32u);
        }
        const auto selected_parameters = *reinterpret_cast<const std::uint64_t*>(
            configurator + parameters_offset);
        if (selected_parameters != 0u) {
            engine_params[32u] = static_cast<std::uint32_t>(selected_parameters);
            engine_params[33u] = static_cast<std::uint32_t>(selected_parameters >> 32u);
        }
    }
    std::memcpy(plan_region, engine_params.data(), 0xC8u);
    *upmix_active = engine_params[48u];
    *layout_io = static_cast<std::int32_t>(engine_params[1u]);
    return 0;
}

std::int64_t auro_remix_v1_Configurator_make_plan(
    std::uint8_t* configurator,
    std::uint8_t* workspace,
    std::uint32_t source_layout,
    std::uint32_t target_layout,
    std::uint32_t preset_kind = 0u,
    std::uint32_t room_preset = 0u) {
    if (!configurator || !workspace)
        return 1;
    if (!auro_speaker_layout_is_consistent(source_layout, 13)
        || !auro_speaker_layout_is_consistent(target_layout, 14))
        return 1;
    std::memset(workspace, 0, kRemixPlanWorkspaceBytes);
    *reinterpret_cast<std::uint32_t*>(workspace + 2280u) = source_layout;
    std::array<std::uint32_t, 3u> legacy_plan{};
    if (auro_remix_v1_plan_Legacy_t_construct(legacy_plan.data(), source_layout) != 0)
        return 1;
    *reinterpret_cast<std::uint32_t*>(workspace + 672u) =
        auro_remix_v1_plan_Legacy_active(legacy_plan.data()) ? 1u : 0u;
    std::memcpy(workspace + 676u, legacy_plan.data(), 12u);
    const std::uint32_t legacy_layout = legacy_plan[1u];
    std::array<std::uint8_t, 24u> bass_config{};
    std::array<std::uint8_t, 24u> alt3d_config{};
    if (remix_configurator_get_bass_config(configurator, bass_config.data()) != 0
        || remix_configurator_get_alt3d_config(configurator, alt3d_config.data()) != 0)
        return 1;
    std::uint32_t projected_layout = target_layout;
    const std::uint32_t listening_mode =
        *reinterpret_cast<const std::uint32_t*>(configurator + 16u);
    const std::uint32_t device =
        *reinterpret_cast<const std::uint32_t*>(configurator + 20u);
    if (!auro_remix_v1_project_output_layout(
            &projected_layout,
            target_layout,
            legacy_layout,
            device,
            listening_mode))
        return 1;
    std::uint32_t current_layout = legacy_layout;
    std::uint32_t bass_flag = 0u;
    if (auro_remix_v1_Configurator_plan_bass_management(
            workspace + 1348u,
            &current_layout,
            &bass_flag,
            legacy_layout,
            projected_layout,
            bass_config.data(),
            device) != 0)
        return 1;
    *reinterpret_cast<std::uint32_t*>(workspace + 1344u) = bass_flag;
    std::array<std::uint8_t, 16u> alt3d_flags{};
    std::uint32_t alt_store_active = 0u;
    std::array<std::uint8_t, 56u> downmix_engine{};
    if (auro_remix_v1_Configurator_plan_alt_3D_store(
            downmix_engine.data(),
            workspace + 8u,
            current_layout,
            projected_layout,
            alt3d_config.data(),
            &current_layout,
            &alt_store_active) != 0)
        return 1;
    *reinterpret_cast<std::uint32_t*>(workspace + 0u) = alt_store_active;
    const std::uint32_t downmix_target =
        auro_matic_v3_Configurator_get_downmix_layout(current_layout, projected_layout);
    if (auro_remix_v1_Configurator_compute_downmix_plan(
            workspace + 696u,
            current_layout,
            downmix_target,
            reinterpret_cast<std::uint32_t*>(workspace + 688u)) != 0)
        return 1;
    std::int32_t layout_after_upmix = static_cast<std::int32_t>(current_layout);
    std::uint32_t upmix_active = 0u;
    if (auro_matic_v3_Configurator_process(
            configurator,
            workspace + 1392u,
            &layout_after_upmix,
            &upmix_active,
            current_layout,
            projected_layout,
            preset_kind,
            room_preset) != 0)
        return 1;
    *reinterpret_cast<std::uint32_t*>(workspace + 1388u) = upmix_active;
    current_layout = static_cast<std::uint32_t>(layout_after_upmix);
    const std::uint32_t alt3d_required =
        *reinterpret_cast<const std::uint32_t*>(workspace + 16u);
    if ((projected_layout & alt3d_required & static_cast<std::uint32_t>(layout_after_upmix))
        != alt3d_required)
        return 1;
    const std::uint32_t second_target =
        current_layout & (~downmix_target | projected_layout);
    if (auro_remix_v1_Configurator_compute_downmix_plan(
            workspace + 1616u,
            current_layout,
            second_target,
            reinterpret_cast<std::uint32_t*>(workspace + 1608u)) != 0)
        return 1;
    if (current_layout != projected_layout)
        return 1;
    *reinterpret_cast<std::uint32_t*>(workspace + 2268u) = current_layout;
    *reinterpret_cast<std::uint32_t*>(workspace + 2272u) =
        target_layout & ~current_layout;
    *reinterpret_cast<std::uint32_t*>(workspace + 2264u) =
        (target_layout & ~current_layout) == 0u ? 0u : 1u;
    if ((current_layout & ~target_layout) != 0u)
        return 1;
    const bool silence_flag =
        upmix_active != 0u
        && *reinterpret_cast<const std::uint32_t*>(workspace + 1584u) != 0u;
    *reinterpret_cast<std::uint32_t*>(workspace + 2276u) = silence_flag ? 1u : 0u;
    *reinterpret_cast<std::uint32_t*>(workspace + 2284u) = target_layout;
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Plan_configure_remixer(
    std::uint8_t* plan,
    std::uint8_t* meta_info,
    std::int64_t /*preset_manager*/,
    std::uint32_t device) {
    if (!plan || !meta_info)
        return 1;
    const auto* input_record =
        *reinterpret_cast<const std::uint8_t* const*>(meta_info + 0u);
    if (input_record
        && *reinterpret_cast<const std::uint32_t*>(input_record + 8u) > 96000u)
        return 0;
    std::array<std::uint8_t, 248u> remix_configurator{};
    if (auro_remix_v1_Configurator_t_construct(remix_configurator.data()) == 0)
        return 1;
    auro_remix_v1_Configurator_set_listening_mode(
        remix_configurator.data(),
        *reinterpret_cast<const std::uint32_t*>(meta_info + 16u));
    if (!auro_remix_v1_Configurator_set_device(remix_configurator.data(), device))
        return 1;
    std::array<std::uint8_t, 16u> alt3d_envelope{};
    *reinterpret_cast<std::uint32_t*>(alt3d_envelope.data() + 8u) =
        *reinterpret_cast<const std::uint32_t*>(meta_info + 40u);
    auro_remix_v1_Configurator_configure_alt_3D(
        remix_configurator.data(), alt3d_envelope.data());
    std::array<std::uint8_t, 24u> bass_envelope{};
    std::memcpy(bass_envelope.data(), meta_info + 44u, 16u);
    *reinterpret_cast<std::uint64_t*>(bass_envelope.data() + 16u) =
        *reinterpret_cast<const std::uint64_t*>(meta_info + 76u);
    auro_remix_v1_Configurator_configure_bass_management(
        remix_configurator.data(), bass_envelope.data());
    std::array<std::uint8_t, kRemixPlanWorkspaceBytes> workspace{};
    const auto* input_layout_record =
        *reinterpret_cast<const std::uint8_t* const*>(meta_info + 0u);
    const auto* output_layout_record =
        *reinterpret_cast<const std::uint8_t* const*>(meta_info + 8u);
    if (!input_layout_record || !output_layout_record)
        return 1;
    const std::uint32_t source_layout =
        *reinterpret_cast<const std::uint32_t*>(input_layout_record + 0u);
    const std::uint32_t target_layout =
        *reinterpret_cast<const std::uint32_t*>(output_layout_record + 0u);
    if (auro_remix_v1_Configurator_make_plan(
            remix_configurator.data(),
            workspace.data(),
            source_layout,
            target_layout,
            *reinterpret_cast<const std::uint32_t*>(meta_info + 24u),
            *reinterpret_cast<const std::uint32_t*>(meta_info + 20u)) != 0)
        return 1;
    *reinterpret_cast<std::uint32_t*>(plan + 24u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 672u);
    *reinterpret_cast<std::uint64_t*>(plan + 28u) =
        *reinterpret_cast<const std::uint64_t*>(workspace.data() + 676u);
    *reinterpret_cast<std::uint32_t*>(plan + 36u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 684u);
    *reinterpret_cast<std::uint32_t*>(plan + 112u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 112u);
    std::memcpy(plan + 120u, workspace.data() + 120u, 0x298u);
    *reinterpret_cast<std::uint32_t*>(plan + 784u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 784u);
    std::memcpy(plan + 792u, workspace.data() + 792u, 0x288u);
    *reinterpret_cast<std::uint32_t*>(plan + 40u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 0x540u);
    // Native Plan_configure_remixer writes the BassManagement optional plan
    // as eleven consecutive 16-byte fields (a1+44..a1+204) from the
    // Configurator workspace v99..v109 (workspace+0x544..+0x5F3).
    std::memcpy(plan + 44u, workspace.data() + 0x544u, 0xB0u);
    *reinterpret_cast<std::uint32_t*>(plan + 1456u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 1388u);
    std::memcpy(plan + 1464u, workspace.data() + 1392u, 0xC8u);
    *reinterpret_cast<std::uint32_t*>(plan + 1664u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 1592u);
    std::memcpy(plan + 1668u, workspace.data() + 1596u, 16u);
    *reinterpret_cast<std::uint32_t*>(plan + 1680u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 1608u);
    std::memcpy(plan + 1688u, workspace.data() + 1616u, 0x288u);
    *reinterpret_cast<std::uint32_t*>(plan + 2344u) =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 2264u);
    *reinterpret_cast<std::uint64_t*>(plan + 2348u) =
        *reinterpret_cast<const std::uint64_t*>(workspace.data() + 2268u);
    std::uint32_t legacy_transform = 0u;
    if (auro_remix_v1_Legacy_t_transform(
            &legacy_transform,
            *reinterpret_cast<const std::uint32_t*>(plan + 2348u)) != 0)
        return 1;
    *reinterpret_cast<std::uint32_t*>(plan + 1440u) = legacy_transform;
    *reinterpret_cast<std::uint32_t*>(plan + 2336u) = legacy_transform;
    const auto remixer_flags =
        *reinterpret_cast<const std::uint32_t*>(workspace.data() + 2276u);
    const auto upmix_active =
        *reinterpret_cast<const std::uint32_t*>(plan + 1456u);
    return normalize_remixer_core_rate_native(
        meta_info, remixer_flags, upmix_active);
}

std::int64_t auro_a3deng_v3_pipeline_plan_Creator_make(
    std::uint32_t* creator,
    std::uint32_t* plan,
    std::int64_t preset_manager,
    const std::uint8_t* config,
    std::uint64_t remap_context) {
    if (!creator || !plan || !config)
        return 205;
    std::array<std::uint8_t, 88u> meta_info{};
    if (auro_a3deng_v3_pipeline_plan_MetaInfo_t_construct(
            meta_info.data(), config) != 0)
        return 205;
    const auto* output_record =
        *reinterpret_cast<const std::uint8_t* const*>(meta_info.data() + 8u);
    const auto* input_record =
        *reinterpret_cast<const std::uint8_t* const*>(meta_info.data() + 0u);
    if (!output_record || !input_record)
        return 205;
    if (*reinterpret_cast<const std::uint32_t*>(output_record + 4u)
        > *reinterpret_cast<const std::uint32_t*>(input_record + 4u))
        return 204; // native compares block-size fields at record+4
    std::memset(plan, 0, kPlanBufferBytes);
    const auto mode_rc = creator[1u] == 6u
        ? plan_creator_mode_build_headphones(plan, meta_info.data(), remap_context)
        : plan_creator_mode_build_default(creator, plan, meta_info.data(), remap_context);
    if (mode_rc != 0)
        return mode_rc;
    if (plan[364] != 0u) {
        const auto remix_rc = auro_a3deng_v3_pipeline_Plan_configure_remixer(
            reinterpret_cast<std::uint8_t*>(plan),
            meta_info.data(),
            preset_manager,
            creator[0]);
        if (remix_rc != 0)
            return remix_rc;
    }
    const auto convert_rc = auro_a3deng_v3_pipeline_Plan_configure_sample_convertors(
        reinterpret_cast<std::uint8_t*>(plan),
        meta_info.data(),
        creator[2]);
    if (convert_rc != 0)
        return convert_rc;
    const auto resample_rc = auro_a3deng_v3_pipeline_Plan_configure_resamplers(
        reinterpret_cast<std::uint8_t*>(plan),
        meta_info.data());
    if (resample_rc != 0)
        return resample_rc;
    plan[602] = 1u;
    plan[603] = 1u;
    plan[604] = creator[3];
    // Native Plan_Creator_make tail: *((QWORD*)s + 318) = 0x100000001
    // followed by *((DWORD*)s + 638) = creator[3].
    plan[636] = 1u;
    plan[637] = 1u;
    plan[638] = creator[3];
    return plan_creator_post_process_default(
        creator, plan, preset_manager, remap_context);
}

std::int64_t auro_a3deng_v3_pipeline_Manager_activate_plan(
    std::uint8_t* manager,
    std::int32_t initial,
    std::uint32_t* plan) {
    if (!manager || !plan)
        return 1;
    auto* step_chain = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(
        *reinterpret_cast<std::uint64_t*>(manager + ::auro_engine_v4_ida::kManager_off_step_chain)));
    if (!step_chain)
        return 1;
    *reinterpret_cast<std::uint64_t*>(manager + 0u) =
        reinterpret_cast<std::uint64_t>(manager + kManagerOffBlockRecord);
    const auto* input_record = manager + kManagerOffBlockRecord;
    const std::uint32_t input_layout =
        *reinterpret_cast<const std::uint32_t*>(input_record + 0u);
    const std::uint32_t output_layout =
        *reinterpret_cast<const std::uint32_t*>(manager + 28u);
    if (!auro_a3deng_v3_RemapOutput_set_output_layout(
            manager + kManagerOffRemapOutput,
            input_layout,
            output_layout))
        return 1;
    std::uint8_t metadata[20u]{};
    *reinterpret_cast<std::uint32_t*>(metadata + 16u) =
        *reinterpret_cast<const std::uint32_t*>(manager + 24u);
    std::uint8_t* predecessor = manager + kManagerOffBlockRecord;
    *reinterpret_cast<std::uint32_t*>(manager + kManagerOffConfigureResult) = 0u;
    for (const auto& entry : kPlanActivationOrder) {
        auto* step = step_chain + entry.step_base;
        const bool first_time =
            *reinterpret_cast<const std::uint64_t*>(step + 32u) == 0u;
        *reinterpret_cast<std::uint64_t*>(step + 32u) = 0u;
        if (plan[entry.plan_index] == 0u)
            continue;
        if (manager_link_plan_metadata(step, predecessor) != 0)
            return 1;
        std::int64_t rc = 1;
        if (entry.plan_index == 364u) {
            rc = manager_configure_upmix_step(
                step,
                reinterpret_cast<const std::uint8_t*>(plan + entry.plan_index),
                first_time,
                manager + kManagerOffBlockRecord);
        } else if (entry.plan_index == 10u) {
            rc = manager_configure_bass_management_step(
                step,
                plan + entry.plan_index + 1u,
                *reinterpret_cast<const std::uint32_t*>(
                    manager + kManagerOffBlockRecord + 8u));
            if (rc == 0)
                *reinterpret_cast<std::uint8_t*>(step + 40u) |= 8u;
        } else if (entry.plan_index == 6u) {
            rc = manager_configure_legacy_step(
                step,
                plan + entry.plan_index,
                *reinterpret_cast<const std::uint32_t*>(predecessor));
        } else if (entry.plan_index == 362u) {
            rc = manager_configure_delay_step(
                step,
                plan + entry.plan_index,
                *reinterpret_cast<const std::uint32_t*>(predecessor));
        } else if (entry.plan_index == 28u) {
            rc = manager_configure_alt3d_store_step(
                step, plan + entry.plan_index);
        } else if (entry.plan_index == 416u) {
            rc = manager_configure_alt3d_mix_step(
                step, step_chain + 4456u, plan + entry.plan_index);
        } else if (entry.plan_index == 586u) {
            // Silence::configure @ 0x4D4DD0 consumes block_state+8 and also
            // merges that mask into the linked layout metadata at step+40.
            rc = manager_configure_silence_step(
                step, manager + kManagerOffBlockRecord);
            if (rc == 0)
                *reinterpret_cast<std::uint32_t*>(step + 40u) |=
                    *reinterpret_cast<const std::uint32_t*>(
                        manager + kManagerOffBlockRecord + 8u);
        } else if (entry.plan_index == 598u) {
            // ChannelTest::configure @ 0x4D60E0 consumes its own three-word
            // plan record, not the predecessor metadata.
            rc = manager_configure_channel_test_step(
                step,
                reinterpret_cast<const std::uint8_t*>(plan + entry.plan_index));
        } else if (entry.plan_index == 589u) {
            // Scene::configure @ 0x4D26F0 selects the direct callbacks only
            // for mode 1.  The processor-backed branch requires ASC4HE and
            // must not be silently treated as a no-op.
            rc = manager_configure_scene_step(
                step, plan + entry.plan_index);
        } else if (entry.plan_index == 196u || entry.plan_index == 420u) {
            rc = manager_configure_downmixer_step(
                step,
                plan + entry.plan_index + 1u,
                first_time,
                manager + kManagerOffBlockRecord);
        } else if (entry.plan_index == 4u || entry.plan_index == 5u) {
            rc = manager_configure_downsample_step(
                step,
                plan + entry.plan_index,
                first_time,
                manager + kManagerOffBlockRecord);
        } else if (entry.plan_index == 601u) {
            rc = manager_configure_upsample_step(
                step,
                plan + entry.plan_index,
                first_time,
                manager + kManagerOffBlockRecord);
        } else if (entry.plan_index == 602u) {
            rc = manager_configure_headroom_step(
                step,
                plan + entry.plan_index,
                first_time,
                manager + kManagerOffBlockRecord);
        } else if (entry.plan_index == 603u) {
            rc = manager_configure_gate_step(
                step,
                plan + entry.plan_index,
                first_time,
                manager + kManagerOffBlockRecord);
        } else if (entry.plan_index == 604u) {
            rc = manager_configure_peak_limiter_step(
                step,
                plan + entry.plan_index,
                first_time,
                manager + kManagerOffBlockRecord);
        } else if (entry.plan_index == 596u) {
            // Native plan[596] is the AHP/AM4HP virtualization step. Its
            // processor still requires the native HP core and must not be
            // cleared as an unknown plan entry.
            return 1;
        } else if (entry.plan_index == 605u || entry.plan_index == 0u) {
            // SampleConvertor::configure @ 0x4CF840 (input plan[0] / output plan[605]).
            rc = manager_configure_sample_convertor_step(
                step,
                plan + entry.plan_index,
                first_time,
                manager + kManagerOffBlockRecord);
        } else {
            *reinterpret_cast<std::uint64_t*>(step + 32u) = 0u;
            continue;
        }
        *reinterpret_cast<std::uint32_t*>(manager + kManagerOffConfigureResult) =
            static_cast<std::uint32_t>(rc);
        if (rc != 0)
            return rc;
        predecessor = step + 40u;
        std::memcpy(metadata, predecessor, 16u);
        *reinterpret_cast<std::uint32_t*>(metadata + 16u) =
            *reinterpret_cast<const std::uint32_t*>(predecessor + 16u);
    }
    // Native sub_4CD6C0 epilogue: build expected FramesMeta from RemapOutput[0]
    // replacing manager+28, then compare 20 bytes against the final predecessor.
    std::array<std::uint32_t, 5u> expected{};
    std::memcpy(expected.data(), manager + 28u, 20u);
    expected[0] = *reinterpret_cast<const std::uint32_t*>(
        manager + kManagerOffRemapOutput);
    const auto* produced = reinterpret_cast<const std::uint32_t*>(predecessor);
    if (!frames_meta_t_equal(produced, expected.data()))
        return 1;
    if (initial != 0) {
        // Native calls sub_4CC230 (latency/gain table refresh) on initial
        // activation. Keep the refresh in the same phase as native.
        if (const auto info_rc = manager_refresh_info_native(manager); info_rc != 0)
            return info_rc;
    } else {
        *reinterpret_cast<std::uint32_t*>(manager + kManagerOffLayoutEqual) = 1u;
    }
    *reinterpret_cast<std::uint32_t*>(manager + kManagerOffLayoutReady) = 1u;
    return 0;
}

std::int64_t auro_a3deng_v3_pipeline_Manager_configure_plan_entry(
    std::uint64_t manager_u64,
    std::uint32_t initial) {
    auto* manager = reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(manager_u64));
    std::array<std::uint32_t, kPlanBufferBytes / sizeof(std::uint32_t)> plan{};
    auto* creator = reinterpret_cast<std::uint32_t*>(manager + kManagerOffPlanCreator);
    const auto make_rc = auro_a3deng_v3_pipeline_plan_Creator_make(
        creator,
        plan.data(),
        reinterpret_cast<std::int64_t>(manager + kManagerOffPlanScratch),
        manager + kManagerOffBlockRecord,
        reinterpret_cast<std::uint64_t>(manager + kManagerOffRemapOutput));
    if (make_rc != 0)
        return make_rc;
    const auto activate_rc = auro_a3deng_v3_pipeline_Manager_activate_plan(
        manager, static_cast<std::int32_t>(initial), plan.data());
    return activate_rc != 0 ? 500 + activate_rc : 0;
}

bool auromatic_plan_creator_self_test(std::string& detail) {
    if (soundbar_category_from_layout_portable(0x3u) != 0u
        || soundbar_category_from_layout_portable(0x633u) != 1u
        || soundbar_category_from_layout_portable(0x600030u) != 2u
        || preset_family_from_native_fields(2u, 0x3u) != 0u
        || preset_family_from_native_fields(1u, 0x633u) != 2u
        || preset_family_from_native_fields(1u, 0x600030u) != 3u
        || preset_family_from_native_fields(3u, 0x3u) != 4u) {
        detail = "PresetManager family dispatch mismatch";
        return false;
    }
    // PresetManager_configure selects every family/variant/room combination
    // through the native address table.  Exercise the complete matrix here,
    // including Speech (native room slot 4), rather than validating only the
    // default multichannel preset.
    constexpr std::uint32_t kPresetTestTargets[] = {
        0x603u, 0x633u, 0x63Fu, 0x7BFu, 0x6633u, 0x6637u, 0x663Fu,
        0x677Fu, 0x763Fu, 0x77BFu, 0x7E3Fu, 0x7FBFu,
    };
    for (const auto target_layout : kPresetTestTargets) {
        for (std::uint32_t family = 0u; family < 5u; ++family) {
            for (std::uint32_t surround = 0u; surround < 2u; ++surround) {
                const std::uint32_t source_layout = surround != 0u ? 0x3Fu : 0x3u;
                // Configurator never selects the surround XinN branch for a
                // stereo source, and an upmix target cannot discard channels
                // from its input bed. Keep the matrix exhaustive only across
                // semantically reachable source/target pairs.
                if ((target_layout & source_layout) != source_layout)
                    continue;
                for (std::uint32_t room = 0u; room < 5u; ++room) {
                    std::array<std::uint32_t, 64u> engine_params{};
                    if (build_engine_params_for_upmix_plan_family(
                            engine_params.data(), source_layout, target_layout, 48000u,
                            surround != 0u, room, family) != 0
                        || engine_params[0u] != source_layout
                        || engine_params[1u] == 0u) {
                        detail = "PresetManager exhaustive layout/family/room matrix mismatch target="
                            + std::to_string(target_layout)
                            + " family=" + std::to_string(family)
                            + " surround=" + std::to_string(surround)
                            + " room=" + std::to_string(room)
                            + " output=" + std::to_string(engine_params[1u]);
                        return false;
                    }
                }
            }
        }
    }
    {
        // ChannelTest::configure/process @ 0x4D60E0/0x4D6150.  The plan
        // record is {active, source_layout, target_layout}; its enable rule
        // and the 1/sqrt(2) center synthesis are independent of the normal
        // predecessor metadata record.
        std::array<std::uint8_t, 512u> channel_test{};
        manager_construct_channel_test_step(channel_test.data());
        std::array<std::uint32_t, 3u> channel_plan{1u, 0x1BFu, 0x7Fu};
        std::array<std::uint32_t, 3u> block_record{0x7Fu, 1u, 0x7Fu};
        *reinterpret_cast<std::uint64_t*>(channel_test.data() + 32u) =
            reinterpret_cast<std::uint64_t>(block_record.data());
        if (manager_configure_channel_test_step(
                channel_test.data(),
                reinterpret_cast<const std::uint8_t*>(channel_plan.data())) != 0
            || *reinterpret_cast<const std::uint32_t*>(channel_test.data() + 64u) != 1u) {
            detail = "ChannelTest configure mismatch";
            return false;
        }
        std::array<std::array<float, 32u>, 31u> channel_data{};
        for (std::uint32_t i = 0u; i < 32u; ++i) {
            channel_data[7u][i] = 1.0f;
            channel_data[8u][i] = 0.5f;
        }
        std::array<void*, 31u> channel_io{};
        for (std::uint32_t id = 0u; id < 31u; ++id)
            channel_io[id] = channel_data[id].data();
        if (manager_process_channel_test_step(
                channel_test.data(), channel_io.data()) != 0
            || std::fabs(channel_data[6u][0u] - 1.0606601718f) > 1.0e-6f) {
            detail = "ChannelTest center synthesis mismatch";
            return false;
        }
    }
    {
        // p3D3D's first rate predicate accepts all ten native codec rates,
        // while the callback-rate predicate rejects Line callbacks outside
        // 32/44.1/48 kHz.  Exercise both sides of that exact split.
        constexpr std::uint32_t kNativeRates[] = {
            16000u, 22050u, 24000u, 32000u, 44100u,
            48000u, 88200u, 96000u, 176400u, 192000u,
        };
        for (const auto rate : kNativeRates) {
            std::array<std::uint8_t, 520u> step{};
            std::array<std::uint32_t, 64u> config{};
            if (manager_configure_upmix_3d3d(
                    step.data(), rate, config.data()) != 0) {
                detail = "p3D3D native empty-mode rate rejected";
                return false;
            }
        }
        if (auromatic_3d3d_callback_rate(96000u)
            || !auromatic_3d3d_callback_rate(48000u)) {
            detail = "p3D3D callback-rate predicate mismatch";
            return false;
        }
        std::array<std::uint8_t, 14784u> callback_step_storage{};
        std::array<float, 590u> line2_storage{};
        std::array<float, 1496u> line3a_storage{};
        std::array<float, 1496u> line3b_storage{};
        std::array<float, 416u> line4_storage{};
        auto* callback_step = callback_step_storage.data();
        auto* callback_state = callback_step + 40u;
        *reinterpret_cast<float**>(callback_state + 368u) = line2_storage.data();
        *reinterpret_cast<float**>(callback_state + 376u) = line3a_storage.data();
        *reinterpret_cast<float**>(callback_state + 384u) = line3b_storage.data();
        *reinterpret_cast<float**>(callback_state + 392u) = line4_storage.data();
        std::array<float, 64u> callback_coefficients{};
        if (!manager_construct_upmix_3d3d(callback_step)) {
            detail = "p3D3D callback state setup failed";
            return false;
        }
        std::uint64_t callback_actual = 0u;
        std::uint32_t callback_actual_count = 0u;
        std::int64_t callback_config_rc = 0;
        const auto check_callback = [&](std::uint32_t mode,
                                        std::uint32_t second,
                                        std::uint32_t flag,
                                        std::uint64_t expected) {
            std::array<std::uint32_t, 16u> cfg{};
            cfg[0u] = mode;
            cfg[1u] = second;
            *reinterpret_cast<std::uint64_t*>(cfg.data() + 8u) =
                reinterpret_cast<std::uint64_t>(callback_coefficients.data());
            if (flag != 0u)
                cfg[flag] = 1u;
            callback_config_rc = manager_configure_upmix_3d3d(
                callback_step, 48000u, cfg.data());
            if (callback_config_rc != 0)
                return false;
            const auto* state = callback_step + 40u;
            callback_actual_count = *reinterpret_cast<const std::uint32_t*>(state + 344u);
            callback_actual = *reinterpret_cast<const std::uint64_t*>(state + 280u);
            return *reinterpret_cast<const std::uint32_t*>(state + 344u) == 1u
                && *reinterpret_cast<const std::uint64_t*>(state + 280u)
                    == expected;
        };
        if (!check_callback(1u, 0u, 0u, 0x5513D0u)
            || !check_callback(1u, 1u, 0u, 0x551330u)
            || !check_callback(2u, 0u, 0u, 0x5514F0u)
            || !check_callback(2u, 1u, 0u, 0x551480u)
            || !check_callback(0u, 1u, 1u, 0x551580u)
            || !check_callback(0u, 0u, 6u, 0x551630u)
            || !check_callback(0u, 0u, 2u, 0x5516B0u)
            || !check_callback(0u, 0u, 3u, 0x551720u)
            || !check_callback(0u, 0u, 4u, 0x551790u)
            || !check_callback(0u, 0u, 5u, 0x551800u)) {
            detail = "p3D3D callback matrix mismatch count="
                + std::to_string(callback_actual_count)
                + " id=" + std::to_string(callback_actual)
                + " rc=" + std::to_string(callback_config_rc);
            return false;
        }
        {
            // Native configure() replaces a null Config[4] with its static
            // 14-float coefficient block, even after a previous configure.
            *reinterpret_cast<float**>(callback_state + 272u) = nullptr;
            std::array<std::uint32_t, 16u> fallback_cfg{};
            fallback_cfg[0u] = 1u;
            if (manager_configure_upmix_3d3d(
                    callback_step, 48000u, fallback_cfg.data()) != 0
                || *reinterpret_cast<float* const*>(callback_state + 272u)
                    == nullptr) {
                detail = "p3D3D null coefficient fallback mismatch";
                return false;
            }
        }
        {
            std::array<std::uint32_t, 16u> rejected_cfg{};
            rejected_cfg[0u] = 2u;
            rejected_cfg[2u] = 1u;
            *reinterpret_cast<std::uint64_t*>(rejected_cfg.data() + 8u) =
                reinterpret_cast<std::uint64_t>(callback_coefficients.data());
            if (manager_configure_upmix_3d3d(
                    callback_step, 48000u, rejected_cfg.data()) == 0) {
                detail = "p3D3D mode-2 flag-2 was accepted";
                return false;
            }
            if (manager_configure_upmix_3d3d(
                    callback_step, 24000u, rejected_cfg.data()) == 0) {
                detail = "p3D3D callback mode was accepted at 24 kHz";
                return false;
            }
            std::array<std::uint32_t, 16u> conflicting_flags{};
            conflicting_flags[3u] = 1u;
            conflicting_flags[5u] = 1u;
            *reinterpret_cast<std::uint64_t*>(conflicting_flags.data() + 8u) =
                reinterpret_cast<std::uint64_t>(callback_coefficients.data());
            if (manager_configure_upmix_3d3d(
                    callback_step, 48000u, conflicting_flags.data()) == 0) {
                detail = "p3D3D mode-0 flag-3/flag-5 conflict was accepted";
                return false;
            }
        }
        const auto check_appended_callback = [&](std::uint32_t mode,
                                                  std::uint32_t flag,
                                                  std::uint64_t base_callback,
                                                  std::uint64_t appended_callback) {
            std::array<std::uint32_t, 16u> cfg{};
            cfg[0u] = mode;
            cfg[1u] = 0u;
            cfg[flag] = 1u;
            *reinterpret_cast<std::uint64_t*>(cfg.data() + 8u) =
                reinterpret_cast<std::uint64_t>(callback_coefficients.data());
            const auto rc = manager_configure_upmix_3d3d(
                callback_step, 48000u, cfg.data());
            if (rc != 0) {
                detail = "p3D3D appended configure mode="
                    + std::to_string(mode) + " flag="
                    + std::to_string(flag) + " rc=" + std::to_string(rc);
                return false;
            }
            const auto* state = callback_step + 40u;
            const auto count = *reinterpret_cast<const std::uint32_t*>(state + 344u);
            const auto first = *reinterpret_cast<const std::uint64_t*>(state + 280u);
            const auto second = *reinterpret_cast<const std::uint64_t*>(state + 288u);
            if (count != 2u || first != base_callback || second != appended_callback) {
                detail = "p3D3D appended state mode=" + std::to_string(mode)
                    + " flag=" + std::to_string(flag)
                    + " count=" + std::to_string(count)
                    + " first=" + std::to_string(first)
                    + " second=" + std::to_string(second);
                return false;
            }
            return true;
        };
        if (!check_appended_callback(1u, 6u, 0x5513D0u, 0x551630u)
            || !check_appended_callback(1u, 2u, 0x5513D0u, 0x5516B0u)
            || !check_appended_callback(1u, 3u, 0x5513D0u, 0x551720u)
            || !check_appended_callback(1u, 4u, 0x5513D0u, 0x551790u)
            || !check_appended_callback(1u, 5u, 0x5513D0u, 0x551800u)
            || !check_appended_callback(2u, 6u, 0x5514F0u, 0x551630u)
            || !check_appended_callback(2u, 3u, 0x5514F0u, 0x551720u)
            || !check_appended_callback(2u, 4u, 0x5514F0u, 0x551790u)
            || !check_appended_callback(2u, 5u, 0x5514F0u, 0x551800u)) {
            if (detail.empty())
                detail = "p3D3D appended callback matrix mismatch";
            return false;
        }
    }
    {
        // BassManager uses native biquad types 2 and 16.  Verify that the
        // float32 coefficient bridge is populated and finite for both.
        std::array<std::uint8_t, 16u> filter_config{};
        *reinterpret_cast<float*>(filter_config.data() + 4u) = 1000.0f;
        *reinterpret_cast<float*>(filter_config.data() + 8u) = 0.707f;
        std::array<std::uint8_t, 48u> filter_coefficients{};
        *reinterpret_cast<std::uint32_t*>(filter_config.data()) = 2u;
        if (auro_iir_biquad_parameter_Config_float32_t_compute(
                filter_config.data(), 48000u, filter_coefficients.data()) != 0
            || !std::isfinite(*reinterpret_cast<const float*>(
                filter_coefficients.data()))
            || *reinterpret_cast<const float*>(filter_coefficients.data()) == 0.0f) {
            detail = "native biquad type-2 coefficient mismatch";
            return false;
        }
        *reinterpret_cast<std::uint32_t*>(filter_config.data()) = 16u;
        filter_coefficients.fill(0u);
        if (auro_iir_biquad_parameter_Config_float32_t_compute(
                filter_config.data(), 48000u, filter_coefficients.data()) != 0
            || !std::isfinite(*reinterpret_cast<const float*>(
                filter_coefficients.data()))) {
            detail = "native biquad type-16 coefficient mismatch";
            return false;
        }
    }
    {
        std::array<std::uint8_t, 1800u> downmix_step{};
        std::array<std::uint32_t, 64u> downmix_plan{};
        downmix_plan[0u] = 0x3u;
        downmix_plan[1u] = 0x3u;
        downmix_plan[2u] = 1u;
        downmix_plan[4u] = 0u;
        downmix_plan[5u] = 1u;
        *reinterpret_cast<float*>(downmix_plan.data() + 6u) = 0.5f;
        if (manager_configure_downmixer_step(
                downmix_step.data(), downmix_plan.data(), true, nullptr) != 0) {
            detail = "Downmixer configure failed";
            return false;
        }
        std::array<float, 32u> source{};
        std::array<float, 32u> destination{};
        source.fill(2.0f);
        destination.fill(1.0f);
        void* downmix_io[31u]{};
        downmix_io[0u] = source.data();
        downmix_io[1u] = destination.data();
        std::array<std::uint32_t, 4u> downmix_block_state{};
        downmix_block_state[1u] = 1u;
        *reinterpret_cast<std::uint64_t*>(downmix_step.data() + 8u) =
            reinterpret_cast<std::uint64_t>(downmix_block_state.data());
        if (manager_process_downmixer(
                downmix_step.data(), downmix_io) != 0
            || std::fabs(destination[0u] - 2.0f) > 1.0e-6f) {
            detail = "Downmixer row accumulation mismatch";
            return false;
        }
    }
    {
        // BassManager receives the optional object, not its active flag.
        // Validate the native layout-mask route enumeration and peak path.
        std::array<std::uint8_t, 960u> bass_step{};
        std::array<std::uint32_t, 64u> bass_plan{};
        bass_plan[0u] = 3u;
        bass_plan[1u] = 11u;
        if (manager_configure_bass_management_step(
                bass_step.data(), bass_plan.data(), 48000u) != 0
            || *reinterpret_cast<const std::uint32_t*>(
                bass_step.data() + 64u + 628u) != 2u
            || *reinterpret_cast<const std::uint32_t*>(
                bass_step.data() + 64u + 744u) != 3u) {
            detail = "BassManager route configuration mismatch";
            return false;
        }
        std::array<float, 31u> bass_peaks{};
        bass_peaks[1u] = 0.25f;
        bass_peaks[2u] = 0.5f;
        bass_peaks[3u] = 0.75f;
        if (manager_update_peak_bass_management(
                bass_step.data(), bass_peaks.data()) != 0
            || std::fabs(bass_peaks[3u]) > 1.0e-6f) {
            detail = "BassManager peak update mismatch";
            return false;
        }
    }
    {
        // Configurator setter offsets from 0x4DB9C0..0x4DBA40.  Exercise the
        // complete preset/parameter pointer surface, not only the default
        // multichannel path used by the plan smoke test below.
        std::array<std::uint8_t, 248u> configurator{};
        std::array<std::uint8_t, 76u> centergen{};
        for (std::uint32_t i = 0u; i < centergen.size(); ++i)
            centergen[i] = static_cast<std::uint8_t>(i * 3u + 1u);
        auro_remix_v1_Configurator_set_centergen_preset(
            configurator.data(), centergen.data());
        auro_remix_v1_Configurator_set_XinN_stereo_preset(
            configurator.data(), 0x0102030405060708ull);
        auro_remix_v1_Configurator_set_XinN_surround_preset(
            configurator.data(), 0x1112131415161718ull);
        auro_remix_v1_Configurator_set_XinN_stereo_parameters(
            configurator.data(), 0x2122232425262728ull);
        auro_remix_v1_Configurator_set_XinN_surround_parameters(
            configurator.data(), 0x3132333435363738ull);
        auro_remix_v1_Configurator_set_3d3d_preset(
            configurator.data(), 0x4142434445464748ull);
        auro_remix_v1_Configurator_set_force_front_manager_reset(
            configurator.data(), 1u);
        if (std::memcmp(configurator.data() + 72u, centergen.data(), 76u) != 0
            || *reinterpret_cast<const std::uint64_t*>(configurator.data() + 152u)
                != 0x0102030405060708ull
            || *reinterpret_cast<const std::uint64_t*>(configurator.data() + 160u)
                != 0x1112131415161718ull
            || *reinterpret_cast<const std::uint64_t*>(configurator.data() + 168u)
                != 0x2122232425262728ull
            || *reinterpret_cast<const std::uint64_t*>(configurator.data() + 176u)
                != 0x3132333435363738ull
            || *reinterpret_cast<const std::uint64_t*>(configurator.data() + 184u)
                != 0x4142434445464748ull
            || *reinterpret_cast<const std::uint32_t*>(configurator.data() + 192u) != 1u) {
            detail = "Configurator preset setter offsets mismatch";
            return false;
        }
    }
    std::array<std::uint8_t, 128u> config{};
    *reinterpret_cast<std::uint32_t*>(config.data() + 0u) = 0x3u;
    *reinterpret_cast<std::uint32_t*>(config.data() + 4u) = 32u;
    *reinterpret_cast<std::uint32_t*>(config.data() + 8u) = 48000u;
    *reinterpret_cast<std::uint32_t*>(config.data() + 20u) = 0x6633u;
    *reinterpret_cast<std::uint32_t*>(config.data() + 24u) = 32u;
    *reinterpret_cast<std::uint32_t*>(config.data() + 28u) = 48000u;
    *reinterpret_cast<std::uint32_t*>(config.data() + 60u) = 0u;
    *reinterpret_cast<std::uint32_t*>(config.data() + 100u) = 0x6633u;
    if (!auro_speaker_layout_is_consistent(0x3u, 13)) {
        detail = "stereo layout failed schema-13 consistency";
        return false;
    }
    if (!auro_speaker_layout_is_consistent(0x6633u, 14)) {
        detail = "5.1 layout failed schema-14 consistency";
        return false;
    }
    std::array<std::uint8_t, kRemixPlanWorkspaceBytes> workspace{};
    std::array<std::uint8_t, 248u> remix{};
    auro_remix_v1_Configurator_t_construct(remix.data());
    const auto* matic = remix.data() + 40u;
    if (*reinterpret_cast<const std::uint64_t*>(remix.data() + 8u)
            != reinterpret_cast<std::uint64_t>(matic)
        || *reinterpret_cast<const std::uint64_t*>(matic + 0u)
            != reinterpret_cast<std::uint64_t>(matic)
        || *reinterpret_cast<const std::uint64_t*>(matic + 8u) != 0x54F890u
        || *reinterpret_cast<const std::uint64_t*>(matic + 16u) != 0x54F8E0u
        || *reinterpret_cast<const std::uint64_t*>(matic + 24u) != 0x54F830u) {
        detail = "embedded Matic Configurator constructor mismatch";
        return false;
    }
    auro_remix_v1_Configurator_set_listening_mode(remix.data(), 0u);
    auro_remix_v1_Configurator_set_device(remix.data(), 1u);
    if (const auto rc = auro_remix_v1_Configurator_make_plan(
            remix.data(), workspace.data(), 0x3u, 0x6633u)) {
        detail = "Configurator_make_plan stereo->5.1 failed rc=" + std::to_string(rc);
        return false;
    }
    if (*reinterpret_cast<const std::uint32_t*>(workspace.data() + 1388u) == 0u) {
        detail = "Configurator_make_plan did not activate upmix";
        return false;
    }
    // Exercise the native PresetManager argument order used by
    // Plan_configure_remixer: family selector (a6) and room (a3).  This is
    // intentionally done through make_plan, not only through the lower-level
    // XinN resource table, so a room can no longer be silently discarded.
    for (const std::uint32_t preset_kind : {1u, 2u, 3u}) {
        for (std::uint32_t room = 0u; room < 5u; ++room) {
            std::array<std::uint8_t, kRemixPlanWorkspaceBytes> variant_workspace{};
            if (auro_remix_v1_Configurator_make_plan(
                    remix.data(), variant_workspace.data(), 0x3u, 0x6633u,
                    preset_kind, room) != 0) {
                detail = "Configurator_make_plan family/room matrix failed family="
                    + std::to_string(preset_kind) + " room="
                    + std::to_string(room);
                return false;
            }
        }
    }
    // project_output_layout accepts native device IDs 0,1,2,3,5,6; ID 4
    // falls through the native mask switch and is rejected, as are values
    // above 6. Exercise accepted IDs through the remixer orchestration so
    // device-specific Bass/Alt3D projection is not covered only by a helper.
    for (const std::uint32_t device : {0u, 1u, 2u, 3u, 5u, 6u}) {
        auro_remix_v1_Configurator_set_device(remix.data(), device);
        std::array<std::uint8_t, kRemixPlanWorkspaceBytes> device_workspace{};
        if (auro_remix_v1_Configurator_make_plan(
                remix.data(), device_workspace.data(), 0x3u, 0x6633u,
                3u, 0u) != 0) {
            detail = "Configurator_make_plan device matrix failed device="
                + std::to_string(device);
            return false;
        }
    }
    for (const std::uint32_t device : {4u, 7u}) {
        auro_remix_v1_Configurator_set_device(remix.data(), device);
        std::array<std::uint8_t, kRemixPlanWorkspaceBytes> rejected_workspace{};
        if (auro_remix_v1_Configurator_make_plan(
                remix.data(), rejected_workspace.data(), 0x3u, 0x6633u,
                3u, 0u) == 0) {
            detail = "Configurator_make_plan accepted native-invalid device="
                + std::to_string(device);
            return false;
        }
    }
    auro_remix_v1_Configurator_set_device(remix.data(), 1u);
    std::array<std::uint32_t, 16u> creator{};
    creator[0] = 1u;
    creator[2] = 4u;
    creator[3] = 1u;
    std::array<std::uint32_t, kPlanBufferBytes / sizeof(std::uint32_t)> plan{};
    if (const auto rc = auro_a3deng_v3_pipeline_plan_Creator_make(
            creator.data(),
            plan.data(),
            0,
            config.data(),
            0u)) {
        detail = "Plan_Creator_make failed rc=" + std::to_string(rc);
        return false;
    }
    if (plan[364] == 0u || plan[602] == 0u || plan[603] == 0u
        || plan[605] == 0u || plan[608] != 4u
        || plan[636] != 1u || plan[637] != 1u || plan[638] != creator[3]) {
        detail = "Plan_Creator_make missing expected active steps";
        return false;
    }
    const auto* engine_in_plan =
        reinterpret_cast<const std::uint32_t*>(reinterpret_cast<const std::uint8_t*>(plan.data()) + 1464u);
    if (engine_in_plan[0] == 0u || engine_in_plan[1] == 0u) {
        detail = "Plan upmix engine payload empty at +1464";
        return false;
    }
    if (!auro_speaker_layout_is_consistent(0x3u, 13)
        || !auro_speaker_layout_is_consistent(0x6633u, 14)) {
        detail = "speaker_layout_is_consistent mismatch";
        return false;
    }
    detail = "Plan_Creator_make and Configurator_make_plan PASS";
    AuromaticV3UpmixRuntime runtime;
    if (runtime.construct() != 0) {
        detail = "runtime construct failed";
        return false;
    }
    if (const auto via_rc = runtime.configure_via_manager(
            48000u, 0x3u, 0x6633u, false, 0u)) {
        detail = "configure_via_manager failed rc=" + std::to_string(via_rc);
        return false;
    }
    if (*reinterpret_cast<const std::uint32_t*>(
            runtime.manager_storage() + 480u)
        == 0u) {
        detail = "FramesMeta equal did not mark layout ready";
        return false;
    }
    const auto* upmix = runtime.step_chain_storage() + 7944u;
    if (*reinterpret_cast<const std::uint32_t*>(upmix + 40u) == 0u) {
        detail = "activation did not configure Upmix output mask";
        return false;
    }
    const auto* headroom = runtime.step_chain_storage() + 15800u;
    const auto* gate = runtime.step_chain_storage() + 16176u;
    const auto* peak = runtime.step_chain_storage() + 16248u;
    if (*reinterpret_cast<const std::uint64_t*>(headroom + 32u) == 0u
        || *reinterpret_cast<const std::uint32_t*>(headroom + 372u) != 48000u) {
        detail = "Headroom configure did not bind rate/predecessor";
        return false;
    }
    if (*reinterpret_cast<const std::uint64_t*>(gate + 32u) == 0u
        || *reinterpret_cast<const std::uint64_t*>(gate + 64u)
            != 0x3F8000003F800000ull) {
        detail = "Gate configure did not bind unity gains";
        return false;
    }
    if (plan[604] != 0u
        && (*reinterpret_cast<const std::uint64_t*>(peak + 32u) == 0u
            || *reinterpret_cast<const std::uint32_t*>(peak + 124u) == 0u
            || *reinterpret_cast<const std::uint32_t*>(peak + 128u) != 0x504C4632u)) {
        detail = "PeakLimiter configure did not build channel list/embed";
        return false;
    }
    const auto* sample_convertor = runtime.step_chain_storage() + 61784u;
    if (*reinterpret_cast<const std::uint64_t*>(sample_convertor + 32u) == 0u
        || *reinterpret_cast<const std::uint64_t*>(sample_convertor + 16u)
            != ::auro_engine_v4_ida::kStepSampleConvertor_process_float_clamp) {
        detail = "SampleConvertor configure did not install float±1 clamp";
        return false;
    }
    // Isolated PeakLimiter / SampleConvertor process (do not call
    // runtime.process here — Engine paths still need a full channel set).
    {
        std::array<std::uint8_t, 256u> step{};
        *reinterpret_cast<std::uint32_t*>(step.data() + 40u) = 0x3u;
        *reinterpret_cast<std::uint32_t*>(step.data() + 44u) = 1u;
        *reinterpret_cast<std::uint32_t*>(step.data() + 48u) = 48000u;
        *reinterpret_cast<std::uint64_t*>(step.data() + 32u) =
            reinterpret_cast<std::uint64_t>(step.data() + 40u);
        if (manager_configure_peak_limiter_step(step.data(), nullptr, true, nullptr) != 0
            || *reinterpret_cast<const std::uint32_t*>(step.data() + 128u) != 0x504C4632u) {
            detail = "isolated PeakLimiter configure failed";
            return false;
        }
        std::array<float, 32u> left{};
        std::array<float, 32u> right{};
        left.fill(2.0f);
        right.fill(2.0f);
        void* io[31u]{};
        io[0] = left.data();
        io[1] = right.data();
        if (manager_process_peak_limiter(step.data(), io) != 0) {
            detail = "isolated PeakLimiter process failed";
            return false;
        }
        // Look-ahead ramp starts at prev_gain=1; the block tail must settle.
        if (!(std::fabs(left[31]) < 1.05f) || !(std::fabs(right[31]) < 1.05f)) {
            detail = "PeakLimiter tail not limited L="
                + std::to_string(left[31]) + " R=" + std::to_string(right[31]);
            return false;
        }
        if (!(std::fabs(left[0]) > 1.5f)) {
            detail = "PeakLimiter unexpectedly crushed block head";
            return false;
        }
    }
    {
        std::array<std::uint8_t, 64u> step{};
        *reinterpret_cast<std::uint32_t*>(step.data() + 40u) = 0x3u;
        *reinterpret_cast<std::uint32_t*>(step.data() + 44u) = 1u;
        *reinterpret_cast<std::uint32_t*>(step.data() + 48u) = 48000u;
        *reinterpret_cast<std::uint32_t*>(step.data() + 52u) = 0u;
        *reinterpret_cast<std::uint64_t*>(step.data() + 32u) =
            reinterpret_cast<std::uint64_t>(step.data() + 40u);
        const std::uint32_t plan_entry[4] = {1u, 0u, 0u, 4u};
        if (manager_configure_sample_convertor_step(
                step.data(), plan_entry, true, nullptr)
            != 0
            || *reinterpret_cast<const std::uint64_t*>(step.data() + 16u)
                != ::auro_engine_v4_ida::kStepSampleConvertor_process_float_clamp) {
            detail = "isolated SampleConvertor configure failed";
            return false;
        }
        std::array<float, 32u> left{};
        std::array<float, 32u> right{};
        left.fill(1.5f);
        right.fill(-1.25f);
        left[0] = 0.5f;
        right[0] = -0.25f;
        void* io[31u]{};
        io[0] = left.data();
        io[1] = right.data();
        if (manager_process_sample_convertor(step.data(), io) != 0) {
            detail = "isolated SampleConvertor process failed";
            return false;
        }
        if (!(left[0] == 0.5f) || !(right[0] == -0.25f)
            || !(left[1] == 1.0f) || !(right[1] == -1.0f)) {
            detail = "SampleConvertor float±1 clamp mismatch";
            return false;
        }
    }
    {
        // sub_4CF840 selects 0x4CFF00 for a Float32 predecessor, PCM24
        // converter type 1, and saturation enabled. The callback overwrites
        // planar Float32 storage with truncated, signed PCM24 int32 values.
        std::array<std::uint8_t, 64u> step{};
        *reinterpret_cast<std::uint32_t*>(step.data() + 40u) = 0x1u;
        *reinterpret_cast<std::uint32_t*>(step.data() + 44u) = 1u;
        *reinterpret_cast<std::uint32_t*>(step.data() + 52u) = 0u;
        *reinterpret_cast<std::uint32_t*>(step.data() + 56u) = 32u;
        *reinterpret_cast<std::uint64_t*>(step.data() + 32u) =
            reinterpret_cast<std::uint64_t>(step.data() + 40u);
        const std::uint32_t plan_entry[4] = {1u, 0u, 1u, 1u};
        if (manager_configure_sample_convertor_step(
                step.data(), plan_entry, true, nullptr)
            != 0
            || *reinterpret_cast<const std::uint64_t*>(step.data() + 16u)
                != ::auro_engine_v4_ida::kStepSampleConvertor_process_pcm24_sat) {
            detail = "isolated PCM24 SampleConvertor configure failed";
            return false;
        }
        std::array<float, 32u> samples{};
        samples[0] = 1.0f;
        samples[1] = -1.0f;
        samples[2] = 0.5f;
        samples[3] = 1.5f / 8388608.0f;
        samples[4] = -1.5f / 8388608.0f;
        void* io[31u]{};
        io[0] = samples.data();
        if (manager_process_sample_convertor(step.data(), io) != 0) {
            detail = "isolated PCM24 SampleConvertor process failed";
            return false;
        }
        const auto* pcm = reinterpret_cast<const std::int32_t*>(samples.data());
        if (pcm[0] != 8388607 || pcm[1] != -8388608 || pcm[2] != 4194304
            || pcm[3] != 1 || pcm[4] != -1) {
            detail = "PCM24 SampleConvertor truncation/saturation mismatch";
            return false;
        }
    }
    {
        // Isolated factor-2 Up then Down round-trip on one hot block.
        std::array<std::uint8_t, 2048u> up_step{};
        std::array<std::uint8_t, 2048u> down_step{};
        manager_construct_upsample_step(up_step.data());
        manager_construct_downsample_step(down_step.data());
        *reinterpret_cast<std::uint32_t*>(up_step.data() + 40u) = 0x1u;
        *reinterpret_cast<std::uint32_t*>(up_step.data() + 44u) = 1u;
        *reinterpret_cast<std::uint32_t*>(up_step.data() + 48u) = 48000u;
        *reinterpret_cast<std::uint32_t*>(up_step.data() + 52u) = 0u;
        *reinterpret_cast<std::uint64_t*>(up_step.data() + 32u) =
            reinterpret_cast<std::uint64_t>(up_step.data() + 40u);
        if (manager_configure_upsample_step(up_step.data(), nullptr, true, nullptr) != 0
            || *reinterpret_cast<const std::uint32_t*>(up_step.data() + 44u) != 2u
            || *reinterpret_cast<const std::uint32_t*>(up_step.data() + 48u) != 96000u) {
            detail = "isolated UpSample configure failed";
            return false;
        }
        std::array<float, 64u> mono{};
        for (std::uint32_t i = 0u; i < 32u; ++i)
            mono[i] = (i == 0u) ? 1.0f : 0.0f;
        void* io[31u]{};
        io[0] = mono.data();
        if (manager_process_upsample(up_step.data(), io) != 0) {
            detail = "isolated UpSample process failed";
            return false;
        }
        // Native has a one-output-sample phase offset: out[0] is the zero
        // history, out[1]=2*h[0]*in[0], out[2]=2*h[1]*in[0].
        const float expect_first = 2.0f * matic_resample::kFactor2Tap26[0];
        const float expect_second = 2.0f * matic_resample::kFactor2Tap26[1];
        if (mono[0] != 0.0f
            || !(std::fabs(mono[1] - expect_first) < 1e-5f)
            || !(std::fabs(mono[2] - expect_second) < 1e-5f)) {
            detail = "UpSample impulse phase mismatch got "
                + std::to_string(mono[0]) + "/" + std::to_string(mono[1])
                + "/" + std::to_string(mono[2]);
            return false;
        }
        float up_peak = 0.0f;
        for (float v : mono)
            up_peak = std::max(up_peak, std::fabs(v));
        if (!(up_peak > 0.01f) || !std::isfinite(up_peak)) {
            detail = "UpSample impulse peak too small: " + std::to_string(up_peak);
            return false;
        }
        *reinterpret_cast<std::uint32_t*>(down_step.data() + 40u) = 0x1u;
        *reinterpret_cast<std::uint32_t*>(down_step.data() + 44u) = 2u;
        *reinterpret_cast<std::uint32_t*>(down_step.data() + 48u) = 96000u;
        *reinterpret_cast<std::uint32_t*>(down_step.data() + 52u) = 0u;
        *reinterpret_cast<std::uint64_t*>(down_step.data() + 32u) =
            reinterpret_cast<std::uint64_t>(down_step.data() + 40u);
        if (manager_configure_downsample_step(down_step.data(), nullptr, true, nullptr) != 0
            || *reinterpret_cast<const std::uint32_t*>(down_step.data() + 44u) != 1u
            || *reinterpret_cast<const std::uint32_t*>(down_step.data() + 48u) != 48000u) {
            detail = "isolated DownSample configure failed";
            return false;
        }
        if (manager_process_downsample(down_step.data(), io) != 0) {
            detail = "isolated DownSample process failed";
            return false;
        }
        float down_peak = 0.0f;
        for (std::uint32_t i = 0u; i < 32u; ++i)
            down_peak = std::max(down_peak, std::fabs(mono[i]));
        if (!(down_peak > 0.01f) || !std::isfinite(down_peak)) {
            detail = "DownSample impulse peak too small: " + std::to_string(down_peak);
            return false;
        }
    }
    detail = "Plan_Creator_make and activation PASS";
    return true;
}
