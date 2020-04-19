#include "globlib.h"

#define PATH_SEPARATOR '/'

bool match(const char *path, const char *glob, const char flags) {
    // Flags.
    const bool MATCH_DOTFILES = (flags & 1);

    // * wildcard savepoint.
    const char* starTextSavepoint = NULL;
    const char* starGlobSavepoint = NULL;

    // ** wildcard savepoint.
    const char* dstarTextSavepoint = NULL;
    const char* dstarGlobSavepoint = NULL;

    bool isDotLegal = MATCH_DOTFILES;
    bool invertMatch = false;

    // Invert if necessary.
    if(*glob == '!' || *glob == '^') {
        invertMatch = true;
        ++glob;
    }

    // If the glob contains path separator, match the path relatively.
    if (*glob == PATH_SEPARATOR) {
        // If the path contains ./ pairs, ignore them.
        while (*path == '.' && path[1] == PATH_SEPARATOR)
            path += 2;

        // If the path starts with a path separator, ignore it.
        if (*path == PATH_SEPARATOR)
            ++path;
        ++glob;
    }
    else if (strchr(glob, PATH_SEPARATOR) == NULL) {
        const char *offset = strrchr(path, PATH_SEPARATOR);
        if (offset)
            path = offset + 1;
    }
    while (*path != '\0') {
        switch (*glob) {
            case '*': {
                // Match anything except dot after path separator.
                if (!isDotLegal && *path == '.')
                    break;
                if (*++glob == '*') {
                    // Match everything after path separator.
                    if (*++glob == '\0')
                        return !invertMatch;

                    // Match zero or more directories.
                    if (*glob != PATH_SEPARATOR)
                        return invertMatch;

                    // Create ** savepoint, discard * savepoint.
                    starTextSavepoint = NULL;
                    starGlobSavepoint = NULL;

                    dstarTextSavepoint = path;
                    dstarGlobSavepoint = glob++;
                    continue;
                }
                // Match everything except path separator.
                starTextSavepoint = path;
                starGlobSavepoint = glob;
                continue;
            }
            case '?': {
                // Match anything except dot after path separator.
                if (!isDotLegal && *path == '.')
                    break;

                // Match any character except path separator.
                if (*path == PATH_SEPARATOR)
                    break;

                ++path;
                ++glob;
                continue;
            }
            case '[': {
                ++glob;

                char last = 0xFF;
                bool matched;
                bool negated;

                // Match anything except dot after path separator.
                if (!isDotLegal && *path == '.')
                    break;

                // Match any character in class except path separator.
                if (*path == PATH_SEPARATOR)
                    break;

                // Inverted character class.
                negated = (*glob == '^' || *glob == '!');

                if (negated)
                    ++glob;

                matched = false;

                // Treat the first dash as a normal character.
                if(*glob == '-') {
                    if(*path == '-')
                        matched = true;
                    else last = '-';

                    ++glob;
                }

                // Match character class.
                for (; !matched && *glob != ']' && *glob != '\0'; last = *(glob++)) {
                    if (*glob == '-' && glob[1] != ']' && glob[1] != '\0') {
                        if ((*path <= glob[1]) && (*path >= last)) {
                            // Range match.
                            matched = true;
                        }
                    } else if(*path == *glob) {
                        // Exact match.
                        matched = true;
                    }
                }

                // No match.
                if (matched == negated)
                    break;

                while (*glob != ']' && *glob != '\0')
                    ++glob;

                if(*glob == ']')
                    ++glob;
                ++path;

                continue;
            }
            case '\\': {
                // Escaped character. Fallthrough.
                ++glob;
            }
            default: {
                // Match the current non-separator character.
                if (*glob != *path && !(*glob == PATH_SEPARATOR && *path == PATH_SEPARATOR))
                    break;

                // Do not match a dot with *, ? and [] after path separator.
                isDotLegal = MATCH_DOTFILES || *glob != PATH_SEPARATOR;

                ++path;
                ++glob;

                continue;
            }
        }

        if (starGlobSavepoint != NULL && *starTextSavepoint != PATH_SEPARATOR) {
            // Go back to the * savepoint, do not jump over path separator.
            path = ++starTextSavepoint;
            glob = starGlobSavepoint;
            continue;
        }

        if (dstarGlobSavepoint != NULL) {
            // Go back to the ** savepoint.
            path = ++dstarTextSavepoint;
            glob = dstarGlobSavepoint;
            continue;
        }

        // No match.
        return invertMatch;
    }

    // Ignore trailing stars.
    while (*glob == '*')
        glob++;

    // Match if the glob end was reached.
    return (*glob == '\0') != invertMatch;
}