#!/usr/bin/env python3

from campaign import campaign_identity, validate_resume_summary


def expect_rejected(summary: dict, identity: dict,
                    expected_records: set[tuple[str, int]]) -> None:
    try:
        validate_resume_summary(summary, identity, expected_records)
    except SystemExit:
        return
    raise AssertionError("incompatible resume manifest was accepted")


def main() -> None:
    identity = campaign_identity(
        4,
        1,
        0,
        2,
        ["core", "shapes"],
        1,
        ["gcc=g++-16 -std=c++26 -freflection", "clang=clang++ -std=c++26 -freflection-latest"],
    )
    expected_records = {
        (family, seed)
        for seed in (1, 3)
        for family in ("core", "shapes")
    }

    compatible = {
        **identity,
        "programs": [
            {"family": "core", "seed": 1},
            {"family": "shapes", "seed": 1},
        ],
    }
    validate_resume_summary(compatible, identity, expected_records)

    expect_rejected(
        {**compatible, "compilers": ["gcc=g++-16 -std=c++26 -freflection"]},
        identity,
        expected_records,
    )
    expect_rejected(
        {**compatible, "seed_start": 2},
        identity,
        expected_records,
    )
    expect_rejected(
        {
            **identity,
            "programs": [
                {"family": "core", "seed": 1},
                {"family": "core", "seed": 1},
            ],
        },
        identity,
        expected_records,
    )
    expect_rejected(
        {**identity, "programs": [{"family": "core", "seed": 2}]},
        identity,
        expected_records,
    )


if __name__ == "__main__":
    main()
