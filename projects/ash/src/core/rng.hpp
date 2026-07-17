#pragma once
/// Phase 01 step 0108-dep: xoshiro256** PRNG (Blackman / Vigna).
/// Reference: https://prng.di.unimi.it/xoshiro256starstar.c
/// Used by `render::LightGrid::add_flicker` and any other engine feature
/// that needs fast, high-quality random integers with deterministic seed.
#include <cstdint>

namespace ash {
namespace core {
namespace rng {

class Xoshiro256pp {
public:
    Xoshiro256pp() noexcept;
    explicit Xoshiro256pp(std::uint64_t seed) noexcept;

    void seed(std::uint64_t s) noexcept;

    /// Returns a 64-bit pseudorandom value.
    std::uint64_t next() noexcept;

    /// Uniform integer in [0, bound). bound must be > 0.
    std::uint64_t next_bounded(std::uint64_t bound) noexcept;

    /// Uniform integer in [lo, hi].
    int next_int(int lo, int hi) noexcept;

private:
    std::uint64_t s_[4]{};
    static std::uint64_t rotl(std::uint64_t x, int k) noexcept;
};

}  // namespace rng
}  // namespace core
}  // namespace ash