#pragma once
#include "../objects/objectType.h"

namespace magma
{
    namespace profile
    {
        /* Compile-time profile description of Vulkan API function.
           If function belongs to debugging functionality (e.g. vkSubmitDebugUtilsMessageEXT),
           then <debugScope> should be true. This allows to exclude debugging functions
           from profiling if this isn't neccessary. */

        struct ApiEntryDescription
        {
            const char *const entryName;
            const bool debugScope;
        };

        /* Compile-time profile description of class method that is inherited
           from base Object class and has associated Vulkan object type. */

        struct MethodDescription
        {
            const VkObjectType objectType;
            const char *const methodName;
            const char *const fileName;
            const long line;
        };

        /* Compile-time profile description of non-Object method and standalone function. */

        struct FunctionDescription
        {
            const char *const functionName;
            const char *const fileName;
            const long line;
        };
    } // namespace profile
} // namespace magma
