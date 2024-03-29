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

#include "bridge/bridge.hxx"

#include "../../lib/str.hxx"
#include "../../lib/path.hxx"
#include "../../lib/remem.hxx"
#include "../../lib/buffer.hxx"
#include "../../lib/mem.hxx"
#include "../../lib/chunk.hxx"

#ifdef _MSC_VER
    #include "../../include/dirent.h"
#else
    #include <dirent.h>
#endif

static constexpr auto COMPILER_PATH_SEPARATOR   = '/';
static constexpr auto COMPILER_ERROR_CHUNK_SIZE = 40u;
static constexpr auto COMPILER_PATH_MAX_LENGTH  = 4096u;

static void localize_iterator(const ConstBuffer& iterator, Buffer& src);

enum class TemplateType {
    CONDITIONAL,
    ELSE,
    ELSE_CONDITIONAL,
    LOOP,
    COMPONENT
};

struct TemplateStackInfo {
    TemplateType type;
    size_t inputIndex;      // Where the template starts in the input (provides more information when an exception occurs).
    size_t outputIndex;     // Where the template starts in the output. Points at the start of the OSH data.
    size_t outputBodyIndex; // Where the template body starts in the output (for writing the body size). Points immediately after the OSH data (1 byte after). 

    TemplateStackInfo(TemplateType typ, size_t body, size_t input, size_t output) :
        type(typ), outputBodyIndex(body), inputIndex(input), outputIndex(output) { };
};

struct TemplateEndInfo {
    std::vector<const uint8_t*> escapes;
    size_t index;

    TemplateEndInfo(std::vector<const uint8_t*>&& escapes, size_t endIndex) : escapes(escapes), index(endIndex) { }
};

struct Compiler {
    Eryn::Options* opts;

    Eryn::BridgeCompileData bridge;

    ConstBuffer    input;
    Buffer         output;

    const char*    wd;
    const char*    path;

    const uint8_t* start;
    const uint8_t* current;

    std::stack<TemplateStackInfo> templates;
    std::vector<ConstBuffer>      iterators;

    const BDP::Header BDP832 = BDP::Header(8, 32);

    Compiler(Eryn::Options* opts, Eryn::BridgeCompileData bridge, ConstBuffer input, const char* wd, const char* path)
        : opts(opts), bridge(bridge), input(input), wd(wd), path(path), start(input.data), current(start) { }

    void rebase(size_t index);
    void rebase(const uint8_t* ptr);
    void advance(size_t amount);
    void seek(size_t index);
    void seek(const uint8_t* ptr);
    void skip_whitespace();
    void skip_whitespace_back();
    bool match_current(const std::string& pattern);

    TemplateEndInfo find_template_end(const uint8_t* from);
    TemplateEndInfo find_comment_template_end(const uint8_t* from);

    void write_escaped_content(Buffer& buffer, const uint8_t* start, const uint8_t* end, const std::vector<const uint8_t*>& escapes);
    void localize_all_iterators(Buffer& src);
    void error(const char* file, const char* message, const char* description, size_t errorIndex);

    void prepare_template_start(const char* name, size_t markerSize);

    void compile_plaintext();
    void compile_comment();
    void compile_conditional();
    void compile_else_conditional();
    void compile_else();
    void compile_loop();
    void compile_component();
    void compile_void();
    void compile_body_end();
    void compile_normal();

    void call_hook(Buffer& buffer, const char* origin);
};

void Compiler::rebase(size_t index) {
    start = input.data + index;

    if(current < start) {
        current = start;
    }
}

void Compiler::rebase(const uint8_t* ptr) {
    start = ptr;

    if(current < start) {
        current = start;
    }
}

void Compiler::advance(size_t amount) {
    current += amount;
}

void Compiler::seek(size_t index) {
    current = input.data + index;
}

void Compiler::seek(const uint8_t* ptr) {
    current = ptr;
}

void Compiler::skip_whitespace() {
    while(current < input.end() && str::is_blank(*current)) {
        ++current;
    }
}

void Compiler::skip_whitespace_back() {
    while(current >= input.data && str::is_blank(*current)) {
        --current;
    }
}

bool Compiler::match_current(const std::string& pattern) {
    return input.match(current - input.data, pattern);
}

void Compiler::prepare_template_start(const char* name, size_t markerSize) {
    LOG_DEBUG("Detected %s template", name)

    advance(markerSize);
    skip_whitespace(); // Skips whitespace such that, for example, [|?stuff|] is the same as [|? stuff|]
}

