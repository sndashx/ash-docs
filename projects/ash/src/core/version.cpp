#include "core/version.hpp"

namespace ash
{
namespace core
{
const char* ash_version_string()
{
    return ASH_VERSION_STRING;
}

int ash_version_major()
{
    return ASH_VERSION_MAJOR;
}

int ash_version_minor()
{
    return ASH_VERSION_MINOR;
}

int ash_version_patch()
{
    return ASH_VERSION_PATCH;
}

const char* ash_version_codename()
{
    return ASH_CODENAME;
}
}  // namespace core
}  // namespace ash
