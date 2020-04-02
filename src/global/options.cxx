#include "options.hxx"
#include "../../lib/mem_find.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

bool Global::Options::bypassCache = false;

uint8_t* Global::Options::templateStart = nullptr;
uint8_t  Global::Options::templateStartLength = 0;
uint8_t* Global::Options::templateStartLookup = nullptr;

uint8_t* Global::Options::templateEnd = nullptr;
uint8_t  Global::Options::templateEndLength = 0;
uint8_t* Global::Options::templateEndLookup = nullptr;

uint8_t* Global::Options::templateConditionalStart = nullptr;
uint8_t  Global::Options::templateConditionalStartLength = 0;

uint8_t* Global::Options::templateConditionalEnd = nullptr;
uint8_t  Global::Options::templateConditionalEndLength = 0;

uint8_t* Global::Options::templateLoopStart = nullptr;
uint8_t  Global::Options::templateLoopStartLength = 0;

uint8_t* Global::Options::templateLoopEnd = nullptr;
uint8_t  Global::Options::templateLoopEndLength = 0;

uint8_t* Global::Options::templateLoopSeparator = nullptr;
uint8_t  Global::Options::templateLoopSeparatorLength = 0;
uint8_t* Global::Options::templateLoopSeparatorLookup = nullptr;

uint8_t* Global::Options::templateComponent = nullptr;
uint8_t  Global::Options::templateComponentLength = 0;

uint8_t* Global::Options::templateComponentSeparator = nullptr;
uint8_t  Global::Options::templateComponentSeparatorLength = 0;
uint8_t* Global::Options::templateComponentSeparatorLookup = nullptr;

void Global::Options::setTemplateStart(const char* value) {
    if(templateStart != nullptr)
        free(templateStart);

    uint8_t length = (uint8_t) strlen(value);
    templateStart = (uint8_t*) malloc(length);

    memcpy(templateStart, value, length);

    if(templateStartLookup != nullptr)
        free(templateStartLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateStartLookup = (uint8_t*) malloc(length);
        build_kmp_lookup(templateStartLookup, templateStart, length);
    } else {
        templateStartLookup = (uint8_t*) malloc(256);
        build_horspool_lookup(templateStartLookup, templateStart, length);
    }

    Global::Options::templateStartLength = length;
}

void Global::Options::setTemplateEnd(const char* value) {
    if(templateEnd != nullptr)
        free(templateEnd);

    uint8_t length = (uint8_t) strlen(value);
    templateEnd = (uint8_t*) malloc(length);

    memcpy(templateEnd, value, length);

    if(templateEndLookup != nullptr)
        free(templateEndLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateEndLookup = (uint8_t*) malloc(length);
        build_kmp_lookup(templateEndLookup, templateEnd, length);
    } else {
        templateEndLookup = (uint8_t*) malloc(256);
        build_horspool_lookup(templateEndLookup, templateEnd, length);
    }

    Global::Options::templateEndLength = length;
}

void Global::Options::setTemplateConditionalStart(const char* value) {
    if(templateConditionalStart != nullptr)
        free(templateConditionalStart);

    uint8_t length = (uint8_t) strlen(value);
    templateConditionalStart = (uint8_t*) malloc(length);

    memcpy(templateConditionalStart, value, length);

    Global::Options::templateConditionalStartLength = length;
}

void Global::Options::setTemplateConditionalEnd(const char* value) {
    if(templateConditionalEnd != nullptr)
        free(templateConditionalEnd);

    uint8_t length = (uint8_t) strlen(value);
    templateConditionalEnd = (uint8_t*) malloc(length);

    memcpy(templateConditionalEnd, value, length);

    Global::Options::templateConditionalEndLength = length;
}

void Global::Options::setTemplateLoopStart(const char* value) {
    if(templateLoopStart != nullptr)
        free(templateLoopStart);

    uint8_t length = (uint8_t) strlen(value);
    templateLoopStart = (uint8_t*) malloc(length);

    memcpy(templateLoopStart, value, length);

    Global::Options::templateLoopStartLength = length;
}

