#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>

#include <metapp/allmetatypes.h>
#include <metapp/metarepo.h>

#if defined(__GNUC__) || defined(__clang__)
#define CMM_NOINLINE __attribute__((noinline))
#else
#define CMM_NOINLINE
#endif

CMM_NOINLINE int bench_add(int a, int b)
{
    return a + b;
}

struct Options
{
    std::size_t iterations = 1'000'000;
    std::string json_path;
};

static Options parse_options(int argc, char** argv)
{
    Options options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string_view arg(argv[i]);
        if (arg == "--iterations" && i + 1 < argc)
        {
            options.iterations = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
        }
        else if (arg == "--json" && i + 1 < argc)
        {
            options.json_path = argv[++i];
        }
        else
        {
            std::cerr << "Usage: cmm_metapp_bench [--iterations N] [--json path]\n";
            std::exit(2);
        }
    }
    if (options.iterations == 0) std::exit(2);
    return options;
}

struct Measurement
{
    double ns_per_op = 0.0;
    std::uint64_t checksum = 0;
};

template <typename Fn>
static Measurement measure(std::size_t iterations, Fn&& fn)
{
    using clock = std::chrono::steady_clock;
    std::uint64_t checksum = 0;
    const auto start = clock::now();
    for (std::size_t n = 0; n < iterations; ++n)
    {
        const int a = static_cast<int>(n & 1023U);
        const int b = static_cast<int>((n * 17U + 3U) & 1023U);
        checksum += static_cast<std::uint64_t>(fn(a, b));
    }
    const auto end = clock::now();
    const double total_ns = std::chrono::duration<double, std::nano>(end - start).count();
    return Measurement{total_ns / static_cast<double>(iterations), checksum};
}

int main(int argc, char** argv)
{
    const Options options = parse_options(argc, argv);

    metapp::MetaRepo repo;
    repo.registerCallable("bench_add", &bench_add);
    const metapp::MetaItem callable = repo.getCallable("bench_add");
    if (callable.isEmpty())
    {
        std::cerr << "metapp failed to resolve bench_add\n";
        return 1;
    }

    const Measurement direct = measure(options.iterations, [](int a, int b) {
        return bench_add(a, b);
    });

    const Measurement metapp_result = measure(options.iterations, [&callable](int a, int b) {
        const metapp::Variant result = metapp::callableInvoke(callable, nullptr, a, b);
        return result.get<int>();
    });

    if (direct.checksum != metapp_result.checksum)
    {
        std::cerr << "Correctness mismatch in metapp adapter\n";
        return 1;
    }

    std::cout << std::fixed << std::setprecision(3)
              << "direct ns/op: " << direct.ns_per_op << '\n'
              << "metapp ns/op: " << metapp_result.ns_per_op << '\n';

    if (!options.json_path.empty())
    {
        std::ofstream out(options.json_path);
        if (!out) return 2;
        out << "{\n"
            << "  \"iterations\": " << options.iterations << ",\n"
            << "  \"results\": [\n"
            << "    {\"name\": \"direct\", \"ns_per_op\": " << direct.ns_per_op
            << ", \"checksum\": " << direct.checksum << "},\n"
            << "    {\"name\": \"metapp\", \"ns_per_op\": " << metapp_result.ns_per_op
            << ", \"checksum\": " << metapp_result.checksum << "}\n"
            << "  ]\n"
            << "}\n";
    }

    return 0;
}
