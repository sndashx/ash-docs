#pragma once

/// ASH engine version constants and accessors.
///
/// Phase 00 contract (spec step 0005):
///   - ASH_VERSION_MAJOR/MINOR/PATCH = 0 / 0 / 1
///   - ASH_VERSION_STRING = "0.0.1"
///   - ASH_CODENAME = "First Spark"
///   - ash_version_string(), ash_version_major/minor/patch/codename()
namespace ash
{
namespace core
{
inline constexpr int ASH_VERSION_MAJOR = 0;
inline constexpr int ASH_VERSION_MINOR = 0;
inline constexpr int ASH_VERSION_PATCH = 1;
inline constexpr const char* ASH_VERSION_STRING = "0.0.1";
inline constexpr const char* ASH_CODENAME = "First Spark";

/// Returns the full dotted version string, e.g. "0.0.1".
const char* ash_version_string();

/// Returns the major version component.
int ash_version_major();

/// Returns the minor version component.
int ash_version_minor();

/// Returns the patch version component.
int ash_version_patch();

/// Returns the release codename, e.g. "First Spark".
const char* ash_version_codename();
}  // namespace core
}  // namespace ash
