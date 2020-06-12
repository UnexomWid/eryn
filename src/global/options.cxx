#include "options.hxx"
#include "../../lib/mem_find.h"
#include "../../lib/buffer.hxx"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

bool Global::Options::bypassCache            = false;
bool Global::Options::throwOnEmptyContent    = false;
bool Global::Options::throwOnMissingEntry    = false;
bool Global::Options::throwOnCompileDirError = false;
bool Global::Options::ignoreBlankPlaintext   = false;
bool Global::Options::logRenderTime          = false;

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

void Global::Options::setWorkingDirectory(const char* value) {
    if(Global::Options::workingDirectory != nullptr)
        qfree(Global::Options::workingDirectory);

    Global::Options::workingDirectory = qstrdup(value);
}

void Global::Options::setTemplateEscape(char value) {
    Global::Options::templateEscape = (uint8_t) value;
}

void Global::Options::setTemplateStart(const char* value) {
    if(Global::Options::templateStart != nullptr)
        qfree(Global::Options::templateStart);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateStart = qmalloc(length);

    memcpy(Global::Options::templateStart, value, length);

    if(Global::Options::templateStartLookup != nullptr)
        qfree(Global::Options::templateStartLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateStartLookup = qmalloc(length);
        build_kmp_lookup(Global::Options::templateStartLookup, Global::Options::templateStart, length);
    } else {
        Global::Options::templateStartLookup = qmalloc(256u);
        build_horspool_lookup(Global::Options::templateStartLookup, Global::Options::templateStart, length);
    }

    Global::Options::templateStartLength = length;
}

void Global::Options::setTemplateEnd(const char* value) {
    if(Global::Options::templateEnd != nullptr)
        qfree(Global::Options::templateEnd);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateEnd = qmalloc(length);

    memcpy(Global::Options::templateEnd, value, length);

    if(Global::Options::templateEndLookup != nullptr)
        qfree(Global::Options::templateEndLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateEndLookup = qmalloc(length);
        build_kmp_lookup(Global::Options::templateEndLookup, Global::Options::templateEnd, length);
    } else {
        Global::Options::templateEndLookup = qmalloc(256u);
        build_horspool_lookup(Global::Options::templateEndLookup, Global::Options::templateEnd, length);
    }

    Global::Options::templateEndLength = length;
}

void Global::Options::setTemplateBodyEnd(const char* value) {
    if(Global::Options::templateBodyEnd != nullptr)
        qfree(Global::Options::templateBodyEnd);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateBodyEnd = qmalloc(length);

    memcpy(Global::Options::templateBodyEnd, value, length);

    Global::Options::templateBodyEndLength = length;
}

void Global::Options::setTemplateVoid(const char* value) {
    if(Global::Options::templateVoid != nullptr)
        qfree(Global::Options::templateVoid);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateVoid = qmalloc(length);

    memcpy(Global::Options::templateVoid, value, length);

    Global::Options::templateVoidLength = length;
}

void Global::Options::setTemplateComment(const char* value) {
    if(Global::Options::templateComment != nullptr)
        qfree(Global::Options::templateComment);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateComment = qmalloc(length);

    memcpy(Global::Options::templateComment, value, length);

    Global::Options::templateCommentLength = length;
}

void Global::Options::setTemplateCommentEnd(const char* value) {
    if(Global::Options::templateCommentEnd != nullptr)
        qfree(Global::Options::templateCommentEnd);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateCommentEnd = qmalloc(length);

    memcpy(Global::Options::templateCommentEnd, value, length);

    if(Global::Options::templateCommentEndLookup != nullptr)
        qfree(Global::Options::templateCommentEndLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateCommentEndLookup = qmalloc(length);
        build_kmp_lookup(Global::Options::templateCommentEndLookup, Global::Options::templateCommentEnd, length);
    } else {
        Global::Options::templateCommentEndLookup = qmalloc(256u);
        build_horspool_lookup(Global::Options::templateCommentEndLookup, Global::Options::templateCommentEnd, length);
    }

    Global::Options::templateCommentEndLength = length;
}

void Global::Options::setTemplateConditionalStart(const char* value) {
    if(Global::Options::templateConditionalStart != nullptr)
        qfree(Global::Options::templateConditionalStart);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateConditionalStart = qmalloc(length);

    memcpy(Global::Options::templateConditionalStart, value, length);

    Global::Options::templateConditionalStartLength = length;
}