void Compiler::compile_plaintext() {
    if(start == current) {
        return;
    }

    LOG_DEBUG("--> Found plaintext at %zu", start - input.data);

    bool skip = false;
    if(opts->flags.ignoreBlankPlaintext) {
        skip = true;
        const uint8_t* i = start;

        while(skip && i != current) {
            if(!str::is_blank(*i)) {
                skip = false;
            }
            ++i;
        }
    }

    if(!skip) {
        if (!opts->compileHook.IsEmpty()) {
            Buffer buffer;
            buffer.write(start, current - start);

            call_hook(buffer, "plaintext");
        }

        LOG_DEBUG("Writing plaintext as BDP832 pair %zu -> %zu...", start - input.data, current - input.data);
        output.write_bdp_pair(BDP832, OSH_PLAINTEXT_MARKER, OSH_PLAINTEXT_LENGTH, start, current - start);
        LOG_DEBUG("done\n");
    } else {
        LOG_DEBUG("Skipping blank plaintext");
    }
}

void Compiler::compile_comment() {
    rebase(current);

    auto endInfo = std::move(find_comment_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template?", templateEndIndex - 1);
    }

    LOG_DEBUG("Found template end at %zu", templateEndIndex);

    if (!opts->compileHook.IsEmpty()) {
        Buffer buffer;
        buffer.write(start, templateEndIndex - (start - input.data));

        call_hook(buffer, "comment");
    }

    seek(templateEndIndex);
    advance(opts->templates.commentEnd.size());
}

void Compiler::compile_conditional() {
    rebase(current);

    auto endInfo = std::move(find_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template?", templateEndIndex - 1);
    }

    if(endInfo.index == 0) {
        error(path, "Unexpected template end", "did you forget to write the condition for this template?", start - input.data - 1);
    }

    LOG_DEBUG("Found template end at %zu", templateEndIndex);

    advance(endInfo.index - 1);
    skip_whitespace_back();
    ++current;

    // start != current
    Buffer buffer;

    write_escaped_content(buffer, start, current, endInfo.escapes);
    localize_all_iterators(buffer);

    if (!opts->compileHook.IsEmpty()) {
        call_hook(buffer, "conditional");
    }

    auto oshStart = output.size;

    LOG_DEBUG("Writing conditional template start as BDP832 pair %zu -> %zu...", start - input.data, current - input.data);
    output.write_bdp_pair(BDP832, OSH_TEMPLATE_CONDITIONAL_START_MARKER, OSH_TEMPLATE_CONDITIONAL_START_LENGTH, buffer.data, buffer.size);

    output.repeat(0, 2 * OSH_FORMAT); // One for the end index, one for the true end index.
    
    templates.push(TemplateStackInfo(TemplateType::CONDITIONAL, output.size, start - input.data, oshStart));
    LOG_DEBUG("done\n");

    seek(templateEndIndex);
    advance(opts->templates.end.size());
}

void Compiler::compile_else_conditional() {
    if(templates.empty() || (templates.top().type != TemplateType::CONDITIONAL && templates.top().type != TemplateType::ELSE_CONDITIONAL)) {
        error(path, "Unexpected else conditional template", "there is no preceding conditional template; delete this", start - input.data);
    }

    rebase(current);

    auto endInfo = std::move(find_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template?", templateEndIndex - 1);
    }

    if(endInfo.index == 0) {
        error(path, "Unexpected template end", "did you forget to write the condition for this template?", start - input.data - 1);
    }

    LOG_DEBUG("Found template end at %zu", templateEndIndex);

    advance(endInfo.index - 1);
    skip_whitespace_back();
    ++current;

    // start != current
    Buffer buffer;

    write_escaped_content(buffer, start, current, endInfo.escapes);
    localize_all_iterators(buffer);

    if (!opts->compileHook.IsEmpty()) {
        call_hook(buffer, "else_conditional");
    }

    auto oshStart = output.size;

    LOG_DEBUG("Writing else conditional template start as BDP832 pair %zu -> %zu...", start - input.data, current - input.data);
    output.write_bdp_pair(BDP832, OSH_TEMPLATE_ELSE_CONDITIONAL_START_MARKER, OSH_TEMPLATE_ELSE_CONDITIONAL_START_LENGTH, buffer.data, buffer.size);

    output.repeat(0, 2 * OSH_FORMAT); // One for the end index, one for the true end index.
    
    templates.push(TemplateStackInfo(TemplateType::ELSE_CONDITIONAL, output.size, start - input.data, oshStart));
    LOG_DEBUG("done\n");

    seek(templateEndIndex);
    advance(opts->templates.end.size());
}

