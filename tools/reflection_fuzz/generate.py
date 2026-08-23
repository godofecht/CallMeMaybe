#!/usr/bin/env python3

import argparse
import random
from pathlib import Path


def render(case_count: int, seed: int) -> str:
    rng = random.Random(seed)
    lines = [
        '#include <cstddef>',
        '#include <cstdint>',
        '#include <iostream>',
        '#include <string_view>',
        '#include "cmm/meta.hpp"',
        '',
    ]

    for index in range(case_count):
        pad_type = rng.choice(["char", "short", "long", "double"])
        lines += [
            f'namespace left_{index} {{',
            'struct Type {',
            '    [[=cmm::reflectable]] int same;',
            f'    [[=cmm::reflectable]] {pad_type} pad;',
            '};',
            '}',
            f'namespace right_{index} {{',
            'struct Type {',
            f'    [[=cmm::reflectable]] {pad_type} pad;',
            '    [[=cmm::reflectable]] int same;',
            '};',
            '}',
            '',
            f'int function_{index}(int value, long tag) {{ return value + static_cast<int>(tag); }}',
            f'enum class Enum_{index} : unsigned {{ Zero = 0u, Small = {index + 1}u, High = 0x80000000u }};',
            '',
            f'struct Root_{index} {{',
            '    [[=cmm::reflectable]] int root_value = 0;',
            '    [[=cmm::reflectable]] int read_root() const { return root_value; }',
            '};',
            f'struct LeftBase_{index} : virtual Root_{index} {{}};',
            f'struct RightBase_{index} : virtual Root_{index} {{}};',
            f'struct Diamond_{index} : LeftBase_{index}, RightBase_{index} {{}};',
            '',
        ]

    lines += [
        'template <typename T>',
        'bool dump_type(std::string_view label, cmm::info type_id, std::size_t expected_same_offset) {',
        '    const auto members = cmm::nonstatic_data_members_of(type_id);',
        '    if (members.size() != 2) return false;',
        '    bool found_same = false;',
        '    std::cout << "TYPE\\t" << label << "\\t" << cmm::size_of(type_id) << "\\t" << cmm::alignment_of(type_id) << "\\n";',
        '    for (cmm::info member : members) {',
        '        const auto name = cmm::identifier_of(member);',
        '        const auto parent = cmm::parent_of(member);',
        '        std::cout << "MEMBER\\t" << label << "\\t" << name << "\\t" << cmm::offset_of(member) << "\\n";',
        '        if (parent != type_id) return false;',
        '        if (name == "same") {',
        '            found_same = true;',
        '            if (cmm::offset_of(member) != expected_same_offset) return false;',
        '        }',
        '    }',
        '    return found_same;',
        '}',
        '',
        'int main() {',
        '    bool ok = true;',
    ]

    for index in range(case_count):
        lines += [
            f'    ok = ok && cmm::register_rrefl<^^left_{index}::Type>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^right_{index}::Type>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^function_{index}>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^Enum_{index}>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^Diamond_{index}>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^Root_{index}>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^LeftBase_{index}>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^RightBase_{index}>() == cmm::Error::Success;',
        ]

    lines.append('')

    for index in range(case_count):
        lines += [
            f'    ok = ok && dump_type<left_{index}::Type>("left_{index}::Type", cmm::get_id<^^left_{index}::Type>(), offsetof(left_{index}::Type, same));',
            f'    ok = ok && dump_type<right_{index}::Type>("right_{index}::Type", cmm::get_id<^^right_{index}::Type>(), offsetof(right_{index}::Type, same));',
            '    {',
            f'        const cmm::info function_id = cmm::get_id<^^function_{index}>();',
            '        const auto params = cmm::parameters_of(function_id);',
            '        if (params.size() != 2 || params[0] == params[1]) ok = false;',
            '        for (std::size_t p = 0; p < params.size(); ++p) {',
            '            if (cmm::parent_of(params[p]) != function_id) ok = false;',
            f'            std::cout << "PARAM\\tfunction_{index}\\t" << p << "\\t" << cmm::identifier_of(params[p]) << "\\n";',
            '        }',
            '    }',
            '    {',
            f'        const cmm::info enum_id = cmm::get_id<^^Enum_{index}>();',
            '        const auto values = cmm::enumerators_of(enum_id);',
            '        if (values.size() != 3) ok = false;',
            '        for (cmm::info value : values) {',
            '            if (cmm::parent_of(value) != enum_id) ok = false;',
            f'            std::cout << "ENUM\\tEnum_{index}\\t" << cmm::identifier_of(value) << "\\t" << cmm::value_of(value) << "\\n";',
            '        }',
            '    }',
            '    {',
            f'        const cmm::info root_id = cmm::get_id<^^Root_{index}>();',
            f'        const cmm::info left_id = cmm::get_id<^^LeftBase_{index}>();',
            f'        const cmm::info right_id = cmm::get_id<^^RightBase_{index}>();',
            f'        const cmm::info diamond_id = cmm::get_id<^^Diamond_{index}>();',
            '        const auto left_bases = cmm::bases_of(left_id);',
            '        const auto right_bases = cmm::bases_of(right_id);',
            '        const auto diamond_bases = cmm::bases_of(diamond_id);',
            '        if (left_bases.size() != 1 || right_bases.size() != 1 || diamond_bases.size() != 2) ok = false;',
            '        if (left_bases.size() == 1) {',
            '            if (!cmm::is_virtual_base(left_bases[0])) ok = false;',
            '            if (cmm::parent_of(left_bases[0]) != left_id) ok = false;',
            '        }',
            '        if (right_bases.size() == 1) {',
            '            if (!cmm::is_virtual_base(right_bases[0])) ok = false;',
            '            if (cmm::parent_of(right_bases[0]) != right_id) ok = false;',
            '        }',
            '        for (cmm::info base : diamond_bases) {',
            '            if (cmm::is_virtual_base(base)) ok = false;',
            '            if (cmm::parent_of(base) != diamond_id) ok = false;',
            '        }',
            f'        Diamond_{index} diamond;',
            f'        diamond.root_value = {index + 100};',
            '        const cmm::info read_id = cmm::lookup::get_member(root_id, "read_root");',
            '        if (read_id == cmm::invalid_info) ok = false;',
            f'        else if (cmm::invoke<int>(read_id, &diamond) != {index + 100}) ok = false;',
            f'        std::cout << "INHERIT\\tDiamond_{index}\\t" << left_bases.size() << "\\t" << right_bases.size() << "\\t" << diamond_bases.size() << "\\n";',
            '    }',
        ]

    lines += [
        '    std::cout << "STATUS\\t" << (ok ? "ok" : "failed") << "\\n";',
        '    return ok ? 0 : 1;',
        '}',
        '',
    ]

    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate deterministic CallMeMaybe reflection stress cases")
    parser.add_argument("--cases", type=int, default=32)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if args.cases <= 0:
        raise SystemExit("--cases must be positive")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render(args.cases, args.seed), encoding="utf-8")


if __name__ == "__main__":
    main()
