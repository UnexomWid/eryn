#include "compiler.hxx"

#include "../def/osh.dxx"
#include "../def/logging.dxx"

#include "../../lib/bdp.hxx"
#include "../../lib/mem_find.h"
#include "../../lib/mem_index.h"

#include "../global/cache.hxx"
#include "../global/global.hxx"
#include "../global/options.hxx"

#include "../except/compilation.hxx"

#include <stack>
#include <vector>
#include <memory>
#include <cstdio>
#include <cstdint>
#include <cstdlib>

#ifdef _MSC_VER
    #include "../../include/dirent.h"
#else
    #include <dirent.h>
#endif

using Global::Cache;
using Global::Options;

uint8_t* componentPathToAbsolute(const char* wd, const char* componentPath, size_t componentPathLength, size_t &absoluteLength);
size_t getDirectoryEndIndex(const char* path);
bool isBlank(uint8_t c);

void compile(const char* path) {
    LOG_DEBUG("===> Compiling file '%s'", path);

    if(!Cache::hasEntry(path))
        Cache::addEntry(path, compileFile(path));
    else {
        BinaryData compiled = compileFile(path);
        size_t index = Cache::getRawEntry(path);
        
        qfree((uint8_t*) Cache::getData(index).data);
        Cache::setData(index, compiled);
    }

    LOG_DEBUG("===> Done\n");
}

void compileDir(const char* path, std::vector<std::string> filters) {
    LOG_DEBUG("===> Compiling directory '%s'", path);

    FilterInfo info;

    for(size_t i = 0; i < filters.size(); ++i) {
        const char* start = filters[i].c_str();
        const char* end = start + filters[i].size() - 1;

        bool inverted = false;

        while((*start == ' ' || *start == '"') && start < end)
            ++start;
        while((*end == ' ' || *end == '"') && end > start)
            --end;

        if(start < end) {
            if(*start == '!' || *start == '^') {
                inverted = true;
                ++start;
            }

            if(start < end) {
                if(inverted) 
                    info.addExclusion(start, end - start + 1);
                else info.addFilter(start, end - start + 1);
            }
        }
    }

    compileDir(path, "", info);

    LOG_DEBUG("===> Done\n");
}