void Global::Options::setTemplateLoopEnd(const char* value) {
    if(templateLoopEnd != nullptr)
        free(templateLoopEnd);

    uint8_t length = (uint8_t) strlen(value);
    templateLoopEnd = (uint8_t*) malloc(length);

    memcpy(templateLoopEnd, value, length);

    Global::Options::templateLoopEndLength = length;
}

void Global::Options::setTemplateLoopSeparator(const char* value) {
    if(templateLoopSeparator != nullptr)
        free(templateLoopSeparator);

    uint8_t length = (uint8_t) strlen(value);
    templateLoopSeparator = (uint8_t*) malloc(length);

    memcpy(templateLoopSeparator, value, length);

    if(templateLoopSeparatorLookup != nullptr)
        free(templateLoopSeparatorLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateLoopSeparatorLookup = (uint8_t*) malloc(length);
        build_kmp_lookup(templateLoopSeparatorLookup, templateLoopSeparator, length);
    } else {
        templateLoopSeparatorLookup = (uint8_t*) malloc(256);
        build_horspool_lookup(templateLoopSeparatorLookup, templateLoopSeparator, length);
    }

    Global::Options::templateLoopSeparatorLength = length;
}

void Global::Options::setTemplateComponent(const char* value) {
    if(templateComponent != nullptr)
        free(templateComponent);

    uint8_t length = (uint8_t) strlen(value);
    templateComponent = (uint8_t*) malloc(length);

    memcpy(templateComponent, value, length);

    Global::Options::templateComponentLength = length;
}

void Global::Options::setTemplateComponentSeparator(const char* value) {
    if(templateComponentSeparator != nullptr)
        free(templateComponentSeparator);

    uint8_t length = (uint8_t) strlen(value);
    templateComponentSeparator = (uint8_t*) malloc(length);

    memcpy(templateComponentSeparator, value, length);

    if(templateComponentSeparatorLookup != nullptr)
        free(templateComponentSeparatorLookup);

    if(length < HORSPOOL_THRESHOLD) {
        templateComponentSeparatorLookup = (uint8_t*) malloc(length);
        build_kmp_lookup(templateComponentSeparatorLookup, templateComponentSeparator, length);
    } else {
        templateComponentSeparatorLookup = (uint8_t*) malloc(256);
        build_horspool_lookup(templateComponentSeparatorLookup, templateComponentSeparator, length);
    }

    Global::Options::templateComponentSeparatorLength = length;
}

void Global::Options::restoreDefaults() {
    Global::Options::setTemplateStart("[|");
    Global::Options::setTemplateEnd("|]");

    Global::Options::setTemplateConditionalStart("?");
    Global::Options::setTemplateConditionalEnd("!");

    Global::Options::setTemplateLoopStart("@");
    Global::Options::setTemplateLoopEnd("#");
    Global::Options::setTemplateLoopSeparator(":");

    Global::Options::setTemplateComponent("%");
    Global::Options::setTemplateComponentSeparator(":");

    Global::Options::bypassCache = false;
}

void Global::Options::destroy() {
    if(templateStart != nullptr)
        free(templateStart);
    if(templateEnd != nullptr)
        free(templateEnd);

    if(templateStartLookup != nullptr)
        free(templateStartLookup);
    if(templateEndLookup != nullptr)
        free(templateEndLookup);

    if(templateConditionalStart != nullptr)
        free(templateConditionalStart);
    if(templateConditionalEnd != nullptr)
        free(templateConditionalEnd);

    if(templateLoopStart != nullptr)
        free(templateLoopStart);
    if(templateLoopEnd != nullptr)
        free(templateLoopEnd);
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

bool Global::Options::getBypassCache() {
    return bypassCache;
}

void Global::Options::setBypassCache(bool value) {
    bypassCache = value;
}