#ifndef ERYN_GLOBAL_OPTIONS_HXX_GUARD
#define ERYN_GLOBAL_OPTIONS_HXX_GUARD

#include <cstdint>

namespace Global {
    class Options {
        private:
            static bool bypassCache;
            static bool throwOnEmptyContent;
            static bool throwOnMissingEntry;

            static uint8_t* templateStart;
            static uint8_t  templateStartLength;
            static uint8_t* templateStartLookup;

            static uint8_t* templateEnd;
            static uint8_t  templateEndLength;
            static uint8_t* templateEndLookup;

            static uint8_t* templateConditionalStart;
            static uint8_t  templateConditionalStartLength;

            static uint8_t* templateConditionalEnd;
            static uint8_t  templateConditionalEndLength;

            static uint8_t* templateLoopStart;
            static uint8_t  templateLoopStartLength;

            static uint8_t* templateLoopEnd;
            static uint8_t  templateLoopEndLength;

            static uint8_t* templateLoopSeparator;
            static uint8_t  templateLoopSeparatorLength;
            static uint8_t* templateLoopSeparatorLookup;

            static uint8_t* templateComponent;
            static uint8_t  templateComponentLength;

            static uint8_t* templateComponentSeparator;
            static uint8_t  templateComponentSeparatorLength;
            static uint8_t* templateComponentSeparatorLookup;

            static uint8_t* templateComponentSelf;
            static uint8_t  templateComponentSelfLength;
            static uint8_t* templateComponentSelfLookup;

            static uint8_t* templateComponentEnd;
            static uint8_t  templateComponentEndLength;

        public:
            static bool getBypassCache();
            static bool getThrowOnEmptyContent();
            static bool getThrowOnMissingEntry();

            static const uint8_t* getTemplateStart();
            static       uint8_t  getTemplateStartLength();
            static const uint8_t* getTemplateStartLookup();

            static const uint8_t* getTemplateEnd();
            static       uint8_t  getTemplateEndLength();
            static const uint8_t* getTemplateEndLookup();

            static const uint8_t* getTemplateConditionalStart();
            static       uint8_t  getTemplateConditionalStartLength();

            static const uint8_t* getTemplateConditionalEnd();
            static       uint8_t  getTemplateConditionalEndLength();

            static const uint8_t* getTemplateLoopStart();
            static       uint8_t  getTemplateLoopStartLength();

            static const uint8_t* getTemplateLoopEnd();
            static       uint8_t  getTemplateLoopEndLength();

            static const uint8_t* getTemplateLoopSeparator();
            static       uint8_t  getTemplateLoopSeparatorLength();
            static const uint8_t* getTemplateLoopSeparatorLookup();

            static const uint8_t* getTemplateComponent();
            static       uint8_t  getTemplateComponentLength();

            static const uint8_t* getTemplateComponentSeparator();
            static       uint8_t  getTemplateComponentSeparatorLength();
            static const uint8_t* getTemplateComponentSeparatorLookup();

            static const uint8_t* getTemplateComponentSelf();
            static       uint8_t  getTemplateComponentSelfLength();
            static const uint8_t* getTemplateComponentSelfLookup();

            static const uint8_t* getTemplateComponentEnd();
            static       uint8_t  getTemplateComponentEndLength();

            static void setBypassCache(bool value);
            static void setThrowOnEmptyContent(bool value);
            static void setThrowOnMissingEntry(bool value);

            static void setTemplateStart(const char* value);
            static void setTemplateEnd(const char* value);

            static void setTemplateConditionalStart(const char* value);
            static void setTemplateConditionalEnd(const char* value);

            static void setTemplateLoopStart(const char* value);
            static void setTemplateLoopEnd(const char* value);
            static void setTemplateLoopSeparator(const char* value);

            static void setTemplateComponent(const char* value);
            static void setTemplateComponentSeparator(const char* value);
            static void setTemplateComponentSelf(const char* value);
            static void setTemplateComponentEnd(const char* value);

            static void restoreDefaults();
            static void destroy();
    };
}

#endif