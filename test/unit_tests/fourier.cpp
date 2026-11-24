#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

#include "fourier.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

namespace {
// Deterministic pseudo-random sequence for test vectors without triggering security linter
// warnings.
struct DeterministicRng {
    uint64_t state;
    explicit DeterministicRng(uint64_t seed)
        : state(seed) {}
    double next_double(double min_val, double max_val) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        double unit = static_cast<double>(state >> 11) / static_cast<double>(1ULL << 53);
        return min_val + unit * (max_val - min_val);
    }
};
}  // namespace

TEST_CASE("FFT radix2 forward and inverse round-trip across large size range", "[Neuron][fft]") {
    const std::vector<unsigned long> test_sizes = {
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384};

    for (unsigned long n: test_sizes) {
        DYNAMIC_SECTION("Size " << n) {
            std::vector<double> input(n);
            for (unsigned long i = 0; i < n; ++i) {
                input[i] = std::sin(2.0 * M_PI * i / static_cast<double>(n)) +
                           0.5 * std::cos(4.0 * M_PI * i / static_cast<double>(n)) +
                           static_cast<double>(i + 1);
            }

            auto buffer = input;

            // Forward transform: real -> half-complex
            nrngsl_realft(buffer.data(), n, 1);

            // Inverse transform: half-complex -> real
            nrngsl_realft(buffer.data(), n, -1);

            for (unsigned long i = 0; i < n; ++i) {
                REQUIRE_THAT(buffer[i], WithinAbs(input[i], 1e-10));
            }
        }
    }
}

TEST_CASE("FFT radix2 canonical signals", "[Neuron][fft]") {
    SECTION("Identity for n = 1") {
        double val = 42.0;
        nrngsl_realft(&val, 1, 1);
        REQUIRE(val == 42.0);
        nrngsl_realft(&val, 1, -1);
        REQUIRE(val == 42.0);
    }

    SECTION("Size n = 2") {
        std::vector<double> data = {3.0, 7.0};
        nrngsl_realft(data.data(), 2, 1);
        // For n=2: DC component X0 = 3+7 = 10, Nyquist X1 = 3-7 = -4
        REQUIRE_THAT(data[0], WithinAbs(10.0, 1e-14));
        REQUIRE_THAT(data[1], WithinAbs(-4.0, 1e-14));

        nrngsl_realft(data.data(), 2, -1);
        REQUIRE_THAT(data[0], WithinAbs(3.0, 1e-14));
        REQUIRE_THAT(data[1], WithinAbs(7.0, 1e-14));
    }

    SECTION("DC signal") {
        const unsigned long n = 16;
        const double dc_value = 5.0;
        std::vector<double> data(n, dc_value);

        nrngsl_realft(data.data(), n, 1);
        // DC component at index 0 should be n * dc_value
        REQUIRE_THAT(data[0], WithinAbs(n * dc_value, 1e-14));
        // All AC frequency components should be zero
        for (unsigned long i = 1; i < n; ++i) {
            REQUIRE_THAT(data[i], WithinAbs(0.0, 1e-14));
        }

        nrngsl_realft(data.data(), n, -1);
        for (unsigned long i = 0; i < n; ++i) {
            REQUIRE_THAT(data[i], WithinAbs(dc_value, 1e-14));
        }
    }

    SECTION("Impulse (Kronecker delta)") {
        const unsigned long n = 16;
        std::vector<double> data(n, 0.0);
        data[0] = 1.0;

        nrngsl_realft(data.data(), n, 1);
        // FFT of delta at 0 has constant magnitude 1 across all frequencies
        // DC at 0 = 1, Nyquist at n/2 = 1, real parts = 1, imag parts = 0
        REQUIRE_THAT(data[0], WithinAbs(1.0, 1e-14));
        REQUIRE_THAT(data[n / 2], WithinAbs(1.0, 1e-14));
        for (unsigned long k = 1; k < n / 2; ++k) {
            REQUIRE_THAT(data[k], WithinAbs(1.0, 1e-14));      // Re(X_k)
            REQUIRE_THAT(data[n - k], WithinAbs(0.0, 1e-14));  // Im(X_k)
        }
    }

    SECTION("Pure Cosine wave at fundamental frequency") {
        const unsigned long n = 32;
        std::vector<double> data(n);
        // x[i] = cos(2*pi*i / n) -> frequency k = 1
        for (unsigned long i = 0; i < n; ++i) {
            data[i] = std::cos(2.0 * M_PI * static_cast<double>(i) / static_cast<double>(n));
        }

        nrngsl_realft(data.data(), n, 1);

        // Expect non-zero amplitude only at k = 1: Re(X_1) = n / 2 = 16
        REQUIRE_THAT(data[1], WithinAbs(static_cast<double>(n) / 2.0, 1e-12));
        REQUIRE_THAT(data[n - 1], WithinAbs(0.0, 1e-12));  // Im(X_1) is 0 for cosine

        // DC should be 0
        REQUIRE_THAT(data[0], WithinAbs(0.0, 1e-12));
        // All other frequencies should be zero
        for (unsigned long k = 2; k <= n / 2; ++k) {
            REQUIRE_THAT(data[k], WithinAbs(0.0, 1e-12));
            if (k < n / 2) {
                REQUIRE_THAT(data[n - k], WithinAbs(0.0, 1e-12));
            }
        }
    }
}

