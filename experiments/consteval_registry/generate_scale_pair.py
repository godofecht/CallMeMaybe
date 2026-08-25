#!/usr/bin/env python3

import argparse
from pathlib import Path


def declarations(count: int) -> list[str]:
    return [f"struct ScaleType_{index} {{ int value = {index}; }};" for index in range(count)]


def runtime_source(count: int) -> str:
    lines = [
        "#include <chrono>",
        "#include <cstdint>",
        "#include <iostream>",
        "#include \"cmm/meta.hpp\"",
        "",
        *declarations(count),
        "",
        "int main()",
        "{",
        "    using clock = std::chrono::steady_clock;",
        "    const auto startup_begin = clock::now();",
    ]
    for index in range(count):
        lines.append(
            f"    if (cmm::register_rrefl<^^ScaleType_{index}>() != cmm::Error::Success) return 1;"
        )
    lines += [
        "    const auto startup_end = clock::now();",
        "    std::uint64_t checksum = 0;",
        "    const auto lookup_begin = clock::now();",
    ]
    for index in range(count):
        lines.append(
            f"    checksum += cmm::reflect_name(\"ScaleType_{index}\") != cmm::invalid_info ? 1ULL : 0ULL;"
        )
    lines += [
        "    const auto lookup_end = clock::now();",
        f"    if (checksum != {count}ULL) return 2;",
        "    const double startup_ns = std::chrono::duration<double, std::nano>(startup_end - startup_begin).count();",
        "    const double lookup_ns = std::chrono::duration<double, std::nano>(lookup_end - lookup_begin).count();",
        f"    std::cout << \"{{\\\"backend\\\":\\\"runtime\\\",\\\"entity_count\\\":{count},\\\"startup_ns\\\":\" << startup_ns",
        "              << \",\\\"lookup_ns\\\":\" << lookup_ns",
        "              << \",\\\"checksum\\\":\" << checksum << \"}\\n\";",
        "    return 0;",
        "}",
        "",
    ]
    return "\n".join(lines)


def static_source(count: int) -> str:
    refls = ",\n        ".join(f"^^ScaleType_{index}" for index in range(count))
    lines = [
        "#include <chrono>",
        "#include <cstdint>",
        "#include <iostream>",
        "#include \"cmm/detail/static_active_registry.hpp\"",
        "#include \"cmm/detail/static_type_registry.hpp\"",
        "#include \"cmm/static_meta.hpp\"",
        "",
        *declarations(count),
        "",
        "CMM_USE_STATIC_REGISTRY(",
        "    (cmm::detail::make_type_static_registry<",
        f"        {refls}>()))",
        "",
        "int main()",
        "{",
        "    using clock = std::chrono::steady_clock;",
        "    const auto startup_begin = clock::now();",
        "    const auto startup_end = clock::now();",
        "    std::uint64_t checksum = 0;",
        "    const auto lookup_begin = clock::now();",
    ]
    for index in range(count):
        lines.append(
            f"    checksum += cmm::reflect_name(\"ScaleType_{index}\") != cmm::invalid_info ? 1ULL : 0ULL;"
        )
    lines += [
        "    const auto lookup_end = clock::now();",
        f"    if (checksum != {count}ULL) return 2;",
        "    const double startup_ns = std::chrono::duration<double, std::nano>(startup_end - startup_begin).count();",
        "    const double lookup_ns = std::chrono::duration<double, std::nano>(lookup_end - lookup_begin).count();",
        f"    std::cout << \"{{\\\"backend\\\":\\\"consteval\\\",\\\"entity_count\\\":{count},\\\"startup_ns\\\":\" << startup_ns",
        "              << \",\\\"lookup_ns\\\":\" << lookup_ns",
        "              << \",\\\"checksum\\\":\" << checksum << \"}\\n\";",
        "    return 0;",
        "}",
        "",
    ]
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate matched runtime/consteval registry scaling sources")
    parser.add_argument("--count", type=int, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    if args.count <= 0:
        raise SystemExit("--count must be positive")

    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "runtime.cpp").write_text(runtime_source(args.count), encoding="utf-8")
    (args.output_dir / "consteval.cpp").write_text(static_source(args.count), encoding="utf-8")


if __name__ == "__main__":
    main()
