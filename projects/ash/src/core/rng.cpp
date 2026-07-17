#include "core/rng.hpp"

#include <chrono>

namespace ash {
namespace core {
namespace rng {

namespace {

/// SplitMix64 expansion — used to derive 4 distinct state words from a
/// single 64-bit seed. Avoids the degenerate all-zero state.
std::uint64_t splitmix64(std::uint64_t& state) noexcept {
    std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

}  // namespace

std::uint64_t Xoshiro256pp::rotl(std::uint64_t x, int k) noexcept {
    return (x << k) | (x >> (64 - k));
}

Xoshiro256pp::Xoshiro256pp() noexcept {
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    seed(static_cast<std::uint64_t>(now) ^ 0xCAFEF00DDEADBEEFULL);
}

Xoshiro256pp::Xoshiro256pp(std::uint64_t seed_val) noexcept {
    seed(seed_val);
}

void Xoshiro256pp::seed(std::uint64_t s) noexcept {
    std::uint64_t z = s;
    for (int i = 0; i < 4; ++i) {
        s_[i] = splitmix64(z);
    }
    if (!(s_[0] | s_[1] | s_[2] | s_[3])) {
        s_[0] = 1;  // avoid degenerate all-zero state
    }
}

std::uint64_t Xoshiro256pp::next() noexcept {
    std::uint64_t const result = rotl(s_[1] * 5, 7) * 9;
    std::uint64_t const t = s_[1] << 17;
    s_[2] ^= s_[0];
    s_[3] ^= s_[1];
    s_[1] ^= s_[2];
    s_[0] ^= s_[3];
    s_[2] ^= t;
    s_[3] = rotl(s_[3], 45);
    return result;
}

std::uint64_t Xoshiro256pp::next_bounded(std::uint64_t bound) noexcept {
    if (bound == 0) return 0;
    // Lemire nearly-divisionless bounded reduction.
    std::uint64_t m = static_cast<std::uint64_t>(
        (static_cast<__uint128_t>(next()) * bound) >> 64);
    if (m < bound) {
        std::uint64_t const reject = (0ULL - bound) % bound;
        while (m < reject) {
            m = static_cast<std::uint64_t>(
                (static_cast<__uint128_t>(next()) * bound) >> 64);
        }
    }
    return m;
}

int Xoshiro256pp::next_int(int lo, int hi) noexcept {
    if (hi <= lo) return lo;
    std::uint64_t const span = static_cast<std::uint64_t>(hi - lo + 1);
    return lo + static_cast<int>(next_bounded(span));
}

}  // namespace rng
}  // namespace core
}  // namespace ash