void Compiler::compile_else() {
    rebase(current);

    auto endInfo = std::move(find_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template body?", (current + endInfo.index) - input.data - 1);
    }

    LOG_DEBUG("Found template end at %zu", templateEndIndex);

    advance(endInfo.index - 1);
    skip_whitespace_back();

    if(start != current + 1) {
        // opts.templates.end.size()
        error(path, "Expected template body end", "else template must only contain the marker", start + 1 - input.data);
    }

    if(templates.empty() || (templates.top().type != TemplateType::CONDITIONAL && templates.top().type != TemplateType::ELSE_CONDITIONAL)) {
        error(path, "Unexpected else template", "there is no preceding conditional template; delete this", start - input.data);
    }

    size_t oshStart = output.size;

    LOG_DEBUG("Writing else template start as BDP832 pair %zu -> %zu...", start - input.data, current - input.data);
    // No OSH value, and no end indices.
    output.write_bdp_pair(BDP832, OSH_TEMPLATE_ELSE_START_MARKER, OSH_TEMPLATE_ELSE_START_LENGTH, NULL, 0);
    
    templates.push(TemplateStackInfo(TemplateType::ELSE, output.size, start - input.data, oshStart));
    LOG_DEBUG("done\n");

    seek(templateEndIndex);
    advance(opts->templates.end.size());
}

