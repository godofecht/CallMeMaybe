#!/usr/bin/env python3

import argparse
from pathlib import Path


def render(case_count: int, seed: int) -> str:
    del seed
    lines = [
        '#include <cstddef>',
        '#include <iostream>',
        '#include <string_view>',
        '#include "cmm/meta.hpp"',
        '',
    ]

    for index in range(case_count):
        hidden_value = index + 700
        nested_value = index + 800
        template_value = index + 900
        lines += [
            'namespace {',
            f'struct Hidden_{index} {{',
            f'    [[=cmm::reflectable]] int value = {hidden_value};',
            '    [[=cmm::reflectable]] int read() const { return value; }',
            '};',
            '}',
            '',
            f'struct Outer_{index} {{',
            '    struct Nested {',
            f'        [[=cmm::reflectable]] int value = {nested_value};',
            '        [[=cmm::reflectable]] int read() const { return value; }',
            '    };',
            '};',
            '',
            'template <int N>',
            f'struct Template_{index} {{',
            '    [[=cmm::reflectable]] int value = N;',
            '    [[=cmm::reflectable]] int read() const { return value; }',
            '};',
            '',
            f'using TemplateInstance_{index} = Template_{index}<{template_value}>;',
            '',
        ]

    lines += [
        'template <typename T>',
        'bool validate_shape(cmm::info type_id, int expected) {',
        '    const cmm::info value_id = cmm::lookup::get_member(type_id, "value");',
        '    const cmm::info read_id = cmm::lookup::get_member(type_id, "read");',
        '    if (value_id == cmm::invalid_info || read_id == cmm::invalid_info) return false;',
        '    if (cmm::parent_of(value_id) != type_id || cmm::parent_of(read_id) != type_id) return false;',
        '    if (cmm::offset_of(value_id) != offsetof(T, value)) return false;',
        '    T object;',
        '    if (cmm::invoke<int>(read_id, &object) != expected) return false;',
        '    return true;',
        '}',
        '',
        'int main() {',
        '    bool ok = true;',
    ]

    for index in range(case_count):
        template_value = index + 900
        lines += [
            f'    ok = ok && cmm::register_rrefl<^^Hidden_{index}>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^Outer_{index}::Nested>() == cmm::Error::Success;',
            f'    ok = ok && cmm::register_rrefl<^^Template_{index}<{template_value}>>() == cmm::Error::Success;',
        ]

    lines.append('')

    for index in range(case_count):
        hidden_value = index + 700
        nested_value = index + 800
        template_value = index + 900
        lines += [
            '    {',
            f'        const cmm::info hidden_id = cmm::get_id<^^Hidden_{index}>();',
            f'        const cmm::info nested_id = cmm::get_id<^^Outer_{index}::Nested>();',
            f'        const cmm::info template_id = cmm::get_id<^^Template_{index}<{template_value}>>();',
            f'        ok = ok && validate_shape<Hidden_{index}>(hidden_id, {hidden_value});',
            f'        ok = ok && validate_shape<Outer_{index}::Nested>(nested_id, {nested_value});',
            f'        ok = ok && validate_shape<TemplateInstance_{index}>(template_id, {template_value});',
            '        if (hidden_id == nested_id || hidden_id == template_id || nested_id == template_id) ok = false;',
            f'        std::cout << "SHAPES\\t{index}\\t" << cmm::size_of(hidden_id) << "\\t" << cmm::size_of(nested_id) << "\\t" << cmm::size_of(template_id) << "\\n";',
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
    parser = argparse.ArgumentParser(description="Generate anonymous/nested/template reflection cases")
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
