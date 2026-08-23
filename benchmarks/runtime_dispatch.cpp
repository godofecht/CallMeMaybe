#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "cmm/meta.hpp"

#if defined(__GNUC__) || defined(__clang__)
#define CMM_NOINLINE __attribute__((noinline))
#else
#define CMM_NOINLINE
#endif

CMM_NOINLINE int bench_add(int a, int b)
{
    return a + b;
}

struct VirtualOp
{
    virtual ~VirtualOp() = default;
    virtual int apply(int a, int b) const = 0;
};

struct AddOp final : VirtualOp
{
    CMM_NOINLINE int apply(int a, int b) const override
    {
        return a + b;
    }

    CMM_NOINLINE int operator()(int a, int b) const
    {
        return a + b;
    }
};

struct Options
{
    std::size_t iterations = 1'000'000;
    std::size_t batch = 4'096;
    std::string json_path;
};

struct Result
{
    std::string name;
    std::size_t operations = 0;
    double total_ns = 0.0;
    double ns_per_op = 0.0;
    double p50_ns = 0.0;
    double p95_ns = 0.0;
    double p99_ns = 0.0;
    std::uint64_t checksum = 0;
};

static double percentile(std::vector<double> values, double p)
{
    if (values.empty())
    {
        return 0.0;
    }

    std::sort(values.begin(), values.end());
    const double position = p * static_cast<double>(values.size() - 1);
    const auto lo = static_cast<std::size_t>(position);
    const auto hi = std::min(lo + 1, values.size() - 1);
    const double fraction = position - static_cast<double>(lo);
    return values[lo] + (values[hi] - values[lo]) * fraction;
}

template <typename Fn>
static Result run_benchmark(std::string name, const Options& options, Fn&& fn)
{
    using clock = std::chrono::steady_clock;

    std::vector<double> batch_ns;
    batch_ns.reserve((options.iterations + options.batch - 1) / options.batch);

    std::uint64_t checksum = 0;
    std::size_t completed = 0;
    const auto total_start = clock::now();

    while (completed < options.iterations)
    {
        const std::size_t count = std::min(options.batch, options.iterations - completed);
        const auto start = clock::now();

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::size_t n = completed + i;
            const int a = static_cast<int>(n & 1023U);
            const int b = static_cast<int>((n * 17U + 3U) & 1023U);
            checksum += static_cast<std::uint64_t>(fn(a, b));
        }

        const auto end = clock::now();
        const double elapsed = std::chrono::duration<double, std::nano>(end - start).count();
        batch_ns.push_back(elapsed / static_cast<double>(count));
        completed += count;
    }

    const auto total_end = clock::now();
    const double total_ns = std::chrono::duration<double, std::nano>(total_end - total_start).count();

    return Result{
        .name = std::move(name),
        .operations = options.iterations,
        .total_ns = total_ns,
        .ns_per_op = total_ns / static_cast<double>(options.iterations),
        .p50_ns = percentile(batch_ns, 0.50),
        .p95_ns = percentile(batch_ns, 0.95),
        .p99_ns = percentile(batch_ns, 0.99),
        .checksum = checksum,
    };
}

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
        else if (arg == "--batch" && i + 1 < argc)
        {
            options.batch = static_cast<std::size_t>(std::strtoull(argv[++i], nullptr, 10));
        }
        else if (arg == "--json" && i + 1 < argc)
        {
            options.json_path = argv[++i];
        }
        else if (arg == "--help")
        {
            std::cout << "Usage: cmm_runtime_bench [--iterations N] [--batch N] [--json path]\n";
            std::exit(0);
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << '\n';
            std::exit(2);
        }
    }

    if (options.iterations == 0 || options.batch == 0)
    {
        std::cerr << "iterations and batch must both be non-zero\n";
        std::exit(2);
    }

    return options;
}

static std::string compiler_name()
{
#if defined(__clang__)
    return std::string("clang-") + __clang_version__;
#elif defined(__GNUC__)
    return std::string("gcc-") + __VERSION__;
#else
    return "unknown";
#endif
}

