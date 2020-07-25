#include "options.hxx"

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "../common/str.hxx"

#include "../../lib/remem.hxx"
#include "../../lib/mem_find.h"
#include "../../lib/buffer.hxx"

bool Global::Options::bypassCache            = false;
bool Global::Options::throwOnEmptyContent    = false;
bool Global::Options::throwOnMissingEntry    = false;
bool Global::Options::throwOnCompileDirError = false;
bool Global::Options::ignoreBlankPlaintext   = false;
bool Global::Options::logRenderTime          = false;
bool Global::Options::cloneIterators         = false;

char*   Global::Options::workingDirectory    = nullptr;

uint8_t  Global::Options::templateEscape     = 0;

uint8_t* Global::Options::templateStart                          = nullptr;
uint8_t  Global::Options::templateStartLength                    = 0;
uint8_t* Global::Options::templateStartLookup                    = nullptr;

uint8_t* Global::Options::templateEnd                            = nullptr;
uint8_t  Global::Options::templateEndLength                      = 0;
uint8_t* Global::Options::templateEndLookup                      = nullptr;

uint8_t* Global::Options::templateBodyEnd                        = nullptr;
uint8_t  Global::Options::templateBodyEndLength                  = 0;

uint8_t* Global::Options::templateVoid                           = nullptr;
uint8_t  Global::Options::templateVoidLength                     = 0;

uint8_t* Global::Options::templateComment                        = nullptr;
uint8_t  Global::Options::templateCommentLength                  = 0;

uint8_t* Global::Options::templateCommentEnd                     = nullptr;
uint8_t  Global::Options::templateCommentEndLength               = 0;
uint8_t* Global::Options::templateCommentEndLookup               = nullptr;

uint8_t* Global::Options::templateConditionalStart               = nullptr;
uint8_t  Global::Options::templateConditionalStartLength         = 0;

uint8_t* Global::Options::templateInvertedConditionalStart       = nullptr;
uint8_t  Global::Options::templateInvertedConditionalStartLength = 0;

uint8_t* Global::Options::templateLoopStart                      = nullptr;
uint8_t  Global::Options::templateLoopStartLength                = 0;

uint8_t* Global::Options::templateLoopSeparator                  = nullptr;
uint8_t  Global::Options::templateLoopSeparatorLength            = 0;
uint8_t* Global::Options::templateLoopSeparatorLookup            = nullptr;

uint8_t* Global::Options::templateLoopReverse                    = nullptr;
uint8_t  Global::Options::templateLoopReverseLength              = 0;

uint8_t* Global::Options::templateComponent                      = nullptr;
uint8_t  Global::Options::templateComponentLength                = 0;

uint8_t* Global::Options::templateComponentSeparator             = nullptr;
uint8_t  Global::Options::templateComponentSeparatorLength       = 0;
uint8_t* Global::Options::templateComponentSeparatorLookup       = nullptr;

uint8_t* Global::Options::templateComponentSelf                  = nullptr;
uint8_t  Global::Options::templateComponentSelfLength            = 0;

void Global::Options::setBypassCache(bool value) {
    Global::Options::bypassCache = value;
}

void Global::Options::setThrowOnEmptyContent(bool value) {
    Global::Options::throwOnEmptyContent = value;
}

void Global::Options::setThrowOnMissingEntry(bool value) {
    Global::Options::throwOnMissingEntry = value;
}

void Global::Options::setThrowOnCompileDirError(bool value) {
    Global::Options::throwOnCompileDirError = value;
}

void Global::Options::setIgnoreBlankPlaintext(bool value) {
    Global::Options::ignoreBlankPlaintext = value;
}

void Global::Options::setLogRenderTime(bool value) {
    Global::Options::logRenderTime = value;
}

void Global::Options::setCloneIterators(bool value) {
    Global::Options::cloneIterators = value;
}

void Global::Options::setWorkingDirectory(const char* value) {
    if(Global::Options::workingDirectory != nullptr)
        re::free(Global::Options::workingDirectory);

    Global::Options::workingDirectory = strDup(value);
}

void Global::Options::setTemplateEscape(char value) {
    Global::Options::templateEscape = (uint8_t) value;
}

void Global::Options::setTemplateStart(const char* value) {
    if(Global::Options::templateStart != nullptr)
        re::free(Global::Options::templateStart);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateStart = (uint8_t*) re::malloc(length, "Template start marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateStart, value, length);

    if(Global::Options::templateStartLookup != nullptr)
        re::free(Global::Options::templateStartLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateStartLookup = (uint8_t*) re::malloc(length, "Template start lookup", __FILE__, __LINE__);
        build_kmp_lookup(Global::Options::templateStartLookup, Global::Options::templateStart, length);
    } else {
        Global::Options::templateStartLookup = (uint8_t*) re::malloc(256u, "Template start lookup", __FILE__, __LINE__);
        build_horspool_lookup(Global::Options::templateStartLookup, Global::Options::templateStart, length);
    }

    Global::Options::templateStartLength = length;
}

void Global::Options::setTemplateEnd(const char* value) {
    if(Global::Options::templateEnd != nullptr)
        re::free(Global::Options::templateEnd);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateEnd = (uint8_t*) re::malloc(length, "Template end marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateEnd, value, length);

    if(Global::Options::templateEndLookup != nullptr)
        re::free(Global::Options::templateEndLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateEndLookup = (uint8_t*) re::malloc(length, "Template end lookup", __FILE__, __LINE__);
        build_kmp_lookup(Global::Options::templateEndLookup, Global::Options::templateEnd, length);
    } else {
        Global::Options::templateEndLookup = (uint8_t*) re::malloc(256u, "Template end lookup", __FILE__, __LINE__);
        build_horspool_lookup(Global::Options::templateEndLookup, Global::Options::templateEnd, length);
    }

    Global::Options::templateEndLength = length;
}

void Global::Options::setTemplateBodyEnd(const char* value) {
    if(Global::Options::templateBodyEnd != nullptr)
        re::free(Global::Options::templateBodyEnd);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateBodyEnd = (uint8_t*) re::malloc(length, "Template body end marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateBodyEnd, value, length);

    Global::Options::templateBodyEndLength = length;
}

void Global::Options::setTemplateVoid(const char* value) {
    if(Global::Options::templateVoid != nullptr)
        re::free(Global::Options::templateVoid);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateVoid = (uint8_t*) re::malloc(length, "Template body end marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateVoid, value, length);

    Global::Options::templateVoidLength = length;
}

void Global::Options::setTemplateComment(const char* value) {
    if(Global::Options::templateComment != nullptr)
        re::free(Global::Options::templateComment);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateComment = (uint8_t*) re::malloc(length, "Template comment marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateComment, value, length);

    Global::Options::templateCommentLength = length;
}

void Global::Options::setTemplateCommentEnd(const char* value) {
    if(Global::Options::templateCommentEnd != nullptr)
        re::free(Global::Options::templateCommentEnd);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateCommentEnd = (uint8_t*) re::malloc(length, "Template comment end marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateCommentEnd, value, length);

    if(Global::Options::templateCommentEndLookup != nullptr)
        re::free(Global::Options::templateCommentEndLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateCommentEndLookup = (uint8_t*) re::malloc(length, "Template comment end lookup", __FILE__, __LINE__);
        build_kmp_lookup(Global::Options::templateCommentEndLookup, Global::Options::templateCommentEnd, length);
    } else {
        Global::Options::templateCommentEndLookup = (uint8_t*) re::malloc(256u, "Template comment end lookup", __FILE__, __LINE__);
        build_horspool_lookup(Global::Options::templateCommentEndLookup, Global::Options::templateCommentEnd, length);
    }

    Global::Options::templateCommentEndLength = length;
}

void Global::Options::setTemplateConditionalStart(const char* value) {
    if(Global::Options::templateConditionalStart != nullptr)
        re::free(Global::Options::templateConditionalStart);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateConditionalStart = (uint8_t*) re::malloc(length, "Template conditional start marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateConditionalStart, value, length);

    Global::Options::templateConditionalStartLength = length;
}

void Global::Options::setTemplateInvertedConditionalStart(const char* value) {
    if(Global::Options::templateInvertedConditionalStart != nullptr)
        re::free(Global::Options::templateInvertedConditionalStart);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateInvertedConditionalStart = (uint8_t*) re::malloc(length, "Template inverted conditional start marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateInvertedConditionalStart, value, length);

    Global::Options::templateInvertedConditionalStartLength = length;
}

void Global::Options::setTemplateLoopStart(const char* value) {
    if(Global::Options::templateLoopStart != nullptr)
        re::free(Global::Options::templateLoopStart);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateLoopStart = (uint8_t*) re::malloc(length, "Template loop start marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateLoopStart, value, length);

    Global::Options::templateLoopStartLength = length;
}

void Global::Options::setTemplateLoopSeparator(const char* value) {
    if(Global::Options::templateLoopSeparator != nullptr)
        re::free(Global::Options::templateLoopSeparator);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateLoopSeparator = (uint8_t*) re::malloc(length, "Template loop separator marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateLoopSeparator, value, length);

    if(Global::Options::templateLoopSeparatorLookup != nullptr)
        re::free(Global::Options::templateLoopSeparatorLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateLoopSeparatorLookup = (uint8_t*) re::malloc(length, "Template loop separator lookup", __FILE__, __LINE__);
        build_kmp_lookup(Global::Options::templateLoopSeparatorLookup, Global::Options::templateLoopSeparator, length);
    } else {
        templateLoopSeparatorLookup = (uint8_t*) re::malloc(256u, "Template loop separator lookup", __FILE__, __LINE__);
        build_horspool_lookup(Global::Options::templateLoopSeparatorLookup, Global::Options::templateLoopSeparator, length);
    }

    Global::Options::templateLoopSeparatorLength = length;
}

void Global::Options::setTemplateLoopReverse(const char* value) {
    if(Global::Options::templateLoopReverse != nullptr)
        re::free(Global::Options::templateLoopReverse);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateLoopReverse = (uint8_t*) re::malloc(length, "Template loop reverse marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateLoopReverse, value, length);

    Global::Options::templateLoopReverseLength = length;
}

void Global::Options::setTemplateComponent(const char* value) {
    if(Global::Options::templateComponent != nullptr)
        re::free(Global::Options::templateComponent);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateComponent = (uint8_t*) re::malloc(length, "Template component marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateComponent, value, length);

    Global::Options::templateComponentLength = length;
}

void Global::Options::setTemplateComponentSeparator(const char* value) {
    if(Global::Options::templateComponentSeparator != nullptr)
        re::free(Global::Options::templateComponentSeparator);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateComponentSeparator = (uint8_t*) re::malloc(length, "Template component separator marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateComponentSeparator, value, length);

    if(Global::Options::templateComponentSeparatorLookup != nullptr)
        re::free(Global::Options::templateComponentSeparatorLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateComponentSeparatorLookup = (uint8_t*) re::malloc(length, "Template component separator lookup", __FILE__, __LINE__);
        build_kmp_lookup(Global::Options::templateComponentSeparatorLookup, Global::Options::templateComponentSeparator, length);
    } else {
        Global::Options::templateComponentSeparatorLookup = (uint8_t*) re::malloc(256u, "Template component separator lookup", __FILE__, __LINE__);
        build_horspool_lookup(Global::Options::templateComponentSeparatorLookup, Global::Options::templateComponentSeparator, length);
    }

    Global::Options::templateComponentSeparatorLength = length;
}

void Global::Options::setTemplateComponentSelf(const char* value) {
    if(Global::Options::templateComponentSelf != nullptr)
        re::free(Global::Options::templateComponentSelf);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateComponentSelf = (uint8_t*) re::malloc(length, "Template component self marker", __FILE__, __LINE__);

    memcpy(Global::Options::templateComponentSelf, value, length);

    Global::Options::templateComponentSelfLength = length;
}

void Global::Options::restoreDefaults() {
    Global::Options::setBypassCache(false);
    Global::Options::setThrowOnEmptyContent(false);
    Global::Options::setThrowOnEmptyContent(false);
    Global::Options::setThrowOnCompileDirError(false);
    Global::Options::setIgnoreBlankPlaintext(false);
    Global::Options::setLogRenderTime(false);
    Global::Options::setCloneIterators(false);

    Global::Options::setWorkingDirectory(".");

    Global::Options::setTemplateEscape('\\');

    Global::Options::setTemplateStart("[|");
    Global::Options::setTemplateEnd("|]");

    Global::Options::setTemplateBodyEnd("end");

    Global::Options::setTemplateVoid("#");

    Global::Options::setTemplateComment("//");
    Global::Options::setTemplateCommentEnd("//|]");

    Global::Options::setTemplateConditionalStart("?");

    Global::Options::setTemplateInvertedConditionalStart("!");

    Global::Options::setTemplateLoopStart("@");
    Global::Options::setTemplateLoopSeparator(":");
    Global::Options::setTemplateLoopReverse("~");

    Global::Options::setTemplateComponent("%");
    Global::Options::setTemplateComponentSeparator(":");
    Global::Options::setTemplateComponentSelf("/");
}

void Global::Options::destroy() {
    re::free(workingDirectory);

    re::free(templateStart);
    re::free(templateStartLookup);

    re::free(templateEnd);
    re::free(templateEndLookup);

    re::free(templateBodyEnd);

    re::free(templateVoid);

    re::free(templateComment);

    re::free(templateCommentEnd);
    re::free(templateCommentEndLookup);

    re::free(templateConditionalStart);

    re::free(templateInvertedConditionalStart);

    re::free(templateLoopStart);
    re::free(templateLoopSeparator);
    re::free(templateLoopSeparatorLookup);
    re::free(templateLoopReverse);

    re::free(templateComponent);
    
    re::free(templateComponentSeparator);
    re::free(templateComponentSeparatorLookup);

    re::free(templateComponentSelf);
}

bool Global::Options::getBypassCache() {
    return Global::Options::bypassCache;
}

bool Global::Options::getThrowOnEmptyContent() {
    return Global::Options::throwOnEmptyContent;
}

bool Global::Options::getThrowOnMissingEntry() {
    return Global::Options::throwOnMissingEntry;
}

bool Global::Options::getThrowOnCompileDirError() {
    return Global::Options::throwOnCompileDirError;
}

bool Global::Options::getIgnoreBlankPlaintext() {
    return Global::Options::ignoreBlankPlaintext;
}

bool Global::Options::getLogRenderTime() {
    return Global::Options::logRenderTime;
}

bool Global::Options::getCloneIterators() {
    return Global::Options::cloneIterators;
}

const char* Global::Options::getWorkingDirectory() {
    return Global::Options::workingDirectory;
}

uint8_t Global::Options::getTemplateEscape() {
    return Global::Options::templateEscape;
}

const uint8_t* Global::Options::getTemplateStart() {
    return Global::Options::templateStart;
}

uint8_t Global::Options::getTemplateStartLength() {
    return Global::Options::templateStartLength;
}

const uint8_t* Global::Options::getTemplateStartLookup() {
    return Global::Options::templateStartLookup;
}

const uint8_t* Global::Options::getTemplateEnd() {
    return Global::Options::templateEnd;
}

uint8_t Global::Options::getTemplateEndLength() {
    return Global::Options::templateEndLength;
}

const uint8_t* Global::Options::getTemplateEndLookup() {
    return Global::Options::templateEndLookup;
}

const uint8_t* Global::Options::getTemplateBodyEnd() {
    return Global::Options::templateBodyEnd;
}

uint8_t Global::Options::getTemplateBodyEndLength() {
    return Global::Options::templateBodyEndLength;
}

const uint8_t* Global::Options::getTemplateVoid() {
    return Global::Options::templateVoid;
}

uint8_t Global::Options::getTemplateVoidLength() {
    return Global::Options::templateVoidLength;
}

const uint8_t* Global::Options::getTemplateComment() {
    return Global::Options::templateComment;
}

uint8_t Global::Options::getTemplateCommentLength() {
    return Global::Options::templateCommentLength;
}

const uint8_t* Global::Options::getTemplateCommentEnd() {
    return Global::Options::templateCommentEnd;
}

uint8_t Global::Options::getTemplateCommentEndLength() {
    return Global::Options::templateCommentEndLength;
}

const uint8_t* Global::Options::getTemplateCommentEndLookup() {
    return Global::Options::templateCommentEndLookup;
}

const uint8_t* Global::Options::getTemplateConditionalStart() {
    return Global::Options::templateConditionalStart;
}

uint8_t Global::Options::getTemplateConditionalStartLength() {
    return Global::Options::templateConditionalStartLength;
}

const uint8_t* Global::Options::getTemplateInvertedConditionalStart() {
    return Global::Options::templateInvertedConditionalStart;
}

uint8_t Global::Options::getTemplateInvertedConditionalStartLength() {
    return Global::Options::templateInvertedConditionalStartLength;
}

const uint8_t* Global::Options::getTemplateLoopStart() {
    return Global::Options::templateLoopStart;
}

uint8_t Global::Options::getTemplateLoopStartLength() {
    return Global::Options::templateLoopStartLength;
}

const uint8_t* Global::Options::getTemplateLoopSeparator() {
    return Global::Options::templateLoopSeparator;
}

uint8_t Global::Options::getTemplateLoopSeparatorLength() {
    return Global::Options::templateLoopSeparatorLength;
}

const uint8_t* Global::Options::getTemplateLoopReverse() {
    return Global::Options::templateLoopReverse;
}

uint8_t Global::Options::getTemplateLoopReverseLength() {
    return Global::Options::templateLoopReverseLength;
}

const uint8_t* Global::Options::getTemplateLoopSeparatorLookup() {
    return Global::Options::templateLoopSeparatorLookup;
}

const uint8_t* Global::Options::getTemplateComponent() {
    return Global::Options::templateComponent;
}

uint8_t Global::Options::getTemplateComponentLength() {
    return Global::Options::templateComponentLength;
}

const uint8_t* Global::Options::getTemplateComponentSeparator() {
    return Global::Options::templateComponentSeparator;
}

uint8_t Global::Options::getTemplateComponentSeparatorLength() {
    return Global::Options::templateComponentSeparatorLength;
}

const uint8_t* Global::Options::getTemplateComponentSeparatorLookup() {
    return Global::Options::templateComponentSeparatorLookup;
}

const uint8_t* Global::Options::getTemplateComponentSelf() {
    return Global::Options::templateComponentSelf;
}

uint8_t Global::Options::getTemplateComponentSelfLength() {
    return Global::Options::templateComponentSelfLength;
}