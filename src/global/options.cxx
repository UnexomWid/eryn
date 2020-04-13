#include "options.hxx"
#include "../../lib/mem_find.h"
#include "../../lib/buffer.hxx"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

bool Global::Options::bypassCache         = false;
bool Global::Options::throwOnEmptyContent = true;

uint8_t* Global::Options::templateStart                    = nullptr;
uint8_t  Global::Options::templateStartLength              = 0;
uint8_t* Global::Options::templateStartLookup              = nullptr;

uint8_t* Global::Options::templateEnd                      = nullptr;
uint8_t  Global::Options::templateEndLength                = 0;
uint8_t* Global::Options::templateEndLookup                = nullptr;

uint8_t* Global::Options::templateConditionalStart         = nullptr;
uint8_t  Global::Options::templateConditionalStartLength   = 0;

uint8_t* Global::Options::templateConditionalEnd           = nullptr;
uint8_t  Global::Options::templateConditionalEndLength     = 0;

uint8_t* Global::Options::templateLoopStart                = nullptr;
uint8_t  Global::Options::templateLoopStartLength          = 0;

uint8_t* Global::Options::templateLoopEnd                  = nullptr;
uint8_t  Global::Options::templateLoopEndLength            = 0;

uint8_t* Global::Options::templateLoopSeparator            = nullptr;
uint8_t  Global::Options::templateLoopSeparatorLength      = 0;
uint8_t* Global::Options::templateLoopSeparatorLookup      = nullptr;

uint8_t* Global::Options::templateComponent                = nullptr;
uint8_t  Global::Options::templateComponentLength          = 0;

uint8_t* Global::Options::templateComponentSeparator       = nullptr;
uint8_t  Global::Options::templateComponentSeparatorLength = 0;
uint8_t* Global::Options::templateComponentSeparatorLookup = nullptr;

uint8_t* Global::Options::templateComponentSelf            = nullptr;
uint8_t  Global::Options::templateComponentSelfLength      = 0;
uint8_t* Global::Options::templateComponentSelfLookup      = nullptr;

uint8_t* Global::Options::templateComponentEnd             = nullptr;
uint8_t  Global::Options::templateComponentEndLength       = 0;

void Global::Options::setBypassCache(bool value) {
    bypassCache = value;
}

void Global::Options::setThrowOnEmptyContent(bool value) {
    throwOnEmptyContent = value;
}

void Global::Options::setTemplateStart(const char* value) {
    if(templateStart != nullptr)
        qfree(templateStart);

    uint8_t length = (uint8_t) strlen(value);
    templateStart = qmalloc(length);

    memcpy(templateStart, value, length);

    if(templateStartLookup != nullptr)
        qfree(templateStartLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateStartLookup = qmalloc(length);
        build_kmp_lookup(templateStartLookup, templateStart, length);
    } else {
        templateStartLookup = qmalloc(256u);
        build_horspool_lookup(templateStartLookup, templateStart, length);
    }

    Global::Options::templateStartLength = length;
}

void Global::Options::setTemplateEnd(const char* value) {
    if(templateEnd != nullptr)
        qfree(templateEnd);

    uint8_t length = (uint8_t) strlen(value);
    templateEnd = qmalloc(length);

    memcpy(templateEnd, value, length);

    if(templateEndLookup != nullptr)
        qfree(templateEndLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateEndLookup = qmalloc(length);
        build_kmp_lookup(templateEndLookup, templateEnd, length);
    } else {
        templateEndLookup = qmalloc(256u);
        build_horspool_lookup(templateEndLookup, templateEnd, length);
    }

    Global::Options::templateEndLength = length;
}

void Global::Options::setTemplateConditionalStart(const char* value) {
    if(templateConditionalStart != nullptr)
        qfree(templateConditionalStart);

    uint8_t length = (uint8_t) strlen(value);
    templateConditionalStart = qmalloc(length);

    memcpy(templateConditionalStart, value, length);

    Global::Options::templateConditionalStartLength = length;
}

