#include <stack>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <unordered_set>

#include "engine.hxx"

#include "../def/osh.dxx"
#include "../def/logging.dxx"

#include "../../lib/str.hxx"
#include "../../lib/path.hxx"
#include "../../lib/remem.hxx"
#include "../../lib/buffer.hxx"
#include "../../lib/mem.hxx"
#include "../../lib/chunk.cxx"

#ifdef _MSC_VER
    #include "../../include/dirent.h"
#else
    #include <dirent.h>
#endif

static constexpr auto COMPILER_PATH_SEPARATOR   = '/';
static constexpr auto COMPILER_ERROR_CHUNK_SIZE = 20u;
static constexpr auto COMPILER_PATH_MAX_LENGTH  = 4096u;

static uint8_t* componentPathToAbsolute(const char* wd, const char* componentPath, size_t componentPathLength, size_t &absoluteLength);
static void     localizeIterator(const uint8_t* iterator, size_t iteratorLength, Buffer& src);
static void     compiler_error(const char* file, const char* message, const char* description, ConstBuffer& input, size_t errorIndex);

void Eryn::Engine::compile(const char* path) {
    LOG_DEBUG("===> Compiling file '%s'", path);

    cache.add(path, std::move(compileFile(path)));

    LOG_DEBUG("===> Done\n");
}

void Eryn::Engine::compileString(const char* alias, const char* str) {
    LOG_DEBUG("===> Compiling string '%s'", alias);

    ConstBuffer input(str, strlen(str));
    cache.add(alias, compileBytes(input, opts.workingDir.c_str(), alias));

    LOG_DEBUG("===> Done\n");
}

void Eryn::Engine::compileDir(const char* path, std::vector<string> filters) {
    LOG_DEBUG("===> Compiling directory '%s'", path);

    FilterInfo info;

    for(const auto& filter : filters) {
        const char* start = filter.c_str();
        const char* end   = start + filter.size() - 1;

        bool inverted = false;

        // Trim because the filters shouldn't have trailing spaces.
        while((*start == ' ' || *start == '"') && start < end) {
            ++start;
        }
        while((*end == ' ' || *end == '"') && end > start) {
            --end;
        }

        if(start < end) {
            if(*start == '!' || *start == '^') {
                inverted = true;
                ++start;
            }

            if(start < end) {
                if(inverted) {
                    info.addExclusion(start, end - start + 1);
                } else {
                    info.addFilter(start, end - start + 1);
                }
            }
        }
    }

    compileDir(path, "", info);

    LOG_DEBUG("===> Done\n");
}

// 'rel' is relative to the working directory, and is used for filtering
// 'path' is the full path and is used to read the directory
void Eryn::Engine::compileDir(const char* path, const char* rel, const FilterInfo& info) {
    DIR* dir;
    struct dirent* entry;

    if((dir = opendir(path)) != nullptr) {
        auto absoluteLength = strlen(path);
        auto relLength      = strlen(rel);

        char absolute[COMPILER_PATH_MAX_LENGTH + 1];

        // TODO: add a formatting function
        if(absoluteLength >= COMPILER_PATH_MAX_LENGTH) {
            throw Eryn::CompilationException(path, "Path is too long", ("The maximum allowed path size is " + std::to_string(COMPILER_PATH_MAX_LENGTH)).c_str());
        }

        strcpy(absolute, path);
        absolute[absoluteLength] = COMPILER_PATH_SEPARATOR;
        absolute[absoluteLength + 1] = '\0';

        char* absoluteEnd = absolute + absoluteLength + 1;

        while((entry = readdir(dir)) != nullptr) {
            auto nameLength = strlen(entry->d_name);

            if(entry->d_type == DT_REG) {
                strcpy(&absolute[absoluteLength + 1], entry->d_name);

                std::unique_ptr<char[]> relativePath(
                    new("CompileDir relative path") char[relLength + nameLength + 1]);

                if(relLength > 0) {
                    strcpy(relativePath.get(), rel);
                }

                strcpy(relativePath.get() + relLength, entry->d_name);

                if(info.isFileFiltered(relativePath.get())) {
                    try {
                        compile(absolute);

                        if(opts.flags.debugDumpOSH) {
                            FILE* dump = fopen((absolute + std::string(".osh")).c_str(), "wb");
                            fwrite(cache.get(absolute).data, 1, cache.get(absolute).size, dump);
                            fclose(dump);
                        }
                    } catch(CompilationException& e) {
                        if(opts.flags.throwOnCompileDirError) {
                            throw e;
                        }
                        LOG_ERROR("Error: %s", e.what());
                    }
                }
                else LOG_DEBUG("Ignoring: %s\n", relativePath.get());
            } else if(entry->d_type == DT_DIR) {
                if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;

                const size_t newRelLength = relLength + nameLength;

                std::unique_ptr<char[]> newRel(
                    new("CompileDir new relative path") char[newRelLength + 2]);

                if(relLength > 0) {
                    strcpy(newRel.get(), rel);
                }
                strcpy(newRel.get() + relLength, entry->d_name);

                newRel.get()[newRelLength] = COMPILER_PATH_SEPARATOR;
                newRel.get()[newRelLength + 1] = '\0';

                if(info.isDirFiltered(newRel.get())) {
                    LOG_DEBUG("Scanning: %s\n", newRel.get());

                    strcpy(absoluteEnd, entry->d_name);
                    compileDir(absolute, newRel.get(), info);
                } else LOG_DEBUG("Ignoring: %s\n", newRel.get());
            }
        }
        closedir(dir);
    }
}