void compileDir(const char* path, const char* rel, const FilterInfo& info) {
    DIR* dir;
    struct dirent* entry;

    if((dir = opendir(path)) != nullptr) {
        size_t absoluteLength = strlen(path);
        size_t relLength = strlen(rel);

        char absolute[COMPILER_PATH_MAX_LENGTH + 1];

        strcpy(absolute, path);
        absolute[absoluteLength] = COMPILER_PATH_SEPARATOR;
        absolute[absoluteLength + 1] = '\0';

        char* absoluteEnd = absolute + absoluteLength + 1;

        while((entry = readdir(dir)) != nullptr) {
            size_t nameLength = strlen(entry->d_name);
            if(entry->d_type == DT_REG) {
                strcpy(absoluteEnd, entry->d_name);

                std::unique_ptr<char, decltype(qfree)*> relativePath(
                    reinterpret_cast<char*>(qmalloc(relLength + nameLength + 1)), qfree);

                if(relLength > 0)
                    strcpy(relativePath.get(), rel);
                strcpy(relativePath.get() + relLength, entry->d_name);

                if(info.isFileFiltered(relativePath.get())) {
                    try {
                        compile(absolute);
                    } catch(std::exception &e) {
                        LOG_ERROR("Error: %s", e.what());
                    }
                }
                else LOG_DEBUG("Ignoring: %s\n", relativePath.get());
            } else if(entry->d_type == DT_DIR) {
                if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                    continue;

                const size_t newRelLength = relLength + nameLength;
                std::unique_ptr<char, decltype(qfree)*> newRel(
                    reinterpret_cast<char*>(qmalloc(newRelLength + 2)), qfree);

                if(relLength > 0)
                    strcpy(newRel.get(), rel);
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

BinaryData compileFile(const char* path) {
    FILE* input = fopen(path, "rb");

    if(input == NULL)
        throw CompilationException(path, "IO error", "cannot open file");

    fseek(input, 0, SEEK_END);
    long fileLength = ftell(input);
    fseek(input, 0, SEEK_SET);

    LOG_DEBUG("File size is %ld bytes\n", fileLength);

    size_t inputSize = (size_t) fileLength;
    std::unique_ptr<uint8_t, decltype(qfree)*> inputBuffer(qmalloc(inputSize), qfree);

    fread(inputBuffer.get(), 1, inputSize, input);
    fclose(input);

    std::unique_ptr<char, decltype(qfree)*> wd(qstrndup(path, getDirectoryEndIndex(path)), qfree);

    return compileBytes(inputBuffer.get(), inputSize, wd.get(), path);
}

BinaryData compileBytes(uint8_t* input, size_t inputSize, const char* wd, const char* path) {
    size_t   outputSize = 0;
    size_t   outputCapacity = inputSize;
    std::unique_ptr<uint8_t, decltype(qfree)*> output(qalloc(outputCapacity), qfree);

    uint8_t* start  = input;
    uint8_t* end    = input + mem_find(input, inputSize, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());
    size_t   length = end - start;

    uint8_t* limit  = input + inputSize;

    size_t   remainingLength;
    size_t   index;
    size_t   templateStartIndex;
    size_t   templateEndIndex;

    enum class TemplateType {
        CONDITIONAL,
        LOOP,
        COMPONENT
    };

    struct TemplateStackInfo {
        TemplateType type;    // The template type.
        size_t bodyIndex;     // The index at which the template body starts (for writing the body size).  
        size_t templateIndex; // The index at which the template starts in the input (provides more information when an exception occurs).

        TemplateStackInfo(TemplateType typ, size_t start, size_t index) : type(typ), bodyIndex(start), templateIndex(index) { };
    };

    std::stack<TemplateStackInfo> templateStack;

    while(end < limit) {
        if(start != end) {
            LOG_DEBUG("--> Found plaintext at %zu", start - input);

            bool skip = false;
            if(Options::getIgnoreBlankPlaintext()) {
                skip = true;
                uint8_t* i = start;

                while(skip && i != end) {
                    if(!isBlank(*i))
                        skip = false;
                    ++i;
                }
            }

            if(!skip) {
                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_PLAINTEXT_MARKER_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_MARKER_LENGTH, start, length);
                LOG_DEBUG("done\n");
            } else LOG_DEBUG("Skipping blank plaintext");
        }
        templateStartIndex = end - input;

        LOG_DEBUG("--> Found template start at %zu", templateStartIndex);

        end += Options::getTemplateStartLength();

        while(isBlank(*end))
            ++end;

        if(membcmp(end, Options::getTemplateConditionalStart(), Options::getTemplateConditionalStartLength())) {
            LOG_DEBUG("Detected conditional template start");

            end += Options::getTemplateConditionalStartLength();

            while(isBlank(*end))
                ++end;

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            std::vector<uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == Options::getTemplateEscape()) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index += 1 + mem_find(end + index + 1, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            }

            templateEndIndex = end + index - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                throw CompilationException(path, "Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            if(index == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);           

                CompilationException exception(path, "Unexpected template end", "did you forget to write the condition?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(isBlank(*end))
                --end;
            ++end;

            if(start != end) {
                length = end - start;

                // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.
                
                std::unique_ptr<uint8_t, decltype(qfree)*> buffer(qmalloc(length - escapes.size()), qfree);
                index = 0;

                for(size_t i = 0; i < escapes.size(); ++i) {
                    memcpy(buffer.get() + index, start, escapes[i] - start);
                    index += escapes[i] - start;
                    start = escapes[i] + 1;
                }

                if(length > 0)
                    memcpy(buffer.get() + index, start, end - start);

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length - escapes.size() + OSH_FORMAT > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template conditional start as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_CONDITIONAL_START_MARKER, OSH_TEMPLATE_CONDITIONAL_START_MARKER_LENGTH, buffer.get(), length - escapes.size());
                memset(output.get() + outputSize, 0, OSH_FORMAT);

                outputSize += OSH_FORMAT;
                
                templateStack.push(TemplateStackInfo(TemplateType::CONDITIONAL, outputSize, templateStartIndex));
                LOG_DEBUG("done\n");
            }

            end = start;
        } else if(membcmp(end, Options::getTemplateConditionalEnd(), Options::getTemplateConditionalEndLength())) {
            LOG_DEBUG("Detected conditional template end");

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                throw CompilationException(path, "Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(isBlank(*end))
                --end;

            if(start == end - Options::getTemplateConditionalEndLength() + 1) {
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(free)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                    throw CompilationException(path, "Unexpected conditional end", "there is no conditional template to close; delete this", ln, col, chunk.get(), chunkIndex, chunkSize);
                } else if(templateStack.top().type != TemplateType::CONDITIONAL) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, templateStack.top().templateIndex, &ln, &col);

                    std::string msgBuffer;
                    msgBuffer.reserve(64);

                    msgBuffer += "close the ";          

                    switch(templateStack.top().type) {
                        case TemplateType::LOOP:
                            msgBuffer += "loop";
                            break;
                        case TemplateType::COMPONENT:
                            msgBuffer += "component";
                            break;
                    }

                    msgBuffer += " template at ";
                    msgBuffer += std::to_string(ln);
                    msgBuffer += ":";
                    msgBuffer += std::to_string(col);
                    msgBuffer += " first";

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(free)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                    throw CompilationException(path, "Unexpected conditional end", msgBuffer.c_str(), ln, col, chunk.get(), chunkIndex, chunkSize);
                }

                ++end;
                length = 0;

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template conditional end as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_CONDITIONAL_END_MARKER, OSH_TEMPLATE_CONDITIONAL_END_MARKER_LENGTH, start, length);

                if(BDP::isLittleEndian())
                    BDP::directLengthToBytes(output.get() + templateStack.top().bodyIndex - OSH_FORMAT, outputSize - templateStack.top().bodyIndex, OSH_FORMAT);
                else BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - OSH_FORMAT, outputSize - templateStack.top().bodyIndex, OSH_FORMAT);
                
                templateStack.pop();
                LOG_DEBUG("done\n");

                end = start;
            } else {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = start + Options::getTemplateConditionalEndLength() - input;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                throw CompilationException(path, "Expected template end", "conditional template end must only contain the marker", ln, col, chunk.get(), chunkIndex, chunkSize);
            }
        } else if(membcmp(end, Options::getTemplateLoopStart(), Options::getTemplateLoopStartLength())) {
            LOG_DEBUG("Detected loop template start");

            end += Options::getTemplateLoopStartLength();

            while(isBlank(*end))
                ++end;

            start = end;

            uint8_t* leftStart = start;
            uint8_t* leftEnd = start;
            size_t   sepIndex;

            remainingLength = inputSize - (end - input);
            sepIndex = mem_find(end, remainingLength, Options::getTemplateLoopSeparator(), Options::getTemplateLoopSeparatorLength(), Options::getTemplateLoopSeparatorLookup());
            index    = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            std::vector<uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == Options::getTemplateEscape()) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index += 1 + mem_find(end + index + 1, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            }

            templateEndIndex = end + index - input;

            if(sepIndex > index) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);           

                throw CompilationException(path, "Unexpected end of template", "did you forget to write the loop separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(sepIndex == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + sepIndex) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);         

                throw CompilationException(path, "Unexpected EOF", "did you forget to write the loop separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(sepIndex == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - input;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);           

                throw CompilationException(path, "Unexpected separator", "did you forget to provide the left argument before the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);           

                throw CompilationException(path, "Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found loop template separator at %zu", end + sepIndex - input);
            LOG_DEBUG("Found template end at %zu", end + index - input);

            leftEnd = end + sepIndex - 1;
            start = end + sepIndex + Options::getTemplateLoopSeparatorLength();

            while(*leftEnd == ' ' || *leftEnd == '\t')
                --leftEnd;
            ++leftEnd;

            end = end + index - 1;
            while(isBlank(*end))
                --end;
            ++end;

            if(end == start) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (leftStart + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);          

                throw CompilationException(path, "Unexpected end of template", "did you forget to provide the right argument after the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            while(*start == ' ' || *start == '\t')
                ++start;

            length = end - start;
            size_t leftLength = leftEnd - leftStart;

            // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.
                
            std::unique_ptr<uint8_t, decltype(qfree)*> buffer(qmalloc(length - escapes.size()), qfree);
            index = 0;

            for(size_t i = 0; i < escapes.size(); ++i) {
                memcpy(buffer.get() + index, start, escapes[i] - start);
                index += escapes[i] - start;
                start = escapes[i] + 1;
            }

            if(length > 0)
                memcpy(buffer.get() + index, start, end - start);

            while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_LOOP_START_MARKER_LENGTH > outputCapacity) {
                uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                output.release();
                output.reset(newOutput);
            }

            LOG_DEBUG("Writing template loop start as BDP832 pair %zu -> %zu...", leftStart - input, end - input);
            outputSize += BDP::writeName(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_LOOP_START_MARKER, OSH_TEMPLATE_LOOP_START_MARKER_LENGTH);

            size_t tempBufferSize = Global::BDP832->VALUE_LENGTH_BYTE_SIZE * 2 + leftLength + length - escapes.size();
            std::unique_ptr<uint8_t, decltype(qfree)*> tempBuffer(qmalloc(tempBufferSize), qfree);

            BDP::writeValue(Global::BDP832, tempBuffer.get(), leftStart, leftLength);
            BDP::writeValue(Global::BDP832, tempBuffer.get() + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + leftLength, buffer.get(), length - escapes.size());

            while(outputSize + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + tempBufferSize + OSH_FORMAT > outputCapacity) {
                uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                output.release();
                output.reset(newOutput);
            }

            outputSize += BDP::writeValue(Global::BDP832, output.get() + outputSize, tempBuffer.get(), tempBufferSize);
            memset(output.get() + outputSize, 0, OSH_FORMAT);

            outputSize += OSH_FORMAT;
                
            templateStack.push(TemplateStackInfo(TemplateType::LOOP, outputSize, templateStartIndex));

            LOG_DEBUG("done\n");

            end = leftStart;
        } else if(membcmp(end, Options::getTemplateLoopEnd(), Options::getTemplateLoopEndLength())) {
            LOG_DEBUG("Detected loop template end");

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);          

                throw CompilationException(path, "Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(isBlank(*end))
                --end;

            if(start == end - Options::getTemplateLoopEndLength() + 1) {
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(free)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);          

                    throw CompilationException(path, "Unexpected loop template end", "there is no loop template to close; delete this", ln, col, chunk.get(), chunkIndex, chunkSize);
                } else if(templateStack.top().type != TemplateType::LOOP) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, templateStack.top().templateIndex, &ln, &col);

                    std::string msgBuffer;
                    msgBuffer.reserve(64);

                    msgBuffer += "close the";          

                    switch(templateStack.top().type) {
                        case TemplateType::CONDITIONAL:
                            msgBuffer += " conditional ";
                            break;
                        case TemplateType::COMPONENT:
                            msgBuffer += " component ";
                            break;
                    }

                    msgBuffer += "template at ";
                    msgBuffer += std::to_string(ln);
                    msgBuffer += ":";
                    msgBuffer += std::to_string(col);
                    msgBuffer += " first";

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(free)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                    throw CompilationException(path, "Unexpected loop template end", msgBuffer.c_str(), ln, col, chunk.get(), chunkIndex, chunkSize);
                }

                ++end;
                length = 0;

                if(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_LOOP_END_MARKER_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + OSH_FORMAT + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template loop end as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_LOOP_END_MARKER, OSH_TEMPLATE_LOOP_END_MARKER_LENGTH, start, length);

                if(BDP::isLittleEndian()) {
                    BDP::directLengthToBytes(output.get() + templateStack.top().bodyIndex - OSH_FORMAT, outputSize - templateStack.top().bodyIndex + OSH_FORMAT, OSH_FORMAT);
                    BDP::directLengthToBytes(output.get() + outputSize, outputSize + OSH_FORMAT - templateStack.top().bodyIndex, OSH_FORMAT);
                }
                else {
                    BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex - OSH_FORMAT, outputSize - templateStack.top().bodyIndex + OSH_FORMAT, OSH_FORMAT);
                    BDP::lengthToBytes(output.get() + outputSize, outputSize + OSH_FORMAT - templateStack.top().bodyIndex, OSH_FORMAT);
                }

                outputSize += OSH_FORMAT;
                templateStack.pop();
                LOG_DEBUG("done\n");

                end = start;
            } else {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = start + Options::getTemplateLoopEndLength() - input;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                throw CompilationException(path, "Expected template end", "loop template end must only contain the marker", ln, col, chunk.get(), chunkIndex, chunkSize);
            }
        } else if(membcmp(end, Options::getTemplateComponent(), Options::getTemplateComponentLength())) {
            LOG_DEBUG("Detected component template");

            end += Options::getTemplateComponentLength();

            while((isBlank(*end)) && end < input + inputSize)
                ++end;

            start = end;

            uint8_t* leftStart = start;
            uint8_t* leftEnd = start;
            size_t   sepIndex;

            remainingLength = inputSize - (end - input);
            sepIndex = mem_find(end, remainingLength, Options::getTemplateComponentSeparator(), Options::getTemplateComponentSeparatorLength(), Options::getTemplateComponentSeparatorLookup());
            index    = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            std::vector<uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == Options::getTemplateEscape()) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index += 1 + mem_find(end + index + 1, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            }

            templateEndIndex = end + index - input;

            if(sepIndex == 0) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = end - input;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);            

                throw CompilationException(path, "Unexpected separator", "did you forget to provide the component name before the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);          

                throw CompilationException(path, "Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } 
            
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

                    if(end == start) {
                        size_t ln;
                        size_t col;
                        size_t chunkIndex;
                        size_t chunkSize;
                        size_t errorIndex = (leftStart + index) - input - 1;

                        mem_lncol(input, errorIndex, &ln, &col);
                        std::unique_ptr<uint8_t, decltype(free)*> chunk(
                            mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);         

                        throw CompilationException(path, "Unexpected end of template", "did you forget to provide the component context after the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
                    }

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
                uint8_t* selfStart;

                if(end - start + 1 >= Options::getTemplateComponentSelfLength()) {
                    selfStart = end - Options::getTemplateComponentSelfLength();

                    if(0 == mem_find(selfStart, inputSize - (selfStart - input), Options::getTemplateComponentSelf(), Options::getTemplateComponentSelfLength(), Options::getTemplateComponentSelfLookup())) {
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

                if(end <= start) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = (selfStart) - input;

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(free)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                    throw CompilationException(path, "Unexpected end of template", "did you forget to provide the component context after the separator?", ln, col, chunk.get(), chunkIndex, chunkSize);
                }

                // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.
                
                std::unique_ptr<uint8_t, decltype(qfree)*> buffer(qmalloc(length - escapes.size()), qfree);
                index = 0;

                for(size_t i = 0; i < escapes.size(); ++i) {
                    memcpy(buffer.get() + index, start, escapes[i] - start);
                    index += escapes[i] - start;
                    start = escapes[i] + 1;
                }

                if(length > 0)
                    memcpy(buffer.get() + index, start, end - start);

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_COMPONENT_MARKER_LENGTH > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template component as BDP832 pair %zu -> %zu...", leftStart - input, end - input);
                outputSize += BDP::writeName(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_COMPONENT_MARKER, OSH_TEMPLATE_COMPONENT_MARKER_LENGTH);

                size_t componentPathLength;
                std::unique_ptr<uint8_t, decltype(free)*> componentPath(
                    componentPathToAbsolute(wd, reinterpret_cast<const char*>(leftStart), leftLength, componentPathLength), qfree);

                size_t tempBufferSize = Global::BDP832->VALUE_LENGTH_BYTE_SIZE * 2 + componentPathLength + length - escapes.size();
                std::unique_ptr<uint8_t, decltype(qfree)*> tempBuffer(qmalloc(tempBufferSize), qfree);

                BDP::writeValue(Global::BDP832, tempBuffer.get(), componentPath.get(), componentPathLength);
                BDP::writeValue(Global::BDP832, tempBuffer.get() + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + componentPathLength, buffer.get(), length - escapes.size());

                while(outputSize + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + tempBufferSize + OSH_FORMAT > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                outputSize += BDP::writeValue(Global::BDP832, output.get() + outputSize, tempBuffer.get(), tempBufferSize);
                memset(output.get() + outputSize, 0, OSH_FORMAT);
                
                if(!isSelf)
                    templateStack.push(TemplateStackInfo(TemplateType::COMPONENT, outputSize, templateStartIndex));
                outputSize += OSH_FORMAT;

                LOG_DEBUG("done\n");

                end = leftStart;
            }
        } else if(membcmp(end, Options::getTemplateComponentEnd(), Options::getTemplateComponentEndLength())) {
            LOG_DEBUG("Detected component template end");

            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            templateEndIndex = end + index - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);          

                throw CompilationException(path, "Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            }

            LOG_DEBUG("Found template end at %zu", end + index - input);

            end = end + index - 1;
            while(isBlank(*end))
                --end;

            if(start == end - Options::getTemplateComponentEndLength() + 1) {
                if(templateStack.empty()) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(free)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                    throw CompilationException(path, "Unexpected component template end", "there is no component template to close; delete this", ln, col, chunk.get(), chunkIndex, chunkSize);
                } else if(templateStack.top().type != TemplateType::COMPONENT) {
                    size_t ln;
                    size_t col;
                    size_t chunkIndex;
                    size_t chunkSize;
                    size_t errorIndex = start - input;

                    mem_lncol(input, templateStack.top().templateIndex, &ln, &col);

                    std::string msgBuffer;
                    msgBuffer.reserve(64);

                    msgBuffer += "close the";          

                    switch(templateStack.top().type) {
                        case TemplateType::CONDITIONAL:
                            msgBuffer += " conditional ";
                            break;
                        case TemplateType::LOOP:
                            msgBuffer += " loop ";
                            break;
                    }

                    msgBuffer += "template at ";
                    msgBuffer += std::to_string(ln);
                    msgBuffer += ":";
                    msgBuffer += std::to_string(col);
                    msgBuffer += " first";

                    mem_lncol(input, errorIndex, &ln, &col);
                    std::unique_ptr<uint8_t, decltype(free)*> chunk(
                        mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                    throw CompilationException(path, "Unexpected component end", msgBuffer.c_str(), ln, col, chunk.get(), chunkIndex, chunkSize);
                }

                ++end;
                length = 0;

                while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_COMPONENT_END_MARKER_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                size_t backup = outputSize;

                LOG_DEBUG("Writing template component end as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_COMPONENT_END_MARKER, OSH_TEMPLATE_COMPONENT_END_MARKER_LENGTH, start, length);
                
                if(BDP::isLittleEndian())
                    BDP::directLengthToBytes(output.get() + templateStack.top().bodyIndex, backup - templateStack.top().bodyIndex - OSH_FORMAT, OSH_FORMAT);
                else BDP::lengthToBytes(output.get() + templateStack.top().bodyIndex, backup - templateStack.top().bodyIndex - OSH_FORMAT, OSH_FORMAT);
                
                templateStack.pop();
                LOG_DEBUG("done\n");

                end = start;
            } else {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = start + Options::getTemplateLoopEndLength() - input;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

                throw CompilationException(path, "Expected template end", "component template end must only contain the marker", ln, col, chunk.get(), chunkIndex, chunkSize);
            }
        } else { // Normal Template.
            start = end;
            remainingLength = inputSize - (end - input);
            index = mem_find(end, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());

            std::vector<uint8_t*> escapes;

            while(index < remainingLength && *(end + index - 1) == Options::getTemplateEscape()) {
                LOG_DEBUG("Detected template escape at %zu", end + index - 1 - input);

                escapes.push_back(end + index - 1);
                remainingLength -= index + 1;
                index += 1 + mem_find(end + index + 1, remainingLength, Options::getTemplateEnd(), Options::getTemplateEndLength(), Options::getTemplateEndLookup());
            }

            templateEndIndex = (end + index) - input;

            if(index == remainingLength) {
                size_t ln;
                size_t col;
                size_t chunkIndex;
                size_t chunkSize;
                size_t errorIndex = (end + index) - input - 1;

                mem_lncol(input, errorIndex, &ln, &col);
                std::unique_ptr<uint8_t, decltype(free)*> chunk(
                    mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);    

                throw CompilationException(path, "Unexpected EOF", "did you forget to close the template?", ln, col, chunk.get(), chunkIndex, chunkSize);
            } else if(index != 0 || escapes.size() > 0) {
                LOG_DEBUG("Found template end at %zu", end + index - input);

                end = end + index - 1;
                while(isBlank(*end))
                    --end;
                ++end;

                length = end - start;

                // Defragmentation: remove the escape characters by copying the fragments between them into a buffer.
                
                std::unique_ptr<uint8_t, decltype(qfree)*> buffer(qmalloc(length - escapes.size()), qfree);
                index = 0;

                for(size_t i = 0; i < escapes.size(); ++i) {
                    memcpy(buffer.get() + index, start, escapes[i] - start);
                    index += escapes[i] - start;
                    start = escapes[i] + 1;
                }

                if(length > 0)
                    memcpy(buffer.get() + index, start, end - start);

                if(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_TEMPLATE_MARKER_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length - escapes.size() > outputCapacity) {
                    uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                    output.release();
                    output.reset(newOutput);
                }

                LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - input, end - input);
                outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_MARKER_LENGTH, buffer.get(), length - escapes.size());
                LOG_DEBUG("done\n");
            } else {
                LOG_DEBUG("Detected empty template\n");
            }
        }

        start = input + templateEndIndex + Options::getTemplateEndLength();

        if(start >= limit)
            break;

        remainingLength = inputSize - (start - input);
        end = start + mem_find(start, remainingLength, Options::getTemplateStart(), Options::getTemplateStartLength(), Options::getTemplateStartLookup());
        length = end - start;
    }

    if(!templateStack.empty()) {
        size_t ln;
        size_t col;
        size_t chunkIndex;
        size_t chunkSize;
        size_t errorIndex = templateStack.top().templateIndex;

        mem_lncol(input, errorIndex, &ln, &col);
        std::unique_ptr<uint8_t, decltype(free)*> chunk(
            mem_lnchunk(input, errorIndex, inputSize, COMPILER_ERROR_CHUNK_SIZE, &chunkIndex, &chunkSize), free);

        std::string msgBuffer;
        msgBuffer.reserve(64);

        msgBuffer += "Expected end for";          

        switch(templateStack.top().type) {
            case TemplateType::CONDITIONAL:
                msgBuffer += " conditional ";
                break;
            case TemplateType::LOOP:
                msgBuffer += " loop ";
                break;
            case TemplateType::COMPONENT:
                msgBuffer += " component ";
                break;
        }

        msgBuffer += "template";

        throw CompilationException(path, msgBuffer.c_str(), "did you forget to close this?", ln, col, chunk.get(), chunkIndex, chunkSize);
    }

    if(end > start) {
        LOG_DEBUG("--> Found plaintext at %zu\n", start - input);

        bool skip = false;

        if(Options::getIgnoreBlankPlaintext()) {
            skip = true;
            uint8_t* i = start;

            while(skip && i != end) {
                if(*i != ' ' && *i != '\t' && *i != '\n' && *i != '\r')
                    skip = false;
                ++i;
            }
        }
        if(!skip) {
            while(outputSize + Global::BDP832->NAME_LENGTH_BYTE_SIZE + OSH_PLAINTEXT_MARKER_LENGTH + Global::BDP832->VALUE_LENGTH_BYTE_SIZE + length > outputCapacity) {
                uint8_t* newOutput = qexpand(output.get(), outputCapacity);
                output.release();
                output.reset(newOutput);
            }

            LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - input, end - input);
            outputSize += BDP::writePair(Global::BDP832, output.get() + outputSize, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_MARKER_LENGTH, start, length);
            LOG_DEBUG("done\n");
        } else LOG_DEBUG("Skipping blank plaintext");
    }

    // Bring the capacity to the actual size.
    if(outputSize != outputCapacity) {
        uint8_t* newOutput = qrealloc(output.get(), outputSize);
        output.release();
        output.reset(newOutput);
    }

    uint8_t* compiled = output.get();
    output.release();

    return BinaryData(compiled, outputSize);
}

uint8_t* componentPathToAbsolute(const char* wd, const char* componentPath, size_t componentPathLength, size_t &absoluteLength) {
    if(*componentPath == '/' || *componentPath == '\\') {
        uint8_t* absolute = qmalloc(componentPathLength);
        memcpy(absolute, componentPath, componentPathLength);

        absoluteLength = componentPathLength;
        return absolute;
    }

    std::string pathBuilder(wd);
    pathBuilder.reserve(pathBuilder.size() + 256);

    if(pathBuilder[pathBuilder.size() - 1] != '/' && pathBuilder[pathBuilder.size() - 1] != '\\')
        pathBuilder += '/';
    pathBuilder.append(componentPath, componentPathLength);

    const char* absolute = qstrdup(pathBuilder.c_str());

    absoluteLength = pathBuilder.size();
    return (uint8_t*) absolute;
}

size_t getDirectoryEndIndex(const char* path) {
    size_t length = strlen(path);
    size_t index = 0;
    size_t endIndex = 0;

    while(index < length) {
        if(*path == '/' || *path == '\\')
            endIndex = index;
        ++index;
        ++path;
    }

    return endIndex;
}

bool isBlank(uint8_t c) {
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}