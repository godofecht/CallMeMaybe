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

    layouts = []
    for index in range(case_count):
        pad_type = rng.choice(["char", "short", "long", "double"])
        layouts.append(pad_type)
        lines += [
            f'namespace left_{index} {{',
            f'struct Type {{',
            f'    [[=cmm::reflectable]] int same;',
            f'    [[=cmm::reflectable]] {pad_type} pad;',
            '};',
            '}',
            f'namespace right_{index} {{',
            f'struct Type {{',
            f'    [[=cmm::reflectable]] {pad_type} pad;',
            f'    [[=cmm::reflectable]] int same;',
            '};',
            '}',
            '',
            f'int function_{index}(int value, long tag) {{ return value + static_cast<int>(tag); }}',
            f'enum class Enum_{index} : unsigned {{ Zero = 0u, Small = {index + 1}u, High = 0x80000000u }};',
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
        ]

    lines.append('')

    for index in range(case_count):
        lines += [
            f'    ok = ok && dump_type<left_{index}::Type>("left_{index}::Type", cmm::get_id<^^left_{index}::Type>(), offsetof(left_{index}::Type, same));',
            f'    ok = ok && dump_type<right_{index}::Type>("right_{index}::Type", cmm::get_id<^^right_{index}::Type>(), offsetof(right_{index}::Type, same));',
            f'    {{',
            f'        const cmm::info function_id = cmm::get_id<^^function_{index}>();',
            f'        const auto params = cmm::parameters_of(function_id);',
            f'        if (params.size() != 2 || params[0] == params[1]) ok = false;',
            f'        for (std::size_t p = 0; p < params.size(); ++p) {{',
            f'            if (cmm::parent_of(params[p]) != function_id) ok = false;',
            f'            std::cout << "PARAM\\tfunction_{index}\\t" << p << "\\t" << cmm::identifier_of(params[p]) << "\\n";',
            f'        }}',
            f'    }}',
            f'    {{',
            f'        const cmm::info enum_id = cmm::get_id<^^Enum_{index}>();',
            f'        const auto values = cmm::enumerators_of(enum_id);',
            f'        if (values.size() != 3) ok = false;',
            f'        for (cmm::info value : values) {{',
            f'            if (cmm::parent_of(value) != enum_id) ok = false;',
            f'            std::cout << "ENUM\\tEnum_{index}\\t" << cmm::identifier_of(value) << "\\t" << cmm::value_of(value) << "\\n";',
            f'        }}',
            f'    }}',
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
