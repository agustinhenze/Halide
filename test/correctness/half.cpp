#include <Halide.h>
#include <stdio.h>

// #define HAVE_HALF_HPP
#ifdef HAVE_HALF_HPP
  #define HALF_ROUND_TIES_TO_EVEN 1
  #include "half.hpp"
#endif

using namespace Halide;

bool float_eq(float a, float b) {
    return (isnan(a) && isnan(b)) || (a == b);
}

bool test_float2half() {
    Func f, g;
    Var x;

    struct {
        float f;
        uint16_t h;
    } tab[] = {
        { 1.0f, 0x3c00 },
        { -1.0f, 0xbc00 },
        { 0.5f, 0x3800 },
        { 1.f / (1 << 14), 0x0400 },    // smallest normal half
        { 1.f / (1 << 14) - 1.f/(1 << 24), 0x03ff },
        { 1.f / (1 << 14) - 1.f/(1 << 25), 0x0400 },
        { 1.f / (1 << 15), 0x0200 },    // first subnormal
        { 1.f / (1 << 15) + 1.f/(1 << 24), 0x0201 },
        { 1.f / (1 << 15) + 1.f/(1 << 25), 0x0200 },
        { 1.f / (1 << 15) + 1.f/(1 << 24) + 1.f/(1 << 25), 0x0202 },
        { -6.103515625e-05, 0x8400 },
        { 65504, 0x7bff },             // largest normal half

        // The last number that should round down
        { 65520*(1 - std::numeric_limits<float>::epsilon()), 0x7bff },

        // This is halfway between the largest normal half and its successor.
        // Round to even should produce infinity, not round down to 0x7bff.
        { 65520, 0x7c00 },
        { std::numeric_limits<float>::infinity(), 0x7c00 },
        { std::numeric_limits<float>::quiet_NaN(), 0x7e00 },
    };
    const int N = sizeof(tab) / sizeof(tab[0]);

    Image<float> fim(N);
    for (int i = 0; i < N; i++) {
        fim(i) = tab[i].f;
    }

    f(x) = reinterpret<uint16_t>(cast<Halide::half>(fim(x)));
    Image<uint16_t> im = f.realize(N);

    for (int i = 0; i < N; i++) {
#ifdef HAVE_HALF_HPP
        uint16_t expected = half_float::detail::float2half<std::round_to_nearest>(tab[i].f);
        if (tab[i].h != expected) {
            printf("Table incorrect for %g: 0x%04x should be 0x%04x\n",
                   tab[i].f, tab[i].h, expected);
        }
#endif
        if (im(i) != tab[i].h) {
            printf("Incorrect float->half conversion for %g: 0x%04x should be 0x%04x\n",
                   tab[i].f, im(i), tab[i].h);
            return false;
        }
    }
    return true;
}

bool test_half2float() {
    Func g;
    Var x;
    Image<uint16_t> him(65536);
    for (int i = 0; i < 65536; i++) {
        him(i) = (uint16_t)i;
    }

    g(x) = cast<float>(reinterpret<Halide::half>(him(x)));
    Image<float> out = g.realize(65536);

    for (int i = 0; i < 65536; i++) {
#ifdef HAVE_HALF_HPP
        float expected = half_float::detail::half2float((uint16_t)i);
        if (!float_eq(out(i), expected)) {
            printf("Incorrect half->float conversion: 0x%04x maps to %g, should be %g\n",
                   i, out(i), expected);
            return false;
        }
#endif
    }
    return true;
}

bool test_usage() {
    Func f, g;
    Var x, y, c;
    f(x, y, c) = cast<Halide::half>(x / 256.f);
    f.compute_root();
    g(x, y, c) = cast<float>(f(x, 0, 0));
    g.bound(c, 0, 1).glsl(x, y, c);
    Image<float> result = g.realize(256, 256, 1);
    result.copy_to_host();
    for (int i= 0; i < 256; i++) {
        if (result(i) * 256 != i) {
            printf("%d  %g\n", i, result(i) * 256);
            return false;
        }
    }
    return true;
}

int main(int argc, char **argv) {
    int success = test_float2half();
    success &= test_half2float();
    success &= test_usage();
    return success ? 0 : -1;
}
