#ifndef GLOBLIB_H_GUARD
#define GLOBLIB_H_GUARD

#include <string.h>
#include <stdbool.h>

#define GLOB_NO_FLAGS       0x0
#define GLOB_MATCH_DOTFILES 0x1

#ifdef __cplusplus
extern "C" {
#endif

/// Returns true if a path matches a modern glob. False otherwise.
bool match(const char *path, const char *glob, const char flags);

#ifdef __cplusplus
}
#endif

#endif
