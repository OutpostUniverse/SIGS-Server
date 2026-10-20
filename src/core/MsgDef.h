// Reconstructed interface, not recovered Sierra source. See README.md.
#ifndef SIGS_CORE_MSGDEF_H
#define SIGS_CORE_MSGDEF_H
#include <stddef.h>

namespace sigs::core
{
    struct MessageDefinition
    {
        unsigned int id;
        const char* format;
        const char* descriptive_name; // Project label; not an original symbol.
    };

    enum Registry
    {
        SierraNW,
        SNWValid
    };

    const MessageDefinition* GetMessageDefinitions(Registry registry, size_t* count);
    const MessageDefinition* FindMessageDefinition(Registry registry, unsigned int id);
} // namespace sigs::core
#endif
