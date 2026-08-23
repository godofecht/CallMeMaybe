#include "generator.hpp"

#include <iostream>

int generated_add(int a, int b)
{
    return a + b;
}

double generated_scale(double value, double gain)
{
    return value * gain;
}

bool generated_not(bool value)
{
    return !value;
}

int main()
{
    const cmm::flow::GenerationResult generated =
        cmm::flow::generate_wrapper_fragment<^^generated_add, ^^generated_scale, ^^generated_not>();
    if (generated.error != cmm::Error::Success)
    {
        std::cerr << cmm::to_string(generated.error) << '\n';
        return 1;
    }

    std::cout << generated.source;
    return 0;
}