void Compiler::compile_loop() {
    rebase(current);

    const uint8_t* leftStart = start;
    const uint8_t* leftEnd   = start;

    auto sepIndex = input.find_index(current - input.data, opts->templates.loopSeparator) - (current - input.data);
    auto endInfo  = std::move(find_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(sepIndex > endInfo.index) {
        error(path, "Unexpected end of template", "did you forget to write the loop separator?", templateEndIndex - 1);
    }
    if(current + sepIndex >= input.end()) {
        error(path, "Unexpected EOF", "did you forget to write the loop separator?", (current + sepIndex) - input.data - 1);
    }
    if(sepIndex == 0) {
        error(path, "Unexpected separator", "did you forget to provide the left argument before the separator?", current - input.data);
    }
    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template?", templateEndIndex - 1);
    }

    LOG_DEBUG("Found loop template separator at %zu", (current + sepIndex) - input.data);
    LOG_DEBUG("Found template end at %zu", templateEndIndex);

    leftEnd = current + sepIndex - 1;

    auto rightStart = current + sepIndex + opts->templates.loopSeparator.size();

    while(leftEnd >= input.data && str::is_blank(*leftEnd)) {
        --leftEnd;
    }
    ++leftEnd;

    advance(endInfo.index - 1);
    skip_whitespace_back();
    ++current;

    if(current == rightStart) {
        error(path, "Unexpected end of template", "did you forget to provide the right argument after the separator?", (leftStart + endInfo.index) - input.data - 1);
    }

    while(rightStart < current && str::is_blank(*rightStart)) {
        ++rightStart;
    }

    auto leftLength = leftEnd - leftStart;
    auto isReverse = false;

    // current - rightStart + 1 >= opts->templates.loopReverse.size()
    auto reverseStart = current - opts->templates.loopReverse.size();

    if(input.match(reverseStart - input.data, opts->templates.loopReverse)) {
        LOG_DEBUG("Detected reversed loop template");

        isReverse = true;
        current = reverseStart - 1;

        while(current > rightStart && str::is_blank(*current)) {
            --current;
        }
        ++current;
    }

    Buffer iterableBuffer;
    ConstBuffer finalIterableBuffer(leftStart, leftLength);

    if (!opts->compileHook.IsEmpty()) {
        iterableBuffer.write(leftStart, leftEnd - leftStart);

        call_hook(iterableBuffer, "loop_iterator");

        finalIterableBuffer.data = iterableBuffer.data;
        finalIterableBuffer.size = iterableBuffer.size;
    }

    Buffer buffer;

    write_escaped_content(buffer, rightStart, current, endInfo.escapes);
    localize_all_iterators(buffer);

    if (!opts->compileHook.IsEmpty()) {
        call_hook(buffer, "loop_iterable");
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

    LOG_DEBUG("Writing loop template start as BDP832 pair %zu -> %zu...", leftStart - input.data, current - input.data);
    output.write_bdp_name(BDP832, oshStartMarker, oshStartMarkerLength);

    Buffer tempBuffer;

    tempBuffer.write_bdp_value(BDP832, finalIterableBuffer.data, finalIterableBuffer.size);
    tempBuffer.write_bdp_value(BDP832, buffer.data, buffer.size);

    output.write_bdp_value(BDP832, tempBuffer.data, tempBuffer.size);
    output.repeat(0, OSH_FORMAT);
        
    templates.push(TemplateStackInfo(TemplateType::LOOP, output.size, start - input.data, oshStart));
    iterators.push_back(ConstBuffer(leftStart, leftLength));

    LOG_DEBUG("done\n");

    seek(templateEndIndex);
    advance(opts->templates.end.size());
}

void Compiler::compile_component() {
    rebase(current);

    const uint8_t* leftStart = start;
    const uint8_t* leftEnd = start;

    auto sepIndex = input.find_index(current - input.data, opts->templates.componentSeparator) - (current - input.data);
    auto endInfo  = std::move(find_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(sepIndex == 0) {
        error(path, "Unexpected separator", "did you forget to provide the component name before the separator?", current - input.data);
    }
    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template?", templateEndIndex - 1);
    }
    if(endInfo.index == 0) {
        error(path, "Unexpected end of template", "did you forget to write the component name?", templateEndIndex);
    }

    LOG_DEBUG("Found template component at %zu", current + sepIndex - input.data);
    LOG_DEBUG("Found template end at %zu", templateEndIndex);

    auto rightStart = leftStart;
    auto rightEnd = leftEnd;

    if(sepIndex < endInfo.index) {
        leftEnd = current + sepIndex - 1;

        while(leftEnd >= input.data && str::is_blank(*leftEnd)) {
            --leftEnd;
        }
        ++leftEnd;

        rightStart = current + sepIndex + opts->templates.componentSeparator.size();

        advance(endInfo.index - 1);
        skip_whitespace_back();
        ++current;

        if(current == rightStart) {
            error(path, "Unexpected end of template", "did you forget to provide the component context after the separator?", templateEndIndex - 1);
        }

        while(str::is_blank(*rightStart)) {
            ++rightStart;
        }

        rightEnd = current;
    } else {
        leftEnd = current + endInfo.index - 1;

        advance(endInfo.index - 1);
        skip_whitespace_back();
        ++current;

        while(str::is_blank(*leftEnd)) {
            --leftEnd;
        }
        ++leftEnd;

        rightEnd = leftEnd;

        seek(rightEnd);
    }

    bool isSelf = false;
    const uint8_t* selfStart;

    // current - rightStart + 1 >= opts.templates.componentSelf.size()
    selfStart = current - opts->templates.componentSelf.size();

    if(input.match(selfStart - input.data, opts->templates.componentSelf)) {
        LOG_DEBUG("Detected self-closing component template");

        isSelf = true;
        current = selfStart - 1;

        while(current > rightStart && str::is_blank(*current)) {
            --current;
        }
        ++current;

        rightEnd = current;

        if(rightEnd < leftEnd) {
            leftEnd = current;
        }
    }

    if(rightEnd <= rightStart) {
        error(path, "Unexpected end of template", "did you forget to provide the component context after the separator?", (selfStart) - input.data);
    }

    if(rightStart == leftStart && endInfo.escapes.size() > 0) {
        error(path, "Unexpected escape character(s) before end", "escape characters can only exist after the separator, which was not found; delete all escape characters", (selfStart) - input.data);
    }

    Buffer pathBuffer;
    ConstBuffer finalPathBuffer(leftStart, leftEnd - leftStart);

    if (!opts->compileHook.IsEmpty()) {
        pathBuffer.write(leftStart, leftEnd - leftStart);

        call_hook(pathBuffer, "component_path");

        finalPathBuffer.data = pathBuffer.data;
        finalPathBuffer.size = pathBuffer.size;
    }
    
    Buffer buffer;

    // If the component template has context (i.e. context start is not the same as the path start).
    if(rightStart != leftStart) {
        write_escaped_content(buffer, rightStart, rightEnd, endInfo.escapes);
        localize_all_iterators(buffer);

        if (!opts->compileHook.IsEmpty()) {
            call_hook(buffer, "component_context");
        }
    }

    size_t oshStart = output.size;

    LOG_DEBUG("Writing component template as BDP832 pair %zu -> %zu...", leftStart - input.data, rightEnd - input.data);

    output.write_bdp_name(BDP832, OSH_TEMPLATE_COMPONENT_MARKER, OSH_TEMPLATE_COMPONENT_LENGTH);

    std::string absolutePath = path::append_or_absolute(wd, reinterpret_cast<const char*>(finalPathBuffer.data), finalPathBuffer.size);

    Buffer tempBuffer;
    tempBuffer.write_bdp_value(BDP832, reinterpret_cast<const uint8_t*>(absolutePath.c_str()), absolutePath.size());
    tempBuffer.write_bdp_value(BDP832, buffer.data, buffer.size);

    output.write_bdp_value(BDP832, tempBuffer.data, tempBuffer.size);
    output.repeat(0, OSH_FORMAT);
    
    if(!isSelf) {
        templates.push(TemplateStackInfo(TemplateType::COMPONENT, output.size - OSH_FORMAT, leftStart - input.data, oshStart));
    } else {
        output.write_bdp_pair(BDP832, OSH_TEMPLATE_COMPONENT_BODY_END_MARKER, OSH_TEMPLATE_COMPONENT_BODY_END_LENGTH, leftStart, 0);
    }

    LOG_DEBUG("done\n");

    seek(templateEndIndex);
    advance(opts->templates.end.size());
}

void Compiler::compile_void() {
    rebase(current);

    auto endInfo = std::move(find_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template?", templateEndIndex - 1);
    }
    if(endInfo.index == 0) {
        error(path, "Unexpected template end", "did you forget to write the template content?", current - input.data - 1);
    }

    LOG_DEBUG("Found template end at %zu", templateEndIndex);

    advance(endInfo.index - 1);
    skip_whitespace_back();
    ++current;

    // start != current
    Buffer buffer;

    write_escaped_content(buffer, start, current, endInfo.escapes);
    localize_all_iterators(buffer);

    if (!opts->compileHook.IsEmpty()) {
        call_hook(buffer, "void");
    }

    LOG_DEBUG("Writing void template as BDP832 pair %zu -> %zu...", start - input.data, current - input.data);
    output.write_bdp_pair(BDP832, OSH_TEMPLATE_VOID_MARKER, OSH_TEMPLATE_VOID_LENGTH, buffer.data, buffer.size);
    LOG_DEBUG("done\n");

    seek(templateEndIndex);
    advance(opts->templates.end.size());
}

void Compiler::compile_body_end() {
    rebase(current);

    auto endInfo = std::move(find_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template body?", templateEndIndex - 1);
    }

    LOG_DEBUG("Found template end at %zu", templateEndIndex);

    if(endInfo.index > 0) {
        error(path, "Expected template body end", "template body end must only contain the marker", current - input.data);
    }

    if(templates.empty()) {
        error(path, "Unexpected template body end", "there is no template body to close; delete this", start - input.data);
    }

    const uint8_t* bodyEndMarker;
    uint8_t bodyEndMarkerLength;

    // Choose the appropiate body end marker.
    switch(templates.top().type) {
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

    LOG_DEBUG("Writing template body end as BDP832 pair %zu -> %zu...", start - input.data, current - input.data);
    output.write_bdp_pair(BDP832, bodyEndMarker, bodyEndMarkerLength, start, 0);
    // Here, the value length will always be 0. The template body end has no value in OSH.
    // TODO: remove this inefficiency in the future

    // Write the body length properly.
    switch(templates.top().type) {
        case TemplateType::CONDITIONAL:
            output.write_length(templates.top().outputBodyIndex - 2 * OSH_FORMAT, backup - templates.top().outputBodyIndex, OSH_FORMAT);
            output.write_length(templates.top().outputBodyIndex - OSH_FORMAT, output.size - backup, OSH_FORMAT); // Jumps at the end.
            break;
        case TemplateType::ELSE_CONDITIONAL:
        case TemplateType::ELSE: {
            size_t currentEndIndex = backup; // For the end index. The true end index will always be fixed.

            bool searching = true;

            while(!templates.empty() && searching) {
                switch(templates.top().type) {
                    case TemplateType::CONDITIONAL:
                        // Jumps at the start of the next else (conditional) template.
                        output.write_length(templates.top().outputBodyIndex - 2 * OSH_FORMAT, currentEndIndex - templates.top().outputBodyIndex, OSH_FORMAT);
                        // Jumps at the end. The jump gap keeps increasing.
                        output.write_length(templates.top().outputBodyIndex - OSH_FORMAT, output.size - currentEndIndex, OSH_FORMAT);

                        searching = false;
                        continue; // Don't run the pop() from below.
                    case TemplateType::ELSE:
                        break;
                    case TemplateType::ELSE_CONDITIONAL:
                        // Jumps at the start of the next else (conditiona) template.
                        output.write_length(templates.top().outputBodyIndex - 2 * OSH_FORMAT, currentEndIndex - templates.top().outputBodyIndex, OSH_FORMAT);
                        // Jumps at the end. The jump gap keeps increasing.
                        output.write_length(templates.top().outputBodyIndex - OSH_FORMAT, output.size - currentEndIndex, OSH_FORMAT); // Jumps at the end. The jump gap keeps increasing.
                        break;
                }

                currentEndIndex = templates.top().outputIndex;
                templates.pop();
            }

            if(templates.empty()) {
                error(path, "PANIC", "template body end found else (conditional) template, but no preceding conditional template on the stack (REPORT THIS TO THE DEVS)", start - input.data);
            }

            break;
        }
        case TemplateType::LOOP:
            output.write_length(templates.top().outputBodyIndex - OSH_FORMAT, output.size - templates.top().outputBodyIndex + OSH_FORMAT, OSH_FORMAT);
            output.write_length(output.size + OSH_FORMAT - templates.top().outputBodyIndex, OSH_FORMAT);

            iterators.pop_back();
            break;
        case TemplateType::COMPONENT:
            output.write_length(templates.top().outputBodyIndex, backup - templates.top().outputBodyIndex - OSH_FORMAT, OSH_FORMAT);
            break;
    }
    
    templates.pop();
    LOG_DEBUG("done\n");

    seek(templateEndIndex);
    advance(opts->templates.end.size());
}

void Compiler::compile_normal() {
    rebase(current);

    auto endInfo = std::move(find_template_end(current));
    size_t templateEndIndex = (current + endInfo.index) - input.data;

    if(templateEndIndex >= input.size) {
        error(path, "Unexpected EOF", "did you forget to close the template?", templateEndIndex - 1);
    }

    if(endInfo.index != 0 || endInfo.escapes.size() > 0) {
        LOG_DEBUG("Found template end at %zu", templateEndIndex);

        advance(endInfo.index - 1);
        skip_whitespace_back();
        ++current;

        Buffer buffer;

        write_escaped_content(buffer, start, current, endInfo.escapes);
        localize_all_iterators(buffer);

        if (!opts->compileHook.IsEmpty()) {
            call_hook(buffer, "template");
        }

        LOG_DEBUG("Writing template as BDP832 pair %zu -> %zu...", start - input.data, current - input.data);
        output.write_bdp_pair(BDP832, OSH_TEMPLATE_MARKER, OSH_TEMPLATE_LENGTH, buffer.data, buffer.size);
        LOG_DEBUG("done\n");
    } else {
        LOG_DEBUG("Template is empty\n");
    }

    seek(templateEndIndex);
    advance(opts->templates.end.size());
}

void Compiler::call_hook(Buffer& buffer, const char* origin) {
    LOG_DEBUG("Running hook");
    if (!Eryn::Bridge::call_hook(bridge, opts->compileHook, buffer, origin)) {
        error(path, "Hook returned invalid value", "the compile hook should return Buffer, String, Null or Undefined", start - input.data);
    }
}

void Eryn::Engine::compile(BridgeCompileData bridge, const char* path) {
    LOG_DEBUG("===> Compiling file '%s'", path);

    cache.add(path, std::move(compile_file(bridge, path)));

    LOG_DEBUG("===> Done\n");
}

void Eryn::Engine::compile_string(BridgeCompileData bridge, const char* alias, const char* str) {
    LOG_DEBUG("===> Compiling string '%s'", alias);

    ConstBuffer input(str, strlen(str));
    cache.add(alias, compile_bytes(bridge, input, "", alias));

    LOG_DEBUG("===> Done\n");
}

void Eryn::Engine::compile_dir(BridgeCompileData bridge, const char* path, std::vector<string> filters) {
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
                    info.add_exclusion(start, end - start + 1);
                } else {
                    info.add_filter(start, end - start + 1);
                }
            }
        }
    }

    compile_dir(bridge, path, "", info);

    LOG_DEBUG("===> Done\n");
}

