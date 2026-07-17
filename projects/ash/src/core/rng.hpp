#pragma once
/// Phase 01 step 0100/0108: xoshiro256** PRNG (Blackman-Vigna).
#include <cstdint>

namespace ash {
namespace core {
namespace rng {

/// xoshiro256** — fast, statistically robust 64-bit PRNG.
/// Public domain reference: Vigna, "xoshiro / xoroshiro generators" (2018).
class Xoshiro256pp {
public:
    using result_type = uint64_t;

    Xoshiro256pp() noexcept { seed(0); }
    explicit Xoshiro256pp(uint64_t s) noexcept { seed(s); }

    void seed(uint64_t s) noexcept {
        // SplitMix64 warmup to expand a single seed into 256 bits of state.
        for (int i = 0; i < 4; ++i) {
            s += 0x9E3779B97F4A7C15ULL;
            uint64_t z = s;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            s_[i] = z ^ (z >> 31);
        }
    }

    uint64_t next() noexcept {
        const uint64_t result = rotl(s_[1] * 5, 7) * 9;
        const uint64_t t = s_[1] << 17;
        s_[2] ^= s_[0];
        s_[3] ^= s_[1];
        s_[1] ^= s_[2];
        s_[0] ^= s_[3];
        s_[2] ^= t;
        s_[3] = rotl(s_[3], 45);
        return result;
    }

    /// Uniform integer in [0, bound).
    uint64_t uniform(uint64_t bound) noexcept {
        if (bound == 0) return 0;
        uint64_t mask = bound - 1;
        if ((bound & mask) == 0) {
            return next() & mask;
        }
        uint64_t r;
        do { r = next(); } while (r >= (UINT64_C(-1) - (UINT64_C(-1) % bound) - bound));
        return r % bound;
    }

    static constexpr uint64_t min() noexcept { return 0; }
    static constexpr uint64_t max() noexcept { return UINT64_C(-1); }

private:
    static uint64_t rotl(uint64_t x, int k) noexcept {
        return (x << k) | (x >> (64 - k));
    }

    uint64_t s_[4]{};
};

}  // namespace rng
}  // namespace core
}  // namespace ash