Buffer Eryn::Engine::compileFile(const char* path) {
    FILE* input = fopen(path, "rb");

    if(input == NULL) {
        throw CompilationException(path, "IO error", "cannot open file");
    }

    fseek(input, 0, SEEK_END);
    long fileLength = ftell(input);
    fseek(input, 0, SEEK_SET);

    LOG_DEBUG("File size is %ld bytes\n", fileLength);

    size_t inputSize = (size_t) fileLength;

    std::unique_ptr<uint8_t[]> inputPtr(
        new("CompileFile input buffer") uint8_t[inputSize]);

    fread(inputPtr.get(), 1, inputSize, input);
    fclose(input);

    ConstBuffer inputBuffer(inputPtr.get(), inputSize);
    string wd(path, path::dir_end_index(path, strlen(path)));

    return compileBytes(inputBuffer, wd.c_str(), path);
}

enum class TemplateType {
    CONDITIONAL,
    ELSE,
    ELSE_CONDITIONAL,
    LOOP,
    COMPONENT
};

struct TemplateStackInfo {
    TemplateType type;
    size_t inputIndex;      // The index at which the template starts in the input (provides more information when an exception occurs).
    size_t outputIndex;     // The index at which the template starts in the output. Points at the start of the OSH data.
    size_t outputBodyIndex; // The index at which the template body starts in the output (for writing the body size). Points immediately after the OSH data (1 byte after). 

    TemplateStackInfo(TemplateType typ, size_t body, size_t input, size_t output) :
        type(typ), outputBodyIndex(body), inputIndex(input), outputIndex(output) { };
};

struct CompilerInfo {
    std::stack<TemplateStackInfo> templates;
    std::vector<ConstBuffer> iterators;
};