// 'rel' is relative to the working directory, and is used for filtering
// 'path' is the full path and is used to read the directory
void Eryn::Engine::compile_dir(BridgeCompileData bridge, const char* path, const char* rel, const FilterInfo& info) {
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

                if(info.is_file_filtered(relativePath.get())) {
                    try {
                        compile(bridge, absolute);

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

                if(info.is_dir_filtered(newRel.get())) {
                    LOG_DEBUG("Scanning: %s\n", newRel.get());

                    strcpy(absoluteEnd, entry->d_name);
                    compile_dir(bridge, absolute, newRel.get(), info);
                } else LOG_DEBUG("Ignoring: %s\n", newRel.get());
            }
        }
        closedir(dir);
    }
}

ConstBuffer Eryn::Engine::compile_file(BridgeCompileData bridge, const char* path) {
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

    return compile_bytes(bridge, inputBuffer, wd.c_str(), path);
}

// 'wd' is the working directory, which is necessary to find components
// 'path' is either the full path of the source file, or the alias of the source string
ConstBuffer Eryn::Engine::compile_bytes(BridgeCompileData bridge, ConstBuffer& input, const char* wd, const char* path) {
    Compiler compiler(&opts, bridge, input, wd, path);

    compiler.rebase((size_t) 0);
    compiler.seek(input.find(opts.templates.start));

    const uint8_t* limit = input.end();

    while(compiler.current < limit) {
        compiler.compile_plaintext();
        compiler.rebase(compiler.current);

        LOG_DEBUG("--> Found template start at %zu", compiler.start - input.data);

        compiler.advance(opts.templates.start.size());
        compiler.skip_whitespace(); // Skips whitespace such that, for example, both [|? |] and [| ? |] work.

        if(compiler.match_current(compiler.opts->templates.commentStart)) {
            compiler.prepare_template_start("comment", opts.templates.commentStart.size());
            compiler.compile_comment();
        } else if(compiler.match_current(opts.templates.conditionalStart)) {
            compiler.prepare_template_start("conditional", opts.templates.conditionalStart.size());
            compiler.compile_conditional();
        } else if(compiler.match_current(opts.templates.elseConditionalStart)) {
            compiler.prepare_template_start("else conditional", opts.templates.elseConditionalStart.size());
            compiler.compile_else_conditional();
        } else if(compiler.match_current(opts.templates.elseStart)) {
            compiler.prepare_template_start("else", opts.templates.elseStart.size());
            compiler.compile_else();
        } else if(compiler.match_current(opts.templates.loopStart)) {
            compiler.prepare_template_start("loop", opts.templates.loopStart.size());
            compiler.compile_loop();
        } else if(compiler.match_current(opts.templates.componentStart)) {
            compiler.prepare_template_start("component", opts.templates.componentStart.size());
            compiler.compile_component();
        } else if(compiler.match_current(opts.templates.voidStart)) {
            compiler.prepare_template_start("void", opts.templates.voidStart.size());
            compiler.compile_void();
        } else if(compiler.match_current(opts.templates.bodyEnd)) {
            compiler.prepare_template_start("body end", opts.templates.bodyEnd.size());
            compiler.compile_body_end();
        } else { // Normal Template.
            compiler.prepare_template_start("normal", 0);
            compiler.compile_normal();
        }

        compiler.rebase(compiler.current);
        compiler.seek(input.find(compiler.current - input.data, opts.templates.start));
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

        compiler.error(path, msgBuffer.c_str(), "did you forget to close this?", compiler.templates.top().inputIndex);
    }

    // If the file ends with plaintext, don't forget to write it.
    compiler.compile_plaintext();

    return compiler.output.finalize();
}