void Global::Options::setTemplateConditionalEnd(const char* value) {
    if(templateConditionalEnd != nullptr)
        qfree(templateConditionalEnd);

    uint8_t length = (uint8_t) strlen(value);
    templateConditionalEnd = qmalloc(length);

    memcpy(templateConditionalEnd, value, length);

    Global::Options::templateConditionalEndLength = length;
}

void Global::Options::setTemplateLoopStart(const char* value) {
    if(templateLoopStart != nullptr)
        qfree(templateLoopStart);

    uint8_t length = (uint8_t) strlen(value);
    templateLoopStart = qmalloc(length);

    memcpy(templateLoopStart, value, length);

    Global::Options::templateLoopStartLength = length;
}

void Global::Options::setTemplateLoopEnd(const char* value) {
    if(templateLoopEnd != nullptr)
        qfree(templateLoopEnd);

    uint8_t length = (uint8_t) strlen(value);
    templateLoopEnd = qmalloc(length);

    memcpy(templateLoopEnd, value, length);

    Global::Options::templateLoopEndLength = length;
}

void Global::Options::setTemplateLoopSeparator(const char* value) {
    if(templateLoopSeparator != nullptr)
        qfree(templateLoopSeparator);

    uint8_t length = (uint8_t) strlen(value);
    templateLoopSeparator = qmalloc(length);

    memcpy(templateLoopSeparator, value, length);

    if(templateLoopSeparatorLookup != nullptr)
        qfree(templateLoopSeparatorLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateLoopSeparatorLookup = qmalloc(length);
        build_kmp_lookup(templateLoopSeparatorLookup, templateLoopSeparator, length);
    } else {
        templateLoopSeparatorLookup = qmalloc(256u);
        build_horspool_lookup(templateLoopSeparatorLookup, templateLoopSeparator, length);
    }

    Global::Options::templateLoopSeparatorLength = length;
}

void Global::Options::setTemplateComponent(const char* value) {
    if(templateComponent != nullptr)
        qfree(templateComponent);

    uint8_t length = (uint8_t) strlen(value);
    templateComponent = qmalloc(length);

    memcpy(templateComponent, value, length);

    Global::Options::templateComponentLength = length;
}

void Global::Options::setTemplateComponentSeparator(const char* value) {
    if(templateComponentSeparator != nullptr)
        qfree(templateComponentSeparator);

    uint8_t length = (uint8_t) strlen(value);
    templateComponentSeparator = qmalloc(length);

    memcpy(templateComponentSeparator, value, length);

    if(templateComponentSeparatorLookup != nullptr)
        qfree(templateComponentSeparatorLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateComponentSeparatorLookup = qmalloc(length);
        build_kmp_lookup(templateComponentSeparatorLookup, templateComponentSeparator, length);
    } else {
        templateComponentSeparatorLookup = qmalloc(256u);
        build_horspool_lookup(templateComponentSeparatorLookup, templateComponentSeparator, length);
    }

    Global::Options::templateComponentSeparatorLength = length;
}

void Global::Options::setTemplateComponentSelf(const char* value) {
    if(templateComponentSelf != nullptr)
        qfree(templateComponentSelf);

    uint8_t length = (uint8_t) strlen(value);
    templateComponentSelf = qmalloc(length);

    memcpy(templateComponentSelf, value, length);

    if(templateComponentSelfLookup != nullptr)
        qfree(templateComponentSelfLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateComponentSelfLookup = qmalloc(length);
        build_kmp_lookup(templateComponentSelfLookup, templateComponentSelf, length);
    } else {
        templateComponentSelfLookup = qmalloc(256u);
        build_horspool_lookup(templateComponentSelfLookup, templateComponentSelf, length);
    }

    Global::Options::templateComponentSelfLength = length;
}

void Global::Options::setTemplateComponentEnd(const char* value) {
    if(templateComponentEnd != nullptr)
        qfree(templateComponentEnd);

    uint8_t length = (uint8_t) strlen(value);
    templateComponentEnd = qmalloc(length);

    memcpy(templateComponentEnd, value, length);

    Global::Options::templateComponentEndLength = length;
}