TEST_CASE("FFT Parseval theorem / energy conservation", "[Neuron][fft]") {
    DeterministicRng rng(12345);

    const std::vector<unsigned long> sizes = {4, 8, 16, 64, 256, 1024};
    for (unsigned long n: sizes) {
        DYNAMIC_SECTION("Parseval for size " << n) {
            std::vector<double> x(n);
            double time_energy = 0.0;
            for (unsigned long i = 0; i < n; ++i) {
                x[i] = rng.next_double(-10.0, 10.0);
                time_energy += x[i] * x[i];
            }

            auto H = x;
            nrngsl_realft(H.data(), n, 1);

            // Compute energy in half-complex format:
            // (1/n) * [ |H[0]|^2 + |H[n/2]|^2 + 2 * sum_{k=1}^{n/2-1} (|H[k]|^2 + |H[n-k]|^2) ]
            double freq_energy = H[0] * H[0] + H[n / 2] * H[n / 2];
            for (unsigned long k = 1; k < n / 2; ++k) {
                freq_energy += 2.0 * (H[k] * H[k] + H[n - k] * H[n - k]);
            }
            freq_energy /= static_cast<double>(n);

            REQUIRE_THAT(freq_energy, WithinAbs(time_energy, 1e-9 * time_energy));
        }
    }
}

TEST_CASE("FFT linearity property", "[Neuron][fft]") {
    DeterministicRng rng(54321);

    const unsigned long n = 64;
    std::vector<double> x(n);
    std::vector<double> y(n);
    std::vector<double> combined(n);
    const double alpha = 2.5;
    const double beta = -1.7;

    for (unsigned long i = 0; i < n; ++i) {
        x[i] = rng.next_double(-5.0, 5.0);
        y[i] = rng.next_double(-5.0, 5.0);
        combined[i] = alpha * x[i] + beta * y[i];
    }

    auto H_x = x;
    auto H_y = y;
    auto H_comb = combined;

    nrngsl_realft(H_x.data(), n, 1);
    nrngsl_realft(H_y.data(), n, 1);
    nrngsl_realft(H_comb.data(), n, 1);

    for (unsigned long i = 0; i < n; ++i) {
        double expected = alpha * H_x[i] + beta * H_y[i];
        REQUIRE_THAT(H_comb[i], WithinAbs(expected, 1e-11));
    }
}

TEST_CASE("FFT format conversions nrn_gsl2nrc and nrn_nrc2gsl", "[Neuron][fft]") {
    const unsigned long n = 8;
    std::vector<double> original = {10.0, 2.0, 3.0, 4.0, -5.0, 0.5, -1.5, 2.5};
    std::vector<double> nrc(n, 0.0);
    std::vector<double> restored(n, 0.0);

    nrn_gsl2nrc(original.data(), nrc.data(), n);
    nrn_nrc2gsl(nrc.data(), restored.data(), n);

    // Note: nrn_nrc2gsl applies a scale factor of n/2 (0.5 * n = 4)
    const double scale = 0.5 * n;
    for (unsigned long i = 0; i < n; ++i) {
        REQUIRE_THAT(restored[i], WithinAbs(original[i] * scale, 1e-14));
    }
}