void Compiler::localize_all_iterators(Buffer& src) {
    if(iterators.empty()) {
        return;
    }
    
    LOG_DEBUG("Localizing iterators");

    // If 2 or more iterators share the same name, don't replace twice.
    std::unordered_set<std::string> iteratorSet;

    for(const auto& iterator : iterators) {
        std::string iteratorString = std::string(reinterpret_cast<const char*>(iterator.data), iterator.size);

        if(iteratorSet.find(iteratorString) == iteratorSet.end()) {
            iteratorSet.insert(iteratorString);
            localize_iterator(iterator, src);
        }
    }
}

void Compiler::error(const char* file, const char* message, const char* description, size_t errorIndex) {
    Chunk chunk(input.data, input.size, errorIndex, COMPILER_ERROR_CHUNK_SIZE);
    throw Eryn::CompilationException(file, message, description, chunk);
}

TemplateEndInfo Compiler::find_template_end(const uint8_t* from) {
    size_t index = input.find_index(from - input.data, opts->templates.end) - (from - input.data);

    std::vector<const uint8_t*> escapes;

    while((from + index) > input.data && (from + index) < input.end() && *(from + index - 1) == opts->templates.escape) {
        LOG_DEBUG("Detected template escape at %zu", from + index - 1 - input.data);

        escapes.push_back(from + index - 1);
        index = input.find_index((from + index + 1) - input.data, opts->templates.end) - (from - input.data);
    }

    return { std::move(escapes), index };
}