static void write_json(const std::string& path, const Options& options, const std::vector<Result>& results)
{
    std::ofstream out(path);
    if (!out)
    {
        std::cerr << "Unable to open JSON output: " << path << '\n';
        std::exit(2);
    }

    out << "{\n";
    out << "  \"compiler\": \"" << compiler_name() << "\",\n";
    out << "  \"iterations\": " << options.iterations << ",\n";
    out << "  \"batch\": " << options.batch << ",\n";
    out << "  \"results\": [\n";

    for (std::size_t i = 0; i < results.size(); ++i)
    {
        const Result& result = results[i];
        out << "    {\"name\": \"" << result.name << "\", "
            << "\"operations\": " << result.operations << ", "
            << "\"total_ns\": " << std::fixed << std::setprecision(3) << result.total_ns << ", "
            << "\"ns_per_op\": " << result.ns_per_op << ", "
            << "\"p50_ns\": " << result.p50_ns << ", "
            << "\"p95_ns\": " << result.p95_ns << ", "
            << "\"p99_ns\": " << result.p99_ns << ", "
            << "\"checksum\": " << result.checksum << "}";
        out << (i + 1 == results.size() ? "\n" : ",\n");
    }

    out << "  ]\n";
    out << "}\n";
}

int main(int argc, char** argv)
{
    const Options options = parse_options(argc, argv);

    const cmm::Error registration = cmm::register_rrefl<^^bench_add>();
    if (registration != cmm::Error::Success)
    {
        std::cerr << "Failed to register bench_add: " << cmm::to_string(registration) << '\n';
        return 1;
    }

    const cmm::info add_id = cmm::reflect_name("bench_add");
    if (add_id == cmm::invalid_info)
    {
        std::cerr << "Failed to resolve bench_add\n";
        return 1;
    }

    using FunctionPointer = int (*)(int, int);
    FunctionPointer function_pointer = &bench_add;
    AddOp add_op;
    const VirtualOp* virtual_op = &add_op;
    std::variant<AddOp> variant = AddOp{};
    const std::array<FunctionPointer, 1> dispatch_table{&bench_add};

    std::vector<Result> results;
    results.reserve(7);

    results.push_back(run_benchmark("direct", options, [](int a, int b) {
        return bench_add(a, b);
    }));

    results.push_back(run_benchmark("function_pointer", options, [function_pointer](int a, int b) {
        return function_pointer(a, b);
    }));

    results.push_back(run_benchmark("virtual", options, [virtual_op](int a, int b) {
        return virtual_op->apply(a, b);
    }));

    results.push_back(run_benchmark("variant", options, [&variant](int a, int b) {
        return std::visit([&](const auto& op) { return op(a, b); }, variant);
    }));

    results.push_back(run_benchmark("table", options, [&dispatch_table](int a, int b) {
        return dispatch_table[0](a, b);
    }));

    results.push_back(run_benchmark("cmm_typed", options, [add_id](int a, int b) {
        return cmm::invoke<int>(add_id, a, b);
    }));

    results.push_back(run_benchmark("cmm_raw", options, [add_id](int a, int b) {
        std::array<cmm::Value, 2> args{cmm::Value(a), cmm::Value(b)};
        cmm::Value result;
        const cmm::Error error = cmm::reflect_invoke(add_id, args, result);
        if (error != cmm::Error::Success)
        {
            return 0;
        }
        return result.get<int>();
    }));

    const std::uint64_t expected_checksum = results.front().checksum;
    for (const Result& result : results)
    {
        if (result.checksum != expected_checksum)
        {
            std::cerr << "Correctness mismatch in " << result.name << '\n';
            return 1;
        }
    }

    std::cout << "compiler: " << compiler_name() << '\n';
    std::cout << std::left << std::setw(20) << "dispatch"
              << std::right << std::setw(14) << "ns/op"
              << std::setw(14) << "p50"
              << std::setw(14) << "p95"
              << std::setw(14) << "p99" << '\n';

    for (const Result& result : results)
    {
        std::cout << std::left << std::setw(20) << result.name
                  << std::right << std::fixed << std::setprecision(3)
                  << std::setw(14) << result.ns_per_op
                  << std::setw(14) << result.p50_ns
                  << std::setw(14) << result.p95_ns
                  << std::setw(14) << result.p99_ns << '\n';
    }

    if (!options.json_path.empty())
    {
        write_json(options.json_path, options, results);
    }

    return 0;
}
