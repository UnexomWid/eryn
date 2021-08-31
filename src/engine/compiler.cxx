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

static uint8_t*   component_path_to_absolute(const char* wd, const char* componentPath, size_t componentPathLength, size_t &absoluteLength);
static void       localize_iterator(const uint8_t* iterator, size_t iteratorLength, Buffer& src);
static void       compiler_error(const char* file, const char* message, const char* description, ConstBuffer& input, size_t errorIndex);

struct TemplateEndInfo {
    std::vector<const uint8_t*> escapes;
    size_t index;

    TemplateEndInfo(std::vector<const uint8_t*>&& escapes, size_t endIndex) : escapes(escapes), index(endIndex) { }
};

static TemplateEndInfo find_template_end(ConstBuffer& input, const uint8_t* start, const std::string& templateEnd, char escapeChar);
static void write_escaped_content(ConstBuffer& input, Buffer& output, const uint8_t* start, const uint8_t* end, const std::vector<const uint8_t*>& escapes);

void Eryn::Engine::compile(const char* path) {
    LOG_DEBUG("===> Compiling file '%s'", path);

    cache.add(path, std::move(compile_file(path)));

    LOG_DEBUG("===> Done\n");
}

void Eryn::Engine::compile_string(const char* alias, const char* str) {
    LOG_DEBUG("===> Compiling string '%s'", alias);

    ConstBuffer input(str, strlen(str));
    cache.add(alias, compile_bytes(input, opts.workingDir.c_str(), alias));

    LOG_DEBUG("===> Done\n");
}

void Eryn::Engine::compile_dir(const char* path, std::vector<string> filters) {
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

    compile_dir(path, "", info);

    LOG_DEBUG("===> Done\n");
}

// 'rel' is relative to the working directory, and is used for filtering
// 'path' is the full path and is used to read the directory
void Eryn::Engine::compile_dir(const char* path, const char* rel, const FilterInfo& info) {
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
                    compile_dir(absolute, newRel.get(), info);
                } else LOG_DEBUG("Ignoring: %s\n", newRel.get());
            }
        }
        closedir(dir);
    }
}