void Global::Options::restoreDefaults() {
    Global::Options::setBypassCache(false);
    Global::Options::setThrowOnEmptyContent(true);

    Global::Options::setTemplateStart("[|");
    Global::Options::setTemplateEnd("|]");

    Global::Options::setTemplateConditionalStart("?");
    Global::Options::setTemplateConditionalEnd("!");

    Global::Options::setTemplateLoopStart("@");
    Global::Options::setTemplateLoopEnd("#");
    Global::Options::setTemplateLoopSeparator(":");

    Global::Options::setTemplateComponent("%");
    Global::Options::setTemplateComponentSeparator(":");
    Global::Options::setTemplateComponentSelf("/");
    Global::Options::setTemplateComponentEnd("/");
}

void Global::Options::destroy() {
    if(templateStart != nullptr)
        qfree(templateStart);
    if(templateStartLookup != nullptr)
        qfree(templateStartLookup);

    if(templateEnd != nullptr)
        qfree(templateEnd);
    if(templateEndLookup != nullptr)
        qfree(templateEndLookup);

    if(templateConditionalStart != nullptr)
        qfree(templateConditionalStart);
    if(templateConditionalEnd != nullptr)
        qfree(templateConditionalEnd);

    if(templateLoopStart != nullptr)
        qfree(templateLoopStart);
    if(templateLoopEnd != nullptr)
        qfree(templateLoopEnd);
    if(templateLoopSeparator != nullptr)
        qfree(templateLoopSeparator);
    if(templateLoopSeparatorLookup != nullptr)
        qfree(templateLoopSeparatorLookup);

    if(templateComponent != nullptr)
        qfree(templateComponent);
        
    if(templateComponentSeparator != nullptr)
        qfree(templateComponentSeparator);
    if(templateComponentSeparatorLookup != nullptr)
        qfree(templateComponentSeparatorLookup);

    if(templateComponentSelf != nullptr)
        qfree(templateComponentSelf);
    if(templateComponentSelfLookup != nullptr)
        qfree(templateComponentSelfLookup);

    if(templateComponentEnd != nullptr)
        qfree(templateComponentEnd);
}

bool Global::Options::getBypassCache() {
    return bypassCache;
}

bool Global::Options::getThrowOnEmptyContent() {
    return throwOnEmptyContent;
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

const uint8_t* Global::Options::getTemplateConditionalStart() {
    return Global::Options::templateConditionalStart;
}

uint8_t Global::Options::getTemplateConditionalStartLength() {
    return Global::Options::templateConditionalStartLength;
}

const uint8_t* Global::Options::getTemplateConditionalEnd() {
    return Global::Options::templateConditionalEnd;
}

uint8_t Global::Options::getTemplateConditionalEndLength() {
    return Global::Options::templateConditionalEndLength;
}

const uint8_t* Global::Options::getTemplateLoopStart() {
    return Global::Options::templateLoopStart;
}

uint8_t Global::Options::getTemplateLoopStartLength() {
    return Global::Options::templateLoopStartLength;
}

const uint8_t* Global::Options::getTemplateLoopEnd() {
    return Global::Options::templateLoopEnd;
}

uint8_t Global::Options::getTemplateLoopEndLength() {
    return Global::Options::templateLoopEndLength;
}

const uint8_t* Global::Options::getTemplateLoopSeparator() {
    return Global::Options::templateLoopSeparator;
}

uint8_t Global::Options::getTemplateLoopSeparatorLength() {
    return Global::Options::templateLoopSeparatorLength;
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

const uint8_t* Global::Options::getTemplateComponentSelfLookup() {
    return Global::Options::templateComponentSelfLookup;
}

const uint8_t* Global::Options::getTemplateComponentEnd() {
    return Global::Options::templateComponentEnd;
}

uint8_t Global::Options::getTemplateComponentEndLength() {
    return Global::Options::templateComponentEndLength;
}