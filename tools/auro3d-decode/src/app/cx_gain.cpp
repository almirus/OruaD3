#include "cx_gain.hpp"

#include <cstddef>

namespace auro3d {
namespace cx {

namespace {

template <typename References>
AuroCxIntegralGainInfo find_reference_gain(
    const References& references,
    std::size_t target_index) {
    for (const auto& reference : references)
        if (reference.index == target_index)
            return reference.gain;
    return {};
}

template <typename References>
bool contains_reference(const References& references, std::size_t target_index) {
    for (const auto& reference : references)
        if (reference.index == target_index)
            return true;
    return false;
}

bool contains_all_object_references(
    const std::vector<AuroCxSchemaReferenceInfo>& references,
    const std::vector<std::uint32_t>& linked_object_groups) {
    for (const std::uint32_t group : linked_object_groups)
        if (!contains_reference(references, group))
            return false;
    return true;
}

bool find_reference_location_in_program(
    const CxSchemaParseResult& schema,
    std::size_t program_index,
    std::size_t bed_index,
    const std::vector<std::uint32_t>& linked_object_groups,
    ReferenceGainSelection& selection) {
    if (program_index >= schema.programs.size())
        return false;
    const auto& program = schema.programs[program_index];
    if (contains_reference(program.bed_references, bed_index) &&
        contains_all_object_references(
            program.object_group_references, linked_object_groups)) {
        selection = {};
        selection.program_index = program_index;
        return true;
    }
    for (const std::uint32_t switch_group_index : program.switch_groups) {
        if (switch_group_index >= schema.switch_groups.size())
            return false;
        const auto& switch_group = schema.switch_groups[switch_group_index];
        for (std::size_t element_index = 0;
             element_index < switch_group.elements.size(); ++element_index) {
            const auto& element = switch_group.elements[element_index];
            if (!contains_reference(element.bed_references, bed_index) ||
                !contains_all_object_references(
                    element.object_group_references, linked_object_groups))
                continue;
            selection = {};
            selection.program_index = program_index;
            selection.switch_group_index = switch_group_index;
            selection.switch_element_index = element_index;
            selection.use_switch_group = true;
            return true;
        }
    }
    return false;
}

} // namespace
namespace {

constexpr std::int64_t kUnity = 0x800000;
constexpr std::int64_t kGainScalers[513] = {
440240LL,
445338LL,
450495LL,
455711LL,
460988LL,
466326LL,
471726LL,
477188LL,
482714LL,
488304LL,
493958LL,
499678LL,
505464LL,
511317LL,
517237LL,
523227LL,
529285LL,
535414LL,
541614LL,
547886LL,
554230LL,
560648LL,
567140LL,
573707LL,
580350LL,
587070LL,
593868LL,
600745LL,
607701LL,
614738LL,
621856LL,
629057LL,
636341LL,
643709LL,
651163LL,
658703LL,
666331LL,
674047LL,
681852LL,
689747LL,
697734LL,
705813LL,
713986LL,
722254LL,
730617LL,
739077LL,
747635LL,
756293LL,
765050LL,
773909LL,
782870LL,
791936LL,
801106LL,
810382LL,
819766LL,
829258LL,
838861LL,
848574LL,
858400LL,
868340LL,
878395LL,
888566LL,
898856LL,
909264LL,
919793LL,
930443LL,
941217LL,
952116LL,
963141LL,
974294LL,
985576LL,
996988LL,
1008533LL,
1020211LL,
1032024LL,
1043975LL,
1056063LL,
1068292LL,
1080662LL,
1093176LL,
1105834LL,
1118639LL,
1131592LL,
1144695LL,
1157950LL,
1171359LL,
1184922LL,
1198643LL,
1212523LL,
1226563LL,
1240766LL,
1255133LL,
1269667LL,
1284369LL,
1299242LL,
1314286LL,
1329505LL,
1344900LL,
1360473LL,
1376226LL,
1392162LL,
1408283LL,
1424590LL,
1441086LL,
1457773LL,
1474653LL,
1491729LL,
1509002LL,
1526476LL,
1544151LL,
1562032LL,
1580119LL,
1598416LL,
1616925LL,
1635648LL,
1654588LL,
1673747LL,
1693128LL,
1712734LL,
1732566LL,
1752629LL,
1772923LL,
1793453LL,
1814220LL,
1835227LL,
1856478LL,
1877975LL,
1899721LL,
1921719LL,
1943972LL,
1966482LL,
1989252LL,
2012287LL,
2035588LL,
2059159LL,
2083003LL,
2107123LL,
2131522LL,
2156204LL,
2181172LL,
2206429LL,
2231978LL,
2257823LL,
2283967LL,
2310414LL,
2337168LL,
2364231LL,
2391607LL,
2419301LL,
2447315LL,
2475654LL,
2504320LL,
2533319LL,
2562654LL,
2592328LL,
2622345LL,
2652711LL,
2683428LL,
2714500LL,
2745933LL,
2777729LL,
2809894LL,
2842431LL,
2875345LL,
2908640LL,
2942320LL,
2976390LL,
3010855LL,
3045719LL,
3080987LL,
3116663LL,
3152753LL,
3189260LL,
3226190LL,
3263547LL,
3301337LL,
3339565LL,
3378235LL,
3417353LL,
3456925LL,
3496954LL,
3537447LL,
3578408LL,
3619844LL,
3661760LL,
3704161LL,
3747054LL,
3790442LL,
3834334LL,
3878733LL,
3923647LL,
3969080LL,
4015040LL,
4061532LL,
4108563LL,
4156137LL,
4204263LL,
4252946LL,
4302193LL,
4352010LL,
4402404LL,
4453381LL,
4504949LL,
4557114LL,
4609883LL,
4663263LL,
4717261LL,
4771884LL,
4827140LL,
4883036LL,
4939579LL,
4996776LL,
5054636LL,
5113166LL,
5172374LL,
5232267LL,
5292854LL,
5354142LL,
5416140LL,
5478856LL,
5542298LL,
5606475LL,
5671395LL,
5737067LL,
5803499LL,
5870700LL,
5938680LL,
6007446LL,
6077009LL,
6147378LL,
6218561LL,
6290569LL,
6363410LL,
6437095LL,
6511633LL,
6587034LL,
6663308LL,
6740466LL,
6818517LL,
6897471LL,
6977340LL,
7058134LL,
7139863LL,
7222539LL,
7306172LL,
7390774LL,
7476355LL,
7562927LL,
7650501LL,
7739090LL,
7828704LL,
7919357LL,
8011058LL,
8103822LL,
8197660LL,
8292584LL,
8388608LL,
8485744LL,
8584004LL,
8683402LL,
8783951LL,
8885664LL,
8988555LL,
9092638LL,
9197926LL,
9304433LL,
9412173LL,
9521161LL,
9631411LL,
9742937LL,
9855755LL,
9969879LL,
10085325LL,
10202108LL,
10320242LL,
10439745LL,
10560632LL,
10682918LL,
10806620LL,
10931755LL,
11058339LL,
11186389LL,
11315921LL,
11446953LL,
11579502LL,
11713587LL,
11849224LL,
11986431LL,
12125228LL,
12265631LL,
12407660LL,
12551334LL,
12696672LL,
12843693LL,
12992415LL,
13142861LL,
13295048LL,
13448997LL,
13604729LL,
13762264LL,
13921624LL,
14082829LL,
14245900LL,
14410860LL,
14577730LL,
14746532LL,
14917289LL,
15090023LL,
15264757LL,
15441515LL,
15620319LL,
15801194LL,
15984163LL,
16169251LL,
16356482LL,
16545881LL,
16737473LL,
16931284LL,
17127339LL,
17325664LL,
17526286LL,
17729231LL,
17934526LL,
18142198LL,
18352275LL,
18564784LL,
18779754LL,
18997213LL,
19217191LL,
19439715LL,
19664817LL,
19892524LL,
20122869LL,
20355881LL,
20591591LL,
20830030LL,
21071231LL,
21315224LL,
21562043LL,
21811719LL,
22064287LL,
22319780LL,
22578230LL,
22839674LL,
23104145LL,
23371678LL,
23642310LL,
23916075LL,
24193010LL,
24473152LL,
24756537LL,
25043205LL,
25333191LL,
25626536LL,
25923277LL,
26223454LL,
26527108LL,
26834277LL,
27145003LL,
27459328LL,
27777292LL,
28098938LL,
28424308LL,
28753446LL,
29086395LL,
29423200LL,
29763904LL,
30108554LL,
30457195LL,
30809872LL,
31166634LL,
31527527LL,
31892598LL,
32261897LL,
32635472LL,
33013373LL,
33395650LL,
33782353LL,
34173535LL,
34569245LL,
34969538LL,
35374467LL,
35784084LL,
36198444LL,
36617602LL,
37041614LL,
37470536LL,
37904424LL,
38343336LL,
38787331LL,
39236467LL,
39690804LL,
40150402LL,
40615322LL,
41085625LL,
41561374LL,
42042632LL,
42529463LL,
43021931LL,
43520102LL,
44024041LL,
44533815LL,
45049492LL,
45571141LL,
46098830LL,
46632629LL,
47172609LL,
47718842LL,
48271401LL,
48830357LL,
49395786LL,
49967762LL,
50546362LL,
51131661LL,
51723738LL,
52322670LL,
52928538LL,
53541422LL,
54161402LL,
54788562LL,
55422983LL,
56064751LL,
56713951LL,
57370667LL,
58034988LL,
58707002LL,
59386797LL,
60074463LL,
60770093LL,
61473777LL,
62185610LL,
62905686LL,
63634099LL,
64370947LL,
65116328LL,
65870339LL,
66633082LL,
67404657LL,
68185166LL,
68974713LL,
69773402LL,
70581340LL,
71398634LL,
72225391LL,
73061721LL,
73907736LL,
74763547LL,
75629269LL,
76505014LL,
77390901LL,
78287045LL,
79193566LL,
80110584LL,
81038221LL,
81976600LL,
82925844LL,
83886080LL,
84857435LL,
85840038LL,
86834019LL,
87839509LL,
88856643LL,
89885554LL,
90926380LL,
91979258LL,
93044327LL,
94121730LL,
95211608LL,
96314107LL,
97429371LL,
98557550LL,
99698793LL,
100853251LL,
102021076LL,
103202425LL,
104397452LL,
105606318LL,
106829181LL,
108066205LL,
109317553LL,
110583390LL,
111863886LL,
113159208LL,
114469530LL,
115795025LL,
117135868LL,
118492237LL,
119864313LL,
121252276LL,
122656311LL,
124076605LL,
125513344LL,
126966720LL,
128436925LL,
129924155LL,
131428606LL,
132950477LL,
134489971LL,
136047292LL,
137622645LL,
139216240LL,
140828288LL,
142459003LL,
144108600LL,
145777299LL,
147465321LL,
149172889LL,
150900229LL,
152647572LL,
154415147LL,
156203191LL,
158011938LL,
159841630LL
};

std::int64_t multiply_truncated(std::int64_t left, std::int64_t right) {
    return left * right / kUnity;
}

std::int64_t multiply_rounded(std::int64_t left, std::int64_t right) {
    const std::int64_t product = left * right;
    return (product + (product < 0 ? 0x7FFFFF : 0)) >> 23;
}

} // namespace

std::int64_t gain_to_scaler_q23(const AuroCxIntegralGainInfo& gain) {
    if (gain.selector != 0u)
        return 0;

    std::int32_t value = gain.value;
    std::int64_t factor = kUnity;
    std::int32_t table_value = value;

    if (value > -257) {
        if (value >= 257) {
            const std::uint32_t offset = static_cast<std::uint32_t>(value - 257);
            const std::uint32_t remainder_chunks =
                ((static_cast<std::uint16_t>(value - 257) >> 8u) + 1u) & 3u;
            if (offset >= 0x300u) {
                std::uint32_t chunks =
                    (((offset >> 8u) + 1u) & 0xFFFFFFFCu);
                do {
                    value -= 1024;
                    factor = multiply_truncated(kGainScalers[512], factor);
                    factor = multiply_truncated(kGainScalers[512], factor);
                    factor = multiply_truncated(kGainScalers[512], factor);
                    factor = multiply_truncated(kGainScalers[512], factor);
                    chunks -= 4u;
                } while (chunks);
            }
            table_value = value;
            if (!remainder_chunks)
                return multiply_truncated(kGainScalers[table_value + 256], factor);
            const std::uint32_t target = remainder_chunks << 8u;
            std::uint32_t consumed = 0;
            do {
                factor = multiply_rounded(kGainScalers[512], factor);
                consumed += 256u;
            } while (consumed != target);
            value -= static_cast<std::int32_t>(consumed);
        }
        table_value = value;
    } else {
        do {
            table_value = value + 256;
            factor = multiply_truncated(kGainScalers[0], factor);
            const bool repeat =
                static_cast<std::uint32_t>(value) < 0xFFFFFE00u;
            value += 256;
            if (!repeat)
                break;
        } while (true);
    }
    return multiply_truncated(kGainScalers[table_value + 256], factor);
}

AuroCxIntegralGainInfo get_bed_ref_gain(
    const CxSchemaParseResult& schema,
    std::size_t bed_index,
    const ReferenceGainSelection& selection) {
    if (!schema.gains_enabled)
        return {};
    if (selection.use_switch_group) {
        if (selection.switch_group_index >= schema.switch_groups.size())
            return {};
        const auto& group = schema.switch_groups[selection.switch_group_index];
        if (selection.switch_element_index >= group.elements.size())
            return {};
        return find_reference_gain(
            group.elements[selection.switch_element_index].bed_references,
            bed_index);
    }
    if (selection.program_index >= schema.programs.size())
        return {};
    return find_reference_gain(
        schema.programs[selection.program_index].bed_references,
        bed_index);
}

AuroCxIntegralGainInfo get_object_group_ref_gain(
    const CxSchemaParseResult& schema,
    std::size_t object_group_index,
    const ReferenceGainSelection& selection) {
    if (!schema.gains_enabled)
        return {};
    if (selection.use_switch_group) {
        if (selection.switch_group_index >= schema.switch_groups.size())
            return {};
        const auto& group = schema.switch_groups[selection.switch_group_index];
        if (selection.switch_element_index >= group.elements.size())
            return {};
        return find_reference_gain(
            group.elements[selection.switch_element_index].object_group_references,
            object_group_index);
    }
    if (selection.program_index >= schema.programs.size())
        return {};
    return find_reference_gain(
        schema.programs[selection.program_index].object_group_references,
        object_group_index);
}

AuroCxIntegralGainInfo relative_object_ref_gain(
    const AuroCxIntegralGainInfo& object_group_gain,
    const AuroCxIntegralGainInfo& bed_gain) {
    AuroCxIntegralGainInfo relative{};
    relative.selector = object_group_gain.selector ? 1u : 0u;
    relative.value = object_group_gain.value - bed_gain.value;
    return relative;
}

bool find_reference_location(
    const CxSchemaParseResult& schema,
    std::size_t bed_index,
    const std::vector<std::uint32_t>& linked_object_groups,
    ReferenceGainSelection& selection) {
    selection = {};
    if (find_reference_location_in_program(
            schema, schema.primary_program_index, bed_index,
            linked_object_groups, selection))
        return true;
    for (std::size_t program_index = 0;
         program_index < schema.programs.size(); ++program_index) {
        if (find_reference_location_in_program(
                schema, program_index, bed_index,
                linked_object_groups, selection))
            return true;
    }
    return false;
}

SchemaPosition position_to_schema(
    const AuroCxSchemaPositionInfo& position,
    bool position_16bit) {
    const float denominator = position_16bit ? 32767.0f : 127.0f;
    return {
        static_cast<float>(position.x) / denominator,
        static_cast<float>(position.y) / denominator,
        static_cast<float>(position.z) / denominator
    };
}

SchemaSpread spread_to_schema(const AuroCxSchemaSpreadInfo& spread) {
    constexpr float denominator = 255.0f;
    return {
        static_cast<float>(spread.x) / denominator,
        static_cast<float>(spread.y) / denominator,
        static_cast<float>(spread.z) / denominator
    };
}

bool resolve_gain_subblocks(
    const std::vector<AuroCxSchemaGainSubblockInfo>& encoded,
    std::size_t subblock_count,
    bool use_default,
    std::vector<AuroCxIntegralGainInfo>& resolved) {
    resolved.assign(subblock_count, {});
    if (use_default)
        return true;
    if (encoded.size() != subblock_count ||
        (subblock_count && !encoded.front().changed))
        return false;
    if (!subblock_count)
        return true;

    AuroCxIntegralGainInfo current = encoded.front().absolute;
    resolved.front() = current;
    for (std::size_t index = 1; index < subblock_count; ++index) {
        const auto& item = encoded[index];
        if (item.changed) {
            if (item.differential) {
                if (current.selector == 0u)
                    current.value += item.delta;
            } else {
                current = item.absolute;
            }
        }
        resolved[index] = current;
    }
    return true;
}

bool resolve_position_subblocks(
    const std::vector<AuroCxSchemaPositionSubblockInfo>& encoded,
    std::size_t subblock_count,
    bool use_default,
    const AuroCxSchemaPositionInfo& default_position,
    std::vector<AuroCxSchemaPositionInfo>& resolved) {
    resolved.assign(subblock_count, default_position);
    if (use_default)
        return true;
    if (encoded.size() != subblock_count ||
        (subblock_count && !encoded.front().changed))
        return false;
    if (!subblock_count)
        return true;

    AuroCxSchemaPositionInfo current = encoded.front().absolute;
    resolved.front() = current;
    for (std::size_t index = 1; index < subblock_count; ++index) {
        const auto& item = encoded[index];
        if (item.changed) {
            if (item.differential) {
                current.x += item.delta.x;
                current.y += item.delta.y;
                current.z += item.delta.z;
            } else {
                current = item.absolute;
            }
        }
        resolved[index] = current;
    }
    return true;
}

bool resolve_spread_subblocks(
    const std::vector<AuroCxSchemaSpreadSubblockInfo>& encoded,
    std::size_t subblock_count,
    bool use_default,
    const AuroCxSchemaSpreadInfo& default_spread,
    std::vector<AuroCxSchemaSpreadInfo>& resolved) {
    resolved.assign(subblock_count, default_spread);
    if (use_default)
        return true;
    if (encoded.size() != subblock_count ||
        (subblock_count && !encoded.front().changed))
        return false;
    if (!subblock_count)
        return true;

    AuroCxSchemaSpreadInfo current = encoded.front().absolute;
    resolved.front() = current;
    for (std::size_t index = 1; index < subblock_count; ++index) {
        if (encoded[index].changed)
            current = encoded[index].absolute;
        resolved[index] = current;
    }
    return true;
}

bool resolve_object_metadata(
    const CxSchemaParseResult& schema,
    std::size_t group_index,
    std::size_t object_index,
    ResolvedObjectMetadata& resolved) {
    if (group_index >= schema.object_groups.size() ||
        object_index >= schema.object_groups[group_index].objects.size() ||
        !schema.metadata_subblocks)
        return false;

    const auto& group = schema.object_groups[group_index];
    const auto& object = group.objects[object_index];
    const std::size_t count = schema.metadata_subblocks;

    std::vector<AuroCxIntegralGainInfo> gains;
    if (!schema.gains_enabled) {
        gains.assign(count, {});
    } else if (group.gains_present) {
        if (!resolve_gain_subblocks(
                group.gains, count, group.gains_use_default, gains))
            return false;
    } else if (!resolve_gain_subblocks(
                   object.gains, count, object.gains_use_default, gains)) {
        return false;
    }

    std::vector<AuroCxSchemaPositionInfo> positions;
    if (group.positions_present) {
        if (!resolve_position_subblocks(
                group.positions, count, group.positions_use_default,
                schema.default_object_position, positions))
            return false;
    } else if (!resolve_position_subblocks(
                   object.positions, count, object.positions_use_default,
                   schema.default_object_position, positions)) {
        return false;
    }

    const AuroCxSchemaSpreadInfo default_spread{};
    std::vector<AuroCxSchemaSpreadInfo> spreads;
    if (group.spreads_present) {
        if (!resolve_spread_subblocks(
                group.spreads, count, group.spreads_use_default,
                default_spread, spreads))
            return false;
    } else if (!resolve_spread_subblocks(
                   object.spreads, count, object.spreads_use_default,
                   default_spread, spreads)) {
        return false;
    }

    resolved.gain_q23.resize(count);
    resolved.positions.resize(count);
    resolved.spreads.resize(count);
    for (std::size_t index = 0; index < count; ++index) {
        resolved.gain_q23[index] = gain_to_scaler_q23(gains[index]);
        resolved.positions[index] =
            position_to_schema(positions[index], schema.object_position_16bit);
        resolved.spreads[index] = spread_to_schema(spreads[index]);
    }
    return true;
}

} // namespace cx
} // namespace auro3d
