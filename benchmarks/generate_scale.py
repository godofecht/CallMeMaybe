#!/usr/bin/env python3

import argparse
from pathlib import Path


def render(count: int) -> str:
    lines = [
        '#include <chrono>',
        '#include <cstdint>',
        '#include <iostream>',
        '#include "cmm/meta.hpp"',
        '',
    ]

    for index in range(count):
        lines.append(f'int scale_fn_{index}(int value) {{ return value + {index}; }}')

    lines += [
        '',
        'int main() {',
        '    using clock = std::chrono::steady_clock;',
        '    const auto registration_start = clock::now();',
    ]

    for index in range(count):
        lines.append(
            f'    if (cmm::register_rrefl<^^scale_fn_{index}>() != cmm::Error::Success) return 1;'
        )

    lines += [
        '    const auto registration_end = clock::now();',
        '    std::uint64_t checksum = 0;',
        '    const auto lookup_start = clock::now();',
    ]

    for index in range(count):
        lines.append(
            f'    checksum += cmm::reflect_name("scale_fn_{index}") == cmm::invalid_info ? 0 : 1;'
        )

    lines += [
        '    const auto lookup_end = clock::now();',
        '    const double registration_ns = std::chrono::duration<double, std::nano>(registration_end - registration_start).count();',
        '    const double lookup_ns = std::chrono::duration<double, std::nano>(lookup_end - lookup_start).count();',
        f'    if (checksum != {count}ULL) return 2;',
        '    std::cout << "{\\\"entity_count\\\":" << checksum',
        '              << ",\\\"registration_total_ns\\\":" << registration_ns',
        '              << ",\\\"registration_ns_per_entity\\\":" << (registration_ns / static_cast<double>(checksum))',
        '              << ",\\\"lookup_total_ns\\\":" << lookup_ns',
        '              << ",\\\"lookup_ns_per_entity\\\":" << (lookup_ns / static_cast<double>(checksum))',
        '              << "}\\n";',
        '    return 0;',
        '}',
        '',
    ]

    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description="Generate a CallMeMaybe registry scaling corpus")
    parser.add_argument("--count", type=int, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if args.count <= 0:
        raise SystemExit("--count must be positive")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render(args.count), encoding="utf-8")


if __name__ == "__main__":
    main()