TemplateEndInfo Compiler::find_comment_template_end(const uint8_t* from) {
    size_t index = input.find_index(from - input.data, opts->templates.commentEnd) - (from - input.data);

    std::vector<const uint8_t*> escapes;

    while((from + index) > input.data && (from + index) < input.end() && *(from + index - 1) == opts->templates.escape) {
        LOG_DEBUG("Detected template escape at %zu", from + index - 1 - input.data);

        escapes.push_back(from + index - 1);
        index = input.find_index((from + index + 1) - input.data, opts->templates.commentEnd) - (from - input.data);
    }

    return { std::move(escapes), index };
}

void Compiler::write_escaped_content(Buffer& buffer, const uint8_t* start, const uint8_t* end, const std::vector<const uint8_t*>& escapes) {
    for(auto& escape : escapes) {
        buffer.write(start, escape - start);
        start = escape + 1;
    }

    buffer.write(start, end - start);
}

static void localize_iterator(const ConstBuffer& iterator, Buffer& src) {
    size_t index = 0;
    size_t matchIndex = 0;

    uint8_t quoteCount         = 0;
    uint8_t quoteTemplateCount = 0; // Template count, for template literals such as `text ${template}`.
    uint8_t quoteType          = 0;

    while(index < src.size) {
        uint8_t ch = src.data[index];

        switch(ch) {
            case '\'':
            case '\"':
            case  '`':
                if(index > 0 && src.data[index - 1] == '\\') {
                    ++index;
                    matchIndex = index; // Quotes. The iterator won't be matched, so reset the index.

                    continue;
                }

                if(quoteCount == 0) {
                    ++quoteCount;
                    quoteType = ch;
                } else if(quoteType == ch) {
                    --quoteCount;
                }

                ++index;
                matchIndex = index;

                continue;

            case '$': // Template literals.
            case '}':
                if(ch == '$') { // Doing the check again so both characters can branch, and also fall to default if needed.
                    if(index < src.size - 1 && src.data[index + 1] == '{') {
                        if(quoteCount > 0 && quoteType == '`') {
                            --quoteCount;
                            ++quoteTemplateCount;

                            index += 2;
                            matchIndex = index;

                            continue;
                        }
                    }
                } else {
                    if(quoteTemplateCount > 0) {
                        --quoteTemplateCount;
                        ++quoteCount;
                        quoteType = '`';

                        ++index;
                        matchIndex = index;

                        continue;
                    }
                }
            
            default: { // Regular characters fall here.
                if(index > 0 && matchIndex == index) {
                    if(src.data[index - 1] == '.'
                    || src.data[index - 1] == '\\'
                    || str::valid_in_token(src.data[index - 1])) {

                        ++index;
                        matchIndex = index;

                        continue;
                    }
                }

                if(ch == iterator.data[index - matchIndex] && (quoteCount == 0 || quoteCount < quoteTemplateCount)) {
                    if(index - matchIndex + 1 == iterator.size) {
                        if(index < src.size - 1
                        && (str::valid_in_token(src.data[index + 1])
                        ||  src.data[index + 1] == ':')) { // For object properties, such as {item: item}.

                            ++index;
                            matchIndex = index;

                            continue;
                        }

                        // Copy the prefix.
                        src.move_right(matchIndex, OSH_TEMPLATE_LOCAL_PREFIX_LENGTH);
                        src.write_at(matchIndex, OSH_TEMPLATE_LOCAL_PREFIX, OSH_TEMPLATE_LOCAL_PREFIX_LENGTH);

                        matchIndex += OSH_TEMPLATE_LOCAL_PREFIX_LENGTH + iterator.size;

                        // Copy the suffix.
                        src.move_right(matchIndex, OSH_TEMPLATE_LOCAL_SUFFIX_LENGTH);
                        src.write_at(matchIndex, OSH_TEMPLATE_LOCAL_SUFFIX, OSH_TEMPLATE_LOCAL_SUFFIX_LENGTH);
                        
                        index = matchIndex + OSH_TEMPLATE_LOCAL_SUFFIX_LENGTH;
                        matchIndex = index;

                        continue;
                    } else {
                        ++index;
                    }
                } else {
                    ++index;
                    matchIndex = index;
                }
            }
        }
    }
}