// 'wd' is the working directory, which is necessary to find components
// 'path' is either the full path of the source file, or the alias of the source string
Buffer Eryn::Engine::compileBytes(ConstBuffer& input, const char* wd, const char* path) {
    Buffer output;
    const BDP::Header BDP832 = BDP::Header(8, 32);

    auto start = input.data;
    auto end   = input.find(opts.templates.start);
    size_t length = end - start;

    const uint8_t* limit  = input.end();

    size_t remainingLength;
    size_t index;
    size_t templateStartIndex;
    size_t templateEndIndex;

    CompilerInfo compiler;

    while(end < limit) {
        if(start != end) {
            LOG_DEBUG("--> Found plaintext at %zu", start - input);

            bool skip = false;
            if(opts.flags.ignoreBlankPlaintext) {
                skip = true;
                const uint8_t* i = start;

                while(skip && i != end) {
                    if(!str::is_blank(*i)) {
                        skip = false;
                    }
                    ++i;
                }
            }

            if(!skip) {
                LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - input, end - input);
                output.write_bdp_pair(BDP832, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_LENGTH, start, length);
                LOG_DEBUG("done\n");
            } else LOG_DEBUG("Skipping blank plaintext");
        }
        templateStartIndex = end - input.data;

        LOG_DEBUG("--> Found template start at %zu", templateStartIndex);

        end += opts.templates.start.size();

        while(str::is_blank(*end)) {
            ++end;
        }

        if(input.match(end - input.data, opts.templates.commentStart)) {
            LOG_DEBUG("Detected comment template");

            end += opts.templates.commentStart.size();

            while(str::is_blank(*end)) {
                ++end;
            }

            start = end;
            remainingLength = input.size - (end - input.data);
            index = input.find_index(end - input.data, opts.templates.commentEnd) - (end - input.data);

            while(index < remainingLength && *(end + index - 1) == opts.templates.escape) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                remainingLength -= index + 1;
                index = input.find_index((end + index + 1) - input.data, opts.templates.commentEnd) - (end - input.data);
            }

            templateEndIndex = (end + index) - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + index) - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = start;
            templateEndIndex += opts.templates.commentEnd.size();
        } else if(input.match(end - input.data, opts.templates.conditionalStart)) {
            LOG_DEBUG("Detected conditional template start");

            end += opts.templates.conditionalStart.size();

            while(str::is_blank(*end)) {
                ++end;
            }

            start = end;
            remainingLength = input.size - (end - input.data);
            index = input.find_index(end - input.data, opts.templates.end) - (end - input.data);

            std::vector<const uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == opts.templates.escape) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index = input.find_index((end + index + 1) - input.data, opts.templates.end) - (end - input.data);
            }

            templateEndIndex = end + index - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + index) - input.data - 1);
            }

            if(index == 0)
                compiler_error(path, "Unexpected template end", "did you forget to write the condition?", input, end - input.data - 1);

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(str::is_blank(*end)) {
                --end;
            }
            ++end;

            if(start != end) {
                length = end - start;

                // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.

                Buffer buffer;

                index = 0;

                for(size_t i = 0; i < escapes.size(); ++i) {
                    buffer.write(start, escapes[i] - start);
                    start = escapes[i] + 1;
                }

                if(length > 0) {
                    buffer.write(start, end - start);
                }

                if(!compiler.iterators.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(auto iterator : compiler.iterators) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localizeIterator(iterator.data, iterator.size, buffer);
                        }
                    }
                }

                size_t oshStart = output.size;

                LOG_DEBUG("Writing conditional template start as BDP832 pair %zu -> %zu...", start - input, end - input);
                output.write_bdp_pair(BDP832, OSH_TEMPLATE_CONDITIONAL_START_MARKER, OSH_TEMPLATE_CONDITIONAL_START_LENGTH, buffer.data, buffer.size);

                for(auto i = 0; i < OSH_FORMAT; ++i) {
                    output.write(0); // End index.
                }

                for(auto i = 0; i < OSH_FORMAT; ++i) {
                    output.write(0); // True end index;
                }
                
                compiler.templates.push(TemplateStackInfo(TemplateType::CONDITIONAL, output.size, templateStartIndex, oshStart));
                LOG_DEBUG("done\n");
            }

            end = start;
            templateEndIndex += opts.templates.end.size();
        } else if(input.match(end - input.data, opts.templates.elseConditionalStart)) {
            LOG_DEBUG("Detected else conditional template start");

            end += opts.templates.elseConditionalStart.size();

            while(str::is_blank(*end)) {
                ++end;
            }

            if(compiler.templates.empty() || (compiler.templates.top().type != TemplateType::CONDITIONAL && compiler.templates.top().type != TemplateType::ELSE_CONDITIONAL)) {
                compiler_error(path, "Unexpected else conditional template", "there is no preceding conditional template; delete this", input, templateStartIndex);
            }

            start = end;
            remainingLength = input.size - (end - input.data);
            index = input.find_index(end - input.data, opts.templates.end) - (end - input.data);

            std::vector<const uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == opts.templates.escape) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index = 1 + input.find_index((end + index + 1) - input.data, opts.templates.end) - (end - input.data);
            }

            templateEndIndex = end + index - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + index) - input.data - 1);
            }

            if(index == 0) {
                compiler_error(path, "Unexpected template end", "did you forget to write the condition?", input, end - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(str::is_blank(*end)) {
                --end;
            }
            ++end;

            if(start != end) {
                length = end - start;

                // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.

                Buffer buffer;

                index = 0;

                for(size_t i = 0; i < escapes.size(); ++i) {
                    buffer.write(start, escapes[i] - start);
                    start = escapes[i] + 1;
                }

                if(length > 0) {
                    buffer.write(start, end - start);
                }

                if(!compiler.iterators.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(auto iterator : compiler.iterators) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localizeIterator(iterator.data, iterator.size, buffer);
                        }
                    }
                }

                size_t oshStart = output.size;

                LOG_DEBUG("Writing else conditional template start as BDP832 pair %zu -> %zu...", start - input, end - input);
                output.write_bdp_pair(BDP832, OSH_TEMPLATE_ELSE_CONDITIONAL_START_MARKER, OSH_TEMPLATE_ELSE_CONDITIONAL_START_LENGTH, buffer.data, buffer.size);

                for(auto i = 0; i < OSH_FORMAT; ++i) {
                    output.write(0); // End index.
                }

                for(auto i = 0; i < OSH_FORMAT; ++i) {
                    output.write(0); // True end index;
                }
                
                // Since the scope needs to be decremented and reincremented, just subtract 1 from it.
                // TODO: Check if this comment is still relevant.
                compiler.templates.push(TemplateStackInfo(TemplateType::ELSE_CONDITIONAL, output.size, templateStartIndex, oshStart));
                LOG_DEBUG("done\n");
            }

            end = start;
            templateEndIndex += opts.templates.end.size();
        } else if(input.match(end - input.data, opts.templates.elseStart)) {
            LOG_DEBUG("Detected else template start");

            start = end;
            remainingLength = input.size - (end - input.data);
            index = input.find_index(end - input.data, opts.templates.end) - (end - input.data);
            templateEndIndex = end + index - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template body?", input, (end + index) - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(str::is_blank(*end)) {
                --end;
            }

            if(start != end - opts.templates.elseStart.size() + 1) {
                // opts.templates.end.size()
                compiler_error(path, "Expected template body end", "else template must only contain the marker", input, start + 1 - input.data);
            }

            if(compiler.templates.empty() || (compiler.templates.top().type != TemplateType::CONDITIONAL && compiler.templates.top().type != TemplateType::ELSE_CONDITIONAL)) {
                compiler_error(path, "Unexpected else template", "there is no preceding conditional template; delete this", input, templateStartIndex);
            }

            length = end - start;

            // No OSH value, and no end indices.

            size_t oshStart = output.size;

            LOG_DEBUG("Writing else template start as BDP832 pair %zu -> %zu...", start - input, end - input);
            output.write_bdp_pair(BDP832, OSH_TEMPLATE_ELSE_START_MARKER, OSH_TEMPLATE_ELSE_START_LENGTH, NULL, 0);
            
            compiler.templates.push(TemplateStackInfo(TemplateType::ELSE, output.size, templateStartIndex, oshStart));
            LOG_DEBUG("done\n");

            end = start;
            templateEndIndex += opts.templates.end.size();
        } else if(input.match(end - input.data, opts.templates.loopStart)) {
            LOG_DEBUG("Detected loop template start");

            end += opts.templates.loopStart.size();

            while(str::is_blank(*end)) {
                ++end;
            }

            start = end;

            const uint8_t* leftStart = start;
            const uint8_t* leftEnd = start;
            size_t sepIndex;

            remainingLength = input.size - (end - input.data);
            sepIndex = input.find_index(end - input.data, opts.templates.loopSeparator) - (end - input.data);
            index    = index = input.find_index(end - input.data, opts.templates.end) - (end - input.data);

            std::vector<const uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == opts.templates.escape) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index = 1 + input.find_index((end + index + 1) - input.data, opts.templates.end) - (end - input.data);
            }

            templateEndIndex = end + index - input.data;

            if(sepIndex > index) {
                compiler_error(path, "Unexpected end of template", "did you forget to write the loop separator?", input, (end + index) - input.data - 1);
            }
            if(sepIndex == remainingLength) {
                compiler_error(path, "Unexpected EOF", "did you forget to write the loop separator?", input, (end + sepIndex) - input.data - 1);
            }
            if(sepIndex == 0) {
                compiler_error(path, "Unexpected separator", "did you forget to provide the left argument before the separator?", input, end - input.data);
            }
            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + index) - input.data - 1);
            }

            LOG_DEBUG("Found loop template separator at %zu", end + sepIndex - input);
            LOG_DEBUG("Found template end at %zu", end + index - input);

            leftEnd = end + sepIndex - 1;
            start = end + sepIndex + opts.templates.loopSeparator.size();

            while(*leftEnd == ' ' || *leftEnd == '\t')
                --leftEnd;
            ++leftEnd;

            end = end + index - 1;
            while(str::is_blank(*end)) {
                --end;
            }
            ++end;

            if(end == start) {
                compiler_error(path, "Unexpected end of template", "did you forget to provide the right argument after the separator?", input, (leftStart + index) - input.data - 1);
            }

            while(*start == ' ' || *start == '\t')
                ++start;

            length = end - start;
            size_t leftLength = leftEnd - leftStart;

            bool isReverse = false;
            const uint8_t* reverseStart;

            if(end - start + 1 >= opts.templates.loopReverse.size()) {
                reverseStart = end - opts.templates.loopReverse.size();

                if(input.match(reverseStart - input.data, opts.templates.loopReverse)) {
                    LOG_DEBUG("Detected reversed loop template");

                    isReverse = true;
                    end = reverseStart - 1;

                    while((str::is_blank(*end)) && end > start) {
                        --end;
                    }
                    ++end;

                    if(length != 0)
                        length = end - start;
                    else {
                        leftEnd = end;
                        leftLength = leftEnd - leftStart;
                    }
                }
            }

            // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.

            size_t bufferSize     = length - escapes.size();
            size_t bufferCapacity = bufferSize;
                
            Buffer buffer;
            index = 0;

            for(size_t i = 0; i < escapes.size(); ++i) {
                buffer.write(start, escapes[i] - start);
                start = escapes[i] + 1;
            }

            if(length > 0) {
                buffer.write(start, end - start);
            }

            if(!compiler.iterators.empty()) {
                LOG_DEBUG("Localizing iterators");

                // If 2 or more iterators share the same name, don't replace twice.
                std::unordered_set<std::string> iteratorSet;

                for(auto iterator : compiler.iterators) {
                    std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                    if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                        iteratorSet.insert(iteratorString);
                        localizeIterator(iterator.data, iterator.size, buffer);
                    }
                }
            }

            const uint8_t* oshStartMarker;
            size_t         oshStartMarkerLength;

            if(!isReverse) {
                oshStartMarker       = OSH_TEMPLATE_LOOP_START_MARKER;
                oshStartMarkerLength = OSH_TEMPLATE_LOOP_START_LENGTH;
            } else {
                oshStartMarker       = OSH_TEMPLATE_LOOP_REVERSE_START_MARKER;
                oshStartMarkerLength = OSH_TEMPLATE_LOOP_REVERSE_START_LENGTH;
            }

            size_t oshStart = output.size;

            LOG_DEBUG("Writing loop template start as BDP832 pair %zu -> %zu...", leftStart - input, end - input);
            output.write_bdp_name(BDP832, oshStartMarker, oshStartMarkerLength);

            Buffer tempBuffer;

            tempBuffer.write_bdp_value(BDP832, leftStart, leftLength);
            tempBuffer.write_bdp_value(BDP832, buffer.data, buffer.size);

            output.write_bdp_value(BDP832, tempBuffer.data, tempBuffer.size);
            
            for(auto i = 0; i < OSH_FORMAT; ++i) {
                output.write(0);
            }
                
            compiler.templates.push(TemplateStackInfo(TemplateType::LOOP, output.size, templateStartIndex, oshStart));
            compiler.iterators.push_back(ConstBuffer(leftStart, leftLength));

            LOG_DEBUG("done\n");

            end = leftStart;
            templateEndIndex += opts.templates.end.size();
        } else if(membcmp(end, Options::getTemplateComponent(), Options::getTemplateComponentLength())) {
            LOG_DEBUG("Detected component template");

            end += Options::getTemplateComponentLength();

            while((isBlank(*end)) && end < input + inputSize)
                ++end;

            start = end;

            const uint8_t* leftStart = start;
            const uint8_t* leftEnd = start;
            size_t sepIndex;

            remainingLength = inputSize - (end - input);
            sepIndex = mem_find(end, remainingLength, Options::getTemplateComponentSeparator(), Options::getTemplateComponentSeparatorLength(), Options::getTemplateComponentSeparatorLookup());
            index    = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            std::vector<const uint8_t*> escapes;

            // Escape is present before the template end.
            while(index < remainingLength && *(end + index - 1) == Options::getTemplateEscape()) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index += 1 + mem_find(end + index + 1, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            }

            templateEndIndex = end + index - input;

            if(sepIndex == 0)
                throwCompilationException(path, "Unexpected separator", "did you forget to provide the component name before the separator?", input, inputSize, end - input);
            if(templateEndIndex >= inputSize)
                throwCompilationException(path, "Unexpected EOF", "did you forget to close the template?", input, inputSize, (end + index) - input - 1);
            
            if(index == 0) {
                LOG_DEBUG("Detected empty component template");
            } else {
                LOG_DEBUG("Found template component at %zu", end + sepIndex - input);
                LOG_DEBUG("Found template end at %zu", end + index - input);

                templateEndIndex = end + index - input;

                if(sepIndex < index) {
                    leftEnd = end + sepIndex - 1;

                    while(*leftEnd == ' ' || *leftEnd == '\t')
                        --leftEnd;
                    ++leftEnd;

                    start = end + sepIndex + Options::getTemplateComponentSeparatorLength();
                    end = end + index - 1;
                    while(isBlank(*end))
                        --end;
                    ++end;

                    if(end == start)
                        throwCompilationException(path, "Unexpected end of template", "did you forget to provide the component context after the separator?", input, inputSize, (leftStart + index) - input - 1);

                    while(isBlank(*start))
                        ++start;
                    length = end - start;
                } else {
                    leftEnd = end + index - 1;

                    while(isBlank(*leftEnd))
                        --leftEnd;
                    ++leftEnd;

                    start = leftStart;
                    end = leftEnd;
                    length = 0;
                }

                size_t leftLength = leftEnd - leftStart;

                bool isSelf = false;
                const uint8_t* selfStart;

                if(end - start + 1 >= Options::getTemplateComponentSelfLength()) {
                    selfStart = end - Options::getTemplateComponentSelfLength();

                    if(membcmp(selfStart, Options::getTemplateComponentSelf(), Options::getTemplateComponentSelfLength())) {
                        LOG_DEBUG("Detected self-closing component template");

                        isSelf = true;
                        end = selfStart - 1;

                        while((isBlank(*end)) && end > start)
                            --end;
                        ++end;

                        if(length != 0)
                            length = end - start;
                        else {
                            leftEnd = end;
                            leftLength = leftEnd - leftStart;
                        }
                    }
                }

                if(end <= start)
                    throwCompilationException(path, "Unexpected end of template", "did you forget to provide the component context after the separator?", input, inputSize, (selfStart) - input);

                // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.

                if(length == 0 && escapes.size() > 0)
                    throwCompilationException(path, "Unexpected escape character(s) before end", "escape characters can only exist after the separator, which was not found; delete all escape characters", input, inputSize, (selfStart) - input);

                size_t bufferSize     = length - escapes.size();
                size_t bufferCapacity = bufferSize;
                
                std::unique_ptr<uint8_t, decltype(re::free)*> buffer((uint8_t*) re::alloc(bufferCapacity, "Defrag Buffer", __FILE__, __LINE__), re::free);
                index = 0;

                for(size_t i = 0; i < escapes.size(); ++i) {
                    memcpy(buffer.get() + index, start, escapes[i] - start);
                    index += escapes[i] - start;
                    start = escapes[i] + 1;
                }

                if(length > 0)
                    memcpy(buffer.get() + index, start, end - start);

                if(!iteratorVector.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(auto info : iteratorVector) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(info.iterator), info.iteratorLength);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localizeIterator(info.iterator, info.iteratorLength, buffer, bufferSize, bufferCapacity);
                        }
                    }
                }

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_COMPONENT_LENGTH > outputCapacity) {
                    uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
                    output.release();
                    output.reset(newOutput);
                }

                size_t oshStart = outputSize;

                LOG_DEBUG("Writing component template as BDP832 pair %zu -> %zu...", leftStart - input, end - input);
                outputSize += BDP::writeName(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_COMPONENT_MARKER, OSH_TEMPLATE_COMPONENT_LENGTH);

                size_t componentPathLength;
                std::unique_ptr<uint8_t, decltype(free)*> componentPath(
                    componentPathToAbsolute(wd, reinterpret_cast<const char*>(leftStart), leftLength, componentPathLength), re::free);

                size_t tempBufferSize = Global::BDP832->VALUE_LENGTH_BYTE_SIZE * 2 + componentPathLength + bufferSize;
                std::unique_ptr<uint8_t, decltype(re::free)*> tempBuffer((uint8_t*) re::malloc(tempBufferSize, "Compiler component template temp buffer", __FILE__, __LINE__), re::free);

                BDP::writeValue(Global::BDP832, tempBuffer.get(), componentPath.get(), componentPathLength);
                BDP::writeValue(Global::BDP832, tempBuffer.get() + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + componentPathLength, buffer.get(), bufferSize);

                while(outputSize + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + tempBufferSize + OSH_FORMAT > outputCapacity) {
                    uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
                    output.release();
                    output.reset(newOutput);
                }

                outputSize += BDP::writeValue(Global::BDP832, output.get() + outputSize, tempBuffer.get(), tempBufferSize);
                memset(output.get() + outputSize, 0, OSH_FORMAT);
                
                if(!isSelf) {
                    templateStack.push(TemplateStackInfo(TemplateType::COMPONENT, outputSize, templateStartIndex, oshStart));
                    outputSize += OSH_FORMAT;
                } else {
                    length = 0;

                    outputSize += OSH_FORMAT;

                    while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_COMPONENT_BODY_END_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length > outputCapacity) {
                        uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
                        output.release();
                        output.reset(newOutput);
                    }

                    outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_COMPONENT_BODY_END_MARKER, OSH_TEMPLATE_COMPONENT_BODY_END_LENGTH, start, length);
                }

                LOG_DEBUG("done\n");

                end = leftStart;
            }

            templateEndIndex += Options::getTemplateEndLength();
        } else if(membcmp(end, Options::getTemplateVoid(), Options::getTemplateVoidLength())) {
            LOG_DEBUG("Detected void template");

            end += Options::getTemplateVoidLength();

            while(isBlank(*end))
                ++end;

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            std::vector<const uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == Options::getTemplateEscape()) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index += 1 + mem_find(end + index + 1, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            }

            templateEndIndex = end + index - input;

            if(templateEndIndex >= inputSize)
                throwCompilationException(path, "Unexpected EOF", "did you forget to close the template?", input, inputSize, (end + index) - input - 1);
            if(index == 0)
                throwCompilationException(path, "Unexpected template end", "did you forget to write the body?", input, inputSize, end - input - 1);

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(isBlank(*end))
                --end;
            ++end;

            if(start != end) {
                length = end - start;

                // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.

                size_t bufferSize     = length - escapes.size();
                size_t bufferCapacity = bufferSize;
                
                std::unique_ptr<uint8_t, decltype(re::free)*> buffer((uint8_t*) re::alloc(bufferCapacity, "Defrag Buffer", __FILE__, __LINE__), re::free);
                index = 0;

                for(size_t i = 0; i < escapes.size(); ++i) {
                    memcpy(buffer.get() + index, start, escapes[i] - start);
                    index += escapes[i] - start;
                    start = escapes[i] + 1;
                }

                if(length > 0)
                    memcpy(buffer.get() + index, start, end - start);

                if(!iteratorVector.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(auto info : iteratorVector) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(info.iterator), info.iteratorLength);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localizeIterator(info.iterator, info.iteratorLength, buffer, bufferSize, bufferCapacity);
                        }
                    }
                }

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_VOID_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + bufferSize > outputCapacity) {
                    uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing void template as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_VOID_MARKER, OSH_TEMPLATE_VOID_LENGTH, buffer.get(), bufferSize);
                LOG_DEBUG("done\n");
            }

            end = start;
            templateEndIndex += Options::getTemplateEndLength();
        } else if(membcmp(end, Options::getTemplateBodyEnd(), Options::getTemplateBodyEndLength())) {
            LOG_DEBUG("Detected template body end");

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(templateEndIndex >= inputSize)
                throwCompilationException(path, "Unexpected EOF", "did you forget to close the template body?", input, inputSize, (end + index) - input - 1);

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(isBlank(*end))
                --end;

            if(start == end - Options::getTemplateBodyEndLength() + 1) {
                if(templateStack.empty())
                    throwCompilationException(path, "Unexpected template body end", "there is no template body to close; delete this", input, inputSize, start - input);

                ++end;
                length = 0;

                const uint8_t* bodyEndMarker;
                uint8_t bodyEndMarkerLength;

                // Choose the appropiate body end marker.
                switch(templateStack.top().type) {
                    case TemplateType::INVERTED_CONDITIONAL:
                    case TemplateType::ELSE:
                    case TemplateType::ELSE_CONDITIONAL:
                    case TemplateType::CONDITIONAL:
                        bodyEndMarker = OSH_TEMPLATE_CONDITIONAL_BODY_END_MARKER;
                        bodyEndMarkerLength = OSH_TEMPLATE_CONDITIONAL_BODY_END_LENGTH;
                        break;
                    case TemplateType::LOOP:
                        bodyEndMarker = OSH_TEMPLATE_LOOP_BODY_END_MARKER;
                        bodyEndMarkerLength = OSH_TEMPLATE_LOOP_BODY_END_LENGTH;
                        break;
                    case TemplateType::COMPONENT:
                        bodyEndMarker = OSH_TEMPLATE_COMPONENT_BODY_END_MARKER;
                        bodyEndMarkerLength = OSH_TEMPLATE_COMPONENT_BODY_END_LENGTH;
                        break;
                }

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + bodyEndMarkerLength + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length > outputCapacity) {
                    uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
                    output.release();
                    output.reset(newOutput);
                }

                // The output size before writing the conditional end.
                size_t backup = outputSize;

                LOG_DEBUG("Writing template body end as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, bodyEndMarker, bodyEndMarkerLength, start, length);
                // Here, 'length' is 0, so the value length will always be 0. The template body end has no value in OSH.

                // Write the body length properly.
                switch(templateStack.top().type) {
                    case TemplateType::CONDITIONAL:
                    case TemplateType::INVERTED_CONDITIONAL:
                        BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - 2 * OSH_FORMAT, backup - templateStack.top().bodyIndex, OSH_FORMAT);
                        BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - OSH_FORMAT, outputSize - backup, OSH_FORMAT); // Jumps at the end.
                        break;
                    case TemplateType::ELSE_CONDITIONAL:
                    case TemplateType::ELSE: {
                        size_t currentEndIndex = backup; // For the end index. The true end index will always be fixed.

                        bool searching = true;

                        while(!templateStack.empty() && searching) {
                            switch(templateStack.top().type) {
                                case TemplateType::CONDITIONAL:
                                case TemplateType::INVERTED_CONDITIONAL:
                                    BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - 2 * OSH_FORMAT, currentEndIndex - templateStack.top().bodyIndex, OSH_FORMAT); // Jumps at the start of the next else (conditiona) template.
                                    BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - OSH_FORMAT, outputSize - currentEndIndex, OSH_FORMAT); // Jumps at the end. The jump gap keeps increasing.

                                    searching = false;
                                    continue; // Don't run the pop() from below.
                                case TemplateType::ELSE:
                                    break;
                                case TemplateType::ELSE_CONDITIONAL:
                                    BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - 2 * OSH_FORMAT, currentEndIndex - templateStack.top().bodyIndex, OSH_FORMAT); // Jumps at the start of the next else (conditiona) template.
                                    BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - OSH_FORMAT, outputSize - currentEndIndex, OSH_FORMAT); // Jumps at the end. The jump gap keeps increasing.
                                    break;
                            }

                            currentEndIndex = templateStack.top().outputIndex;
                            templateStack.pop();
                        }

                        if(templateStack.empty())
                            throwCompilationException(path, "PANIC", "template body end found else (conditional) template, but no preceding conditional template on the stack (REPORT THIS TO THE DEVS)", input, inputSize, start + Options::getTemplateBodyEndLength() - input);

                        break;
                    }
                    case TemplateType::LOOP:
                        while(outputSize + OSH_FORMAT > outputCapacity) {
                            uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
                            output.release();
                            output.reset(newOutput);
                        }

                        BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - OSH_FORMAT, outputSize - templateStack.top().bodyIndex + OSH_FORMAT, OSH_FORMAT);
                        BDP::lengthToBytes(output.get() + outputSize, outputSize + OSH_FORMAT - templateStack.top().bodyIndex, OSH_FORMAT);

                        outputSize += OSH_FORMAT;
                        iteratorVector.pop_back();
                        break;
                    case TemplateType::COMPONENT:
                        BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex, backup - templateStack.top().bodyIndex - OSH_FORMAT, OSH_FORMAT);
                        break;
                }
                
                templateStack.pop();
                LOG_DEBUG("done\n");

                end = start;
            } else throwCompilationException(path, "Expected template body end", "template body end must only contain the marker", input, inputSize, start + Options::getTemplateBodyEndLength() - input);

            templateEndIndex += Options::getTemplateEndLength();
        } else { // Normal Template.
            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            std::vector<const uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == Options::getTemplateEscape()) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index += 1 + mem_find(end + index + 1, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            }

            templateEndIndex = (end + index) - input;

            if(templateEndIndex >= inputSize)
                throwCompilationException(path, "Unexpected EOF", "did you forget to close the template?", input, inputSize, (end + index) - input - 1);

            if(index != 0 || escapes.size() > 0) {
                LOG_DEBUG("Found template end at %zu", end + index - input);

                end = end + index - 1;
                while(isBlank(*end))
                    --end;
                ++end;

                length = end - start;

                // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.

                size_t bufferSize     = length - escapes.size();
                size_t bufferCapacity = bufferSize;
                
                std::unique_ptr<uint8_t, decltype(re::free)*> buffer((uint8_t*) re::alloc(bufferCapacity, "Defrag Buffer", __FILE__, __LINE__), re::free);
                index = 0;

                for(size_t i = 0; i < escapes.size(); ++i) {
                    memcpy(buffer.get() + index, start, escapes[i] - start);
                    index += escapes[i] - start;
                    start = escapes[i] + 1;
                }

                if(length > 0)
                    memcpy(buffer.get() + index, start, end - start);

                if(!iteratorVector.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(auto info : iteratorVector) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(info.iterator), info.iteratorLength);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localizeIterator(info.iterator, info.iteratorLength, buffer, bufferSize, bufferCapacity);
                        }
                    }
                }

                if(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + bufferSize > outputCapacity) {
                    uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_LENGTH, buffer.get(), bufferSize);
                LOG_DEBUG("done\n");
            } else {
                LOG_DEBUG("Detected empty template\n");
            }

            templateEndIndex += Options::getTemplateEndLength();
        }

        start = input + templateEndIndex;

        if(start >= limit)
            break;

        remainingLength = inputSize - (start - input);
        end = start + mem_find(start, remainingLength, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());
        length = end - start;
    }

    if(!templateStack.empty()) {
        std::string msgBuffer;
        msgBuffer.reserve(64);

        msgBuffer += "Expected end for";          

        switch(templateStack.top().type) {
            case TemplateType::CONDITIONAL:
                msgBuffer += " conditional ";
                break;
            case TemplateType::ELSE:
                msgBuffer += " else ";
                break;
            case TemplateType::ELSE_CONDITIONAL:
                msgBuffer += " else conditional ";
                break;
            case TemplateType::INVERTED_CONDITIONAL:
                msgBuffer += " inverted conditional ";
                break;
            case TemplateType::LOOP:
                msgBuffer += " loop ";
                break;
            case TemplateType::COMPONENT:
                msgBuffer += " component ";
                break;
        }

        msgBuffer += "template";

        throwCompilationException(path, msgBuffer.c_str(), "did you forget to close this?", input, inputSize, templateStack.top().inputIndex);
    }

    if(end > start) {
        LOG_DEBUG("--> Found plaintext at %zu\n", start - input);

        bool skip = false;

        if(Options::getIgnoreBlankPlaintext()) {
            skip = true;
            const uint8_t* i = start;

            while(skip && i != end) {
                if(*i != ' ' && *i != '\t' && *i != '\n' && *i != '\r')
                    skip = false;
                ++i;
            }
        }
        if(!skip) {
            while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_PLAINTEXT_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length > outputCapacity) {
                uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
                output.release();
                output.reset(newOutput);
            }

            LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - input, end - input);
            outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_LENGTH, start, length);
            LOG_DEBUG("done\n");
        } else LOG_DEBUG("Skipping blank plaintext");
    }

    // Bring the capacity to the actual size.
    if(outputSize != outputCapacity) {
        uint8_t* newOutput = (uint8_t*) re::realloc(output.get(), outputSize, __FILE__, __LINE__);
        output.release();
        output.reset(newOutput);
    }

    uint8_t* compiled = output.get();
    output.release();

    return BinaryData(compiled, outputSize);
}

static void compiler_error(const char* file, const char* message, const char* description, ConstBuffer& input, size_t errorIndex) {
    Chunk chunk(input.data, input.size, errorIndex, COMPILER_ERROR_CHUNK_SIZE);
    throw Eryn::CompilationException(file, message, description, chunk);
}