TEST_CASE("FFT correlation properties", "[Neuron][fft]") {
    const unsigned long n = 4;
    std::vector<double> v1 = {3.0, 2.0, 15.0, 16.0};
    std::vector<double> v2 = v1;
    std::vector<double> ans(n, 0.0);

    SECTION("Autocorrelation matches expected vector") {
        nrn_correl(v1.data(), v2.data(), n, ans.data());
        // Expected from test_vector_api: [494.0, 324.0, 154.0, 324.0]
        REQUIRE_THAT(ans[0], WithinAbs(494.0, 1e-12));
        REQUIRE_THAT(ans[1], WithinAbs(324.0, 1e-12));
        REQUIRE_THAT(ans[2], WithinAbs(154.0, 1e-12));
        REQUIRE_THAT(ans[3], WithinAbs(324.0, 1e-12));
    }

    SECTION("Autocorrelation lag 0 equals sum of squares") {
        const unsigned long sz = 16;
        std::vector<double> data(sz);
        double sum_sq = 0.0;
        for (unsigned long i = 0; i < sz; ++i) {
            data[i] = static_cast<double>(i + 1);
            sum_sq += data[i] * data[i];
        }
        std::vector<double> autocorr(sz);
        auto copy = data;
        nrn_correl(data.data(), copy.data(), sz, autocorr.data());
        REQUIRE_THAT(autocorr[0], WithinAbs(sum_sq, 1e-12));

        // Cauchy-Schwarz: autocorrelation lag 0 is maximum
        for (unsigned long i = 1; i < sz; ++i) {
            REQUIRE(std::abs(autocorr[i]) <= autocorr[0]);
        }
    }
}

TEST_CASE("FFT error handling for non-power-of-two sizes", "[Neuron][fft]") {
    std::vector<double> buf(33, 1.0);
    // Non-powers of two (> 1) must throw an execerror exception
    for (unsigned long invalid_n: {3, 5, 6, 7, 9, 10, 15, 33}) {
        REQUIRE_THROWS(nrngsl_realft(buf.data(), invalid_n, 1));
        REQUIRE_THROWS(nrngsl_realft(buf.data(), invalid_n, -1));
    }
    // n <= 1 is a no-op identity
    REQUIRE_NOTHROW(nrngsl_realft(buf.data(), 0, 1));
    REQUIRE_NOTHROW(nrngsl_realft(buf.data(), 1, 1));
}

// Verify numerical accuracy against golden reference outputs within floating-point tolerance.
TEST_CASE("FFT golden reference numerical accuracy", "[Neuron][fft]") {
    SECTION("Size 8 golden accuracy") {
        const std::vector<double> input_8 = {1.0, 2.0, 3.5, 4.0, 5.5, 6.0, 7.5, 8.0};
        const std::vector<double> golden_fwd_8 = {
            37.5, -4.5, -4.5, -4.5, -2.5, 1.6568542494923797, 4.0, 9.6568542494923797};
        const std::vector<double> golden_inv_8 = {1.0, 2.0, 3.5, 4.0, 5.5, 6.0, 7.5, 8.0};

        auto buf8 = input_8;
        nrngsl_realft(buf8.data(), 8, 1);
        for (size_t i = 0; i < 8; ++i) {
            REQUIRE_THAT(buf8[i], WithinAbs(golden_fwd_8[i], 1e-13));
        }

        nrngsl_realft(buf8.data(), 8, -1);
        for (size_t i = 0; i < 8; ++i) {
            REQUIRE_THAT(buf8[i], WithinAbs(golden_inv_8[i], 1e-13));
        }
    }

    SECTION("Size 16 golden accuracy") {
        const std::vector<double> input_16 = {
            0.5, -1.0, 2.5, 3.0, -0.5, 1.5, 4.0, -2.0, 3.5, 1.0, -1.5, 2.0, 0.0, -3.0, 2.5, 5.0};
        const std::vector<double> golden_fwd_16 = {17.5,
                                                   2.0477726022449869,
                                                   4.146446609406726,
                                                   0.37922857264858223,
                                                   -4.0,
                                                   -9.91476247858132,
                                                   4.853553390593274,
                                                   -4.5122386963122496,
                                                   4.5,
                                                   1.7519007589997411,
                                                   -7.9748737341529168,
                                                   14.808761967135585,
                                                   9.5,
                                                   6.0305873740835629,
                                                   3.0251262658470837,
                                                   -5.0262738340522803};
        const std::vector<double> golden_inv_16 = {
            0.5, -1.0, 2.5, 3.0, -0.5, 1.5, 4.0, -2.0, 3.5, 1.0, -1.5, 2.0, 0.0, -3.0, 2.5, 5.0};

        auto buf16 = input_16;
        nrngsl_realft(buf16.data(), 16, 1);
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE_THAT(buf16[i], WithinAbs(golden_fwd_16[i], 1e-13));
        }

        nrngsl_realft(buf16.data(), 16, -1);
        for (size_t i = 0; i < 16; ++i) {
            REQUIRE_THAT(buf16[i], WithinAbs(golden_inv_16[i], 1e-13));
        }
    }
}