ConstBuffer Eryn::Engine::compile_file(const char* path) {
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

    return compile_bytes(inputBuffer, wd.c_str(), path);
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
ConstBuffer Eryn::Engine::compile_bytes(ConstBuffer& input, const char* wd, const char* path) {
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
            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + index) - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

            end = start;
            templateEndIndex += opts.templates.commentEnd.size();
        } else if(input.match(end - input.data, opts.templates.conditionalStart)) {
            LOG_DEBUG("Detected conditional template start");

            end += opts.templates.conditionalStart.size();

            while(str::is_blank(*end)) {
                ++end;
            }

            start = end;
            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + endInfo.index) - input.data - 1);
            }

            if(endInfo.index == 0) {
                compiler_error(path, "Unexpected template end", "did you forget to write the condition?", input, end - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

            end = end + endInfo.index - 1;
            while(str::is_blank(*end)) {
                --end;
            }
            ++end;

            if(start != end) {
                length = end - start;
                index = 0;

                Buffer buffer;

                write_escaped_content(input, buffer, start, end, endInfo.escapes);

                if(!compiler.iterators.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(const auto& iterator : compiler.iterators) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localize_iterator(iterator.data, iterator.size, buffer);
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
            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + endInfo.index) - input.data - 1);
            }

            if(endInfo.index == 0) {
                compiler_error(path, "Unexpected template end", "did you forget to write the condition?", input, end - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

            end = end + endInfo.index - 1;
            while(str::is_blank(*end)) {
                --end;
            }
            ++end;

            if(start != end) {
                length = end - start;
                index = 0;

                Buffer buffer;

                write_escaped_content(input, buffer, start, end, endInfo.escapes);

                if(!compiler.iterators.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(const auto& iterator : compiler.iterators) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localize_iterator(iterator.data, iterator.size, buffer);
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
            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template body?", input, (end + endInfo.index) - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

            end = end + endInfo.index - 1;
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

            // TODO: add bounds check from components here and everywhere else.
            while(str::is_blank(*end)) {
                ++end;
            }

            start = end;

            const uint8_t* leftStart = start;
            const uint8_t* leftEnd = start;
            size_t sepIndex;

            remainingLength = input.size - (end - input.data);
            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;
            
            sepIndex = input.find_index(end - input.data, opts.templates.loopSeparator) - (end - input.data);

            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(sepIndex > endInfo.index) {
                compiler_error(path, "Unexpected end of template", "did you forget to write the loop separator?", input, (end + endInfo.index) - input.data - 1);
            }
            if(sepIndex == remainingLength) {
                compiler_error(path, "Unexpected EOF", "did you forget to write the loop separator?", input, (end + sepIndex) - input.data - 1);
            }
            if(sepIndex == 0) {
                compiler_error(path, "Unexpected separator", "did you forget to provide the left argument before the separator?", input, end - input.data);
            }
            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + endInfo.index) - input.data - 1);
            }

            LOG_DEBUG("Found loop template separator at %zu", end + sepIndex - input);
            LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

            leftEnd = end + sepIndex - 1;
            start = end + sepIndex + opts.templates.loopSeparator.size();

            while(*leftEnd == ' ' || *leftEnd == '\t')
                --leftEnd;
            ++leftEnd;

            end = end + endInfo.index - 1;
            while(str::is_blank(*end)) {
                --end;
            }
            ++end;

            if(end == start) {
                compiler_error(path, "Unexpected end of template", "did you forget to provide the right argument after the separator?", input, (leftStart + endInfo.index) - input.data - 1);
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

            index = 0;

            Buffer buffer;

            write_escaped_content(input, buffer, start, end, endInfo.escapes);

            if(!compiler.iterators.empty()) {
                LOG_DEBUG("Localizing iterators");

                // If 2 or more iterators share the same name, don't replace twice.
                std::unordered_set<std::string> iteratorSet;

                for(const auto& iterator : compiler.iterators) {
                    std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                    if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                        iteratorSet.insert(iteratorString);
                        localize_iterator(iterator.data, iterator.size, buffer);
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
        } else if(input.match(end - input.data, opts.templates.componentStart)) {
            LOG_DEBUG("Detected component template");

            end += opts.templates.componentStart.size();

            while((str::is_blank(*end)) && end < input.end())
                ++end;

            start = end;

            const uint8_t* leftStart = start;
            const uint8_t* leftEnd = start;
            size_t sepIndex;

            remainingLength = input.size - (end - input.data);
            sepIndex = input.find_index(end - input.data, opts.templates.componentSeparator) - (end - input.data);

            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(sepIndex == 0) {
                compiler_error(path, "Unexpected separator", "did you forget to provide the component name before the separator?", input, end - input.data);
            }
            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + endInfo.index) - input.data - 1);
            }
            
            if(endInfo.index == 0) {
                LOG_DEBUG("Detected empty component template");
            } else {
                LOG_DEBUG("Found template component at %zu", end + sepIndex - input);
                LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

                if(sepIndex < endInfo.index) {
                    leftEnd = end + sepIndex - 1;

                    while(*leftEnd == ' ' || *leftEnd == '\t')
                        --leftEnd;
                    ++leftEnd;

                    start = end + sepIndex + opts.templates.componentSeparator.size();
                    end = end + endInfo.index - 1;
                    while(str::is_blank(*end)) {
                        --end;
                    }
                    ++end;

                    if(end == start) {
                        compiler_error(path, "Unexpected end of template", "did you forget to provide the component context after the separator?", input, (leftStart + endInfo.index) - input.data - 1);
                    }

                    while(str::is_blank(*start)) {
                        ++start;
                    }
                    length = end - start;
                } else {
                    leftEnd = end + endInfo.index - 1;

                    while(str::is_blank(*leftEnd)) {
                        --leftEnd;
                    }
                    ++leftEnd;

                    start = leftStart;
                    end = leftEnd;
                    length = 0;
                }

                size_t leftLength = leftEnd - leftStart;

                bool isSelf = false;
                const uint8_t* selfStart;

                if(end - start + 1 >= opts.templates.componentSelf.size()) {
                    selfStart = end - opts.templates.componentSelf.size();

                    if(input.match(selfStart - input.data, opts.templates.componentSelf)) {
                        LOG_DEBUG("Detected self-closing component template");

                        isSelf = true;
                        end = selfStart - 1;

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

                if(end <= start) {
                    compiler_error(path, "Unexpected end of template", "did you forget to provide the component context after the separator?", input, (selfStart) - input.data);
                }

                if(length == 0 && endInfo.escapes.size() > 0) {
                    compiler_error(path, "Unexpected escape character(s) before end", "escape characters can only exist after the separator, which was not found; delete all escape characters", input, (selfStart) - input.data);
                }

                index = 0;
                
                Buffer buffer;

                write_escaped_content(input, buffer, start, end, endInfo.escapes);

                if(!compiler.iterators.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(const auto& iterator : compiler.iterators) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localize_iterator(iterator.data, iterator.size, buffer);
                        }
                    }
                }

                size_t oshStart = output.size;

                LOG_DEBUG("Writing component template as BDP832 pair %zu -> %zu...", leftStart - input, end - input);
                output.write_bdp_name(BDP832, OSH_TEMPLATE_COMPONENT_MARKER, OSH_TEMPLATE_COMPONENT_LENGTH);

                size_t componentPathLength;
                std::unique_ptr<uint8_t, decltype(free)*> componentPath(
                    component_path_to_absolute(wd, reinterpret_cast<const char*>(leftStart), leftLength, componentPathLength), re::free);

                Buffer tempBuffer;
                tempBuffer.write_bdp_value(BDP832, componentPath.get(), componentPathLength);
                tempBuffer.write_bdp_value(BDP832, buffer.data, buffer.size);

                output.write_bdp_value(BDP832, tempBuffer.data, tempBuffer.size);

                for(auto i = 0; i < OSH_FORMAT; ++i) {
                    output.write(0);
                }
                
                if(!isSelf) {
                    compiler.templates.push(TemplateStackInfo(TemplateType::COMPONENT, output.size - OSH_FORMAT, templateStartIndex, oshStart));
                } else {
                    length = 0;
                    output.write_bdp_pair(BDP832, OSH_TEMPLATE_COMPONENT_BODY_END_MARKER, OSH_TEMPLATE_COMPONENT_BODY_END_LENGTH, start, length);
                }

                LOG_DEBUG("done\n");

                end = leftStart;
            }

            templateEndIndex += opts.templates.end.size();
        } else if(input.match(end - input.data, opts.templates.voidStart)) {
            LOG_DEBUG("Detected void template");

            end += opts.templates.voidStart.size();

            while(str::is_blank(*end)) {
                ++end;
            }

            start = end;
            remainingLength = input.size - (end - input.data);
            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + endInfo.index) - input.data - 1);
            }
            if(endInfo.index == 0) {
                compiler_error(path, "Unexpected template end", "did you forget to write the body?", input, end - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

            end = end + endInfo.index - 1;
            while(str::is_blank(*end)) {
                --end;
            }
            ++end;

            if(start != end) {
                length = end - start;
                index = 0;
                
                Buffer buffer;

                write_escaped_content(input, buffer, start, end, endInfo.escapes);

                if(!compiler.iterators.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(const auto& iterator : compiler.iterators) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localize_iterator(iterator.data, iterator.size, buffer);
                        }
                    }
                }

                LOG_DEBUG("Writing void template as BDP832 pair %zu -> %zu...", start - input, end - input);
                output.write_bdp_pair(BDP832, OSH_TEMPLATE_VOID_MARKER, OSH_TEMPLATE_VOID_LENGTH, buffer.data, buffer.size);
                LOG_DEBUG("done\n");
            }

            end = start;
            templateEndIndex += opts.templates.end.size();
        } else if(input.match(end - input.data, opts.templates.bodyEnd)) {
            LOG_DEBUG("Detected template body end");

            start = end;
            remainingLength = input.size - (end - input.data);
            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template body?", input, (end + endInfo.index) - input.data - 1);
            }

            LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

            end = end + endInfo.index - 1;
            while(str::is_blank(*end)) {
                --end;
            }

            if(start == end - opts.templates.bodyEnd.size() + 1) {
                if(compiler.templates.empty()) {
                    compiler_error(path, "Unexpected template body end", "there is no template body to close; delete this", input, start - input.data);
                }

                ++end;
                length = 0;

                const uint8_t* bodyEndMarker;
                uint8_t bodyEndMarkerLength;

                // Choose the appropiate body end marker.
                switch(compiler.templates.top().type) {
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

                // The output size before writing the conditional end.
                size_t backup = output.size;

                LOG_DEBUG("Writing template body end as BDP832 pair %zu -> %zu...", start - input, end - input);
                output.write_bdp_pair(BDP832, bodyEndMarker, bodyEndMarkerLength, start, length);
                // Here, 'length' is 0, so the value length will always be 0. The template body end has no value in OSH.

                // Write the body length properly.
                switch(compiler.templates.top().type) {
                    case TemplateType::CONDITIONAL:
                        output.write_length(compiler.templates.top().outputBodyIndex - 2 * OSH_FORMAT, backup - compiler.templates.top().outputBodyIndex, OSH_FORMAT);
                        output.write_length(compiler.templates.top().outputBodyIndex - OSH_FORMAT, output.size - backup, OSH_FORMAT); // Jumps at the end.
                        break;
                    case TemplateType::ELSE_CONDITIONAL:
                    case TemplateType::ELSE: {
                        size_t currentEndIndex = backup; // For the end index. The true end index will always be fixed.

                        bool searching = true;

                        while(!compiler.templates.empty() && searching) {
                            switch(compiler.templates.top().type) {
                                case TemplateType::CONDITIONAL:
                                    // Jumps at the start of the next else (conditional) template.
                                    output.write_length(compiler.templates.top().outputBodyIndex - 2 * OSH_FORMAT, currentEndIndex - compiler.templates.top().outputBodyIndex, OSH_FORMAT);
                                    // Jumps at the end. The jump gap keeps increasing.
                                    output.write_length(compiler.templates.top().outputBodyIndex - OSH_FORMAT, output.size - currentEndIndex, OSH_FORMAT);

                                    searching = false;
                                    continue; // Don't run the pop() from below.
                                case TemplateType::ELSE:
                                    break;
                                case TemplateType::ELSE_CONDITIONAL:
                                    // Jumps at the start of the next else (conditiona) template.
                                    output.write_length(compiler.templates.top().outputBodyIndex - 2 * OSH_FORMAT, currentEndIndex - compiler.templates.top().outputBodyIndex, OSH_FORMAT);
                                    // Jumps at the end. The jump gap keeps increasing.
                                    output.write_length(compiler.templates.top().outputBodyIndex - OSH_FORMAT, output.size - currentEndIndex, OSH_FORMAT); // Jumps at the end. The jump gap keeps increasing.
                                    break;
                            }

                            currentEndIndex = compiler.templates.top().outputIndex;
                            compiler.templates.pop();
                        }

                        if(compiler.templates.empty()) {
                            compiler_error(path, "PANIC", "template body end found else (conditional) template, but no preceding conditional template on the stack (REPORT THIS TO THE DEVS)", input, start + opts.templates.bodyEnd.size() - input.data);
                        }

                        break;
                    }
                    case TemplateType::LOOP:
                        output.write_length(compiler.templates.top().outputBodyIndex - OSH_FORMAT, output.size - compiler.templates.top().outputBodyIndex + OSH_FORMAT, OSH_FORMAT);
                        output.write_length(output.size + OSH_FORMAT - compiler.templates.top().outputBodyIndex, OSH_FORMAT);

                        compiler.iterators.pop_back();
                        break;
                    case TemplateType::COMPONENT:
                        output.write_length(compiler.templates.top().outputBodyIndex, backup - compiler.templates.top().outputBodyIndex - OSH_FORMAT, OSH_FORMAT);
                        break;
                }
                
                compiler.templates.pop();
                LOG_DEBUG("done\n");

                end = start;
            } else {
                compiler_error(path, "Expected template body end", "template body end must only contain the marker", input, start + opts.templates.bodyEnd.size() - input.data);
            }

            templateEndIndex += opts.templates.end.size();
        } else { // Normal Template.
            start = end;
            remainingLength = input.size - (end - input.data);
            TemplateEndInfo endInfo = std::move(find_template_end(input, end, opts.templates.end, opts.templates.escape));
            templateEndIndex = (end + endInfo.index) - input.data;

            if(templateEndIndex >= input.size) {
                compiler_error(path, "Unexpected EOF", "did you forget to close the template?", input, (end + endInfo.index) - input.data - 1);
            }

            if(endInfo.index != 0 || endInfo.escapes.size() > 0) {
                LOG_DEBUG("Found template end at %zu", end + endInfo.index - input);

                end = end + endInfo.index - 1;
                while(str::is_blank(*end)) {
                    --end;
                }
                ++end;

                length = end - start;
                index = 0;

                Buffer buffer;

                write_escaped_content(input, buffer, start, end, endInfo.escapes);

                if(!compiler.iterators.empty()) {
                    LOG_DEBUG("Localizing iterators");

                    // If 2 or more iterators share the same name, don't replace twice.
                    std::unordered_set<std::string> iteratorSet;

                    for(const auto& iterator : compiler.iterators) {
                        std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

                        if(iteratorSet.end() == iteratorSet.find(iteratorString)) {
                            iteratorSet.insert(iteratorString);
                            localize_iterator(iterator.data, iterator.size, buffer);
                        }
                    }
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - input, end - input);
                output.write_bdp_pair(BDP832, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_LENGTH, buffer.data, buffer.size);
                LOG_DEBUG("done\n");
            } else {
                LOG_DEBUG("Detected empty template\n");
            }

            templateEndIndex += opts.templates.end.size();
        }

        start = input.data + templateEndIndex;

        if(start >= limit)
            break;

        remainingLength = input.size - (start - input.data);
        end = input.find(start - input.data, opts.templates.start);
        length = end - start;
    }

    if(!compiler.templates.empty()) {
        std::string msgBuffer;

        msgBuffer += "Expected end for";          

        switch(compiler.templates.top().type) {
            case TemplateType::CONDITIONAL:
                msgBuffer += " conditional ";
                break;
            case TemplateType::ELSE:
                msgBuffer += " else ";
                break;
            case TemplateType::ELSE_CONDITIONAL:
                msgBuffer += " else conditional ";
                break;
            case TemplateType::LOOP:
                msgBuffer += " loop ";
                break;
            case TemplateType::COMPONENT:
                msgBuffer += " component ";
                break;
        }

        msgBuffer += "template";

        compiler_error(path, msgBuffer.c_str(), "did you forget to close this?", input, compiler.templates.top().inputIndex);
    }

    if(end > start) {
        LOG_DEBUG("--> Found plaintext at %zu\n", start - input);

        bool skip = false;

        if(opts.flags.ignoreBlankPlaintext) {
            skip = true;
            const uint8_t* i = start;

            while(skip && i != end) {
                if(*i != ' ' && *i != '\t' && *i != '\n' && *i != '\r')
                    skip = false;
                ++i;
            }
        }
        if(!skip) {
            LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - input, end - input);
            output.write_bdp_pair(BDP832, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_LENGTH, start, length);
            LOG_DEBUG("done\n");
        } else {
            LOG_DEBUG("Skipping blank plaintext");
        }
    }

    return output.finalize();
}

static void compiler_error(const char* file, const char* message, const char* description, ConstBuffer& input, size_t errorIndex) {
    Chunk chunk(input.data, input.size, errorIndex, COMPILER_ERROR_CHUNK_SIZE);
    throw Eryn::CompilationException(file, message, description, chunk);
}

static TemplateEndInfo find_template_end(ConstBuffer& input, const uint8_t* start, const std::string& templateEnd, char escapeChar) {
    size_t index = input.find_index(start - input.data, templateEnd) - (start - input.data);

    std::vector<const uint8_t*> escapes;

    while((start + index - 1) < input.end() + 1 && *(start + index - 1) == escapeChar) {
        LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

        escapes.push_back(start + index - 1);
        index = 1 + input.find_index((start + index + 1) - input.data, templateEnd) - (start - input.data);
    }

    return { std::move(escapes), index };
}

static void write_escaped_content(ConstBuffer& input, Buffer& output, const uint8_t* start, const uint8_t* end, const std::vector<const uint8_t*>& escapes) {
    for(const auto& escape : escapes) {
        output.write(start, escape - start);
        start = escape + 1;
    }

    output.write(start, end - start);
}