void Global::Options::setTemplateInvertedConditionalStart(const char* value) {
    if(Global::Options::templateInvertedConditionalStart != nullptr)
        qfree(Global::Options::templateInvertedConditionalStart);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateInvertedConditionalStart = qmalloc(length);

    memcpy(Global::Options::templateInvertedConditionalStart, value, length);

    Global::Options::templateInvertedConditionalStartLength = length;
}

void Global::Options::setTemplateLoopStart(const char* value) {
    if(Global::Options::templateLoopStart != nullptr)
        qfree(Global::Options::templateLoopStart);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateLoopStart = qmalloc(length);

    memcpy(Global::Options::templateLoopStart, value, length);

    Global::Options::templateLoopStartLength = length;
}

void Global::Options::setTemplateLoopSeparator(const char* value) {
    if(Global::Options::templateLoopSeparator != nullptr)
        qfree(Global::Options::templateLoopSeparator);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateLoopSeparator = qmalloc(length);

    memcpy(Global::Options::templateLoopSeparator, value, length);

    if(Global::Options::templateLoopSeparatorLookup != nullptr)
        qfree(Global::Options::templateLoopSeparatorLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateLoopSeparatorLookup = qmalloc(length);
        build_kmp_lookup(Global::Options::templateLoopSeparatorLookup, Global::Options::templateLoopSeparator, length);
    } else {
        templateLoopSeparatorLookup = qmalloc(256u);
        build_horspool_lookup(Global::Options::templateLoopSeparatorLookup, Global::Options::templateLoopSeparator, length);
    }

    Global::Options::templateLoopSeparatorLength = length;
}

void Global::Options::setTemplateLoopReverse(const char* value) {
    if(Global::Options::templateLoopReverse != nullptr)
        qfree(Global::Options::templateLoopReverse);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateLoopReverse = qmalloc(length);

    memcpy(Global::Options::templateLoopReverse, value, length);

    Global::Options::templateLoopReverseLength = length;
}

void Global::Options::setTemplateComponent(const char* value) {
    if(Global::Options::templateComponent != nullptr)
        qfree(Global::Options::templateComponent);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateComponent = qmalloc(length);

    memcpy(Global::Options::templateComponent, value, length);

    Global::Options::templateComponentLength = length;
}

void Global::Options::setTemplateComponentSeparator(const char* value) {
    if(Global::Options::templateComponentSeparator != nullptr)
        qfree(Global::Options::templateComponentSeparator);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateComponentSeparator = qmalloc(length);

    memcpy(Global::Options::templateComponentSeparator, value, length);

    if(Global::Options::templateComponentSeparatorLookup != nullptr)
        qfree(Global::Options::templateComponentSeparatorLookup);

    if(length < HORSPOOL_THRESHOLD) {
        Global::Options::templateComponentSeparatorLookup = qmalloc(length);
        build_kmp_lookup(Global::Options::templateComponentSeparatorLookup, Global::Options::templateComponentSeparator, length);
    } else {
        Global::Options::templateComponentSeparatorLookup = qmalloc(256u);
        build_horspool_lookup(Global::Options::templateComponentSeparatorLookup, Global::Options::templateComponentSeparator, length);
    }

    Global::Options::templateComponentSeparatorLength = length;
}

void Global::Options::setTemplateComponentSelf(const char* value) {
    if(Global::Options::templateComponentSelf != nullptr)
        qfree(Global::Options::templateComponentSelf);

    uint8_t length = (uint8_t) strlen(value);
    Global::Options::templateComponentSelf = qmalloc(length);

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
    qfree(workingDirectory);

    qfree(templateStart);
    qfree(templateStartLookup);

    qfree(templateEnd);
    qfree(templateEndLookup);

    qfree(templateBodyEnd);

    qfree(templateVoid);

    qfree(templateComment);

    qfree(templateCommentEnd);
    qfree(templateCommentEndLookup);

    qfree(templateConditionalStart);

    qfree(templateInvertedConditionalStart);

    qfree(templateLoopStart);
    qfree(templateLoopSeparator);
    qfree(templateLoopSeparatorLookup);
    qfree(templateLoopReverse);

    qfree(templateComponent);
    
    qfree(templateComponentSeparator);
    qfree(templateComponentSeparatorLookup);

    qfree(templateComponentSelf);
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