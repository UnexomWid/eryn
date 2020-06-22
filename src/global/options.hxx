#ifndef ERYN_GLOBAL_OPTIONS_HXX_GUARD
#define ERYN_GLOBAL_OPTIONS_HXX_GUARD

#include <cstdint>

namespace Global {
    class Options {
        private:
            static bool bypassCache;
            static bool throwOnEmptyContent;
            static bool throwOnMissingEntry;
            static bool throwOnCompileDirError;
            static bool ignoreBlankPlaintext;
            static bool logRenderTime;
            static bool cloneIterators;

            static char*    workingDirectory;

            static uint8_t  templateEscape;

            static uint8_t* templateStart;
            static uint8_t  templateStartLength;
            static uint8_t* templateStartLookup;

            static uint8_t* templateEnd;
            static uint8_t  templateEndLength;
            static uint8_t* templateEndLookup;

            static uint8_t* templateBodyEnd;
            static uint8_t  templateBodyEndLength;

            static uint8_t* templateVoid;
            static uint8_t  templateVoidLength;

            static uint8_t* templateComment;
            static uint8_t  templateCommentLength;

            static uint8_t* templateCommentEnd;
            static uint8_t  templateCommentEndLength;
            static uint8_t* templateCommentEndLookup;

            static uint8_t* templateConditionalStart;
            static uint8_t  templateConditionalStartLength;

            static uint8_t* templateInvertedConditionalStart;
            static uint8_t  templateInvertedConditionalStartLength;

            static uint8_t* templateLoopStart;
            static uint8_t  templateLoopStartLength;

            static uint8_t* templateLoopSeparator;
            static uint8_t  templateLoopSeparatorLength;
            static uint8_t* templateLoopSeparatorLookup;

            static uint8_t* templateLoopReverse;
            static uint8_t  templateLoopReverseLength;

            static uint8_t* templateComponent;
            static uint8_t  templateComponentLength;

            static uint8_t* templateComponentSeparator;
            static uint8_t  templateComponentSeparatorLength;
            static uint8_t* templateComponentSeparatorLookup;

            static uint8_t* templateComponentSelf;
            static uint8_t  templateComponentSelfLength;

        public:
            static bool getBypassCache();
            static bool getThrowOnEmptyContent();
            static bool getThrowOnMissingEntry();
            static bool getThrowOnCompileDirError();
            static bool getIgnoreBlankPlaintext();
            static bool getLogRenderTime();
            static bool getCloneIterators();

            static const char*    getWorkingDirectory();

            static       uint8_t  getTemplateEscape();

            static const uint8_t* getTemplateStart();
            static       uint8_t  getTemplateStartLength();
            static const uint8_t* getTemplateStartLookup();

            static const uint8_t* getTemplateEnd();
            static       uint8_t  getTemplateEndLength();
            static const uint8_t* getTemplateEndLookup();

            static const uint8_t* getTemplateBodyEnd();
            static       uint8_t  getTemplateBodyEndLength();
            
            static const uint8_t* getTemplateVoid();
            static       uint8_t  getTemplateVoidLength();

            static const uint8_t* getTemplateComment();
            static       uint8_t  getTemplateCommentLength();

            static const uint8_t* getTemplateCommentEnd();
            static       uint8_t  getTemplateCommentEndLength();
            static const uint8_t* getTemplateCommentEndLookup();

            static const uint8_t* getTemplateConditionalStart();
            static       uint8_t  getTemplateConditionalStartLength();

            static const uint8_t* getTemplateInvertedConditionalStart();
            static       uint8_t  getTemplateInvertedConditionalStartLength();

            static const uint8_t* getTemplateLoopStart();
            static       uint8_t  getTemplateLoopStartLength();

            static const uint8_t* getTemplateLoopSeparator();
            static       uint8_t  getTemplateLoopSeparatorLength();
            static const uint8_t* getTemplateLoopSeparatorLookup();

            static const uint8_t* getTemplateLoopReverse();
            static       uint8_t  getTemplateLoopReverseLength();

            static const uint8_t* getTemplateComponent();
            static       uint8_t  getTemplateComponentLength();

            static const uint8_t* getTemplateComponentSeparator();
            static       uint8_t  getTemplateComponentSeparatorLength();
            static const uint8_t* getTemplateComponentSeparatorLookup();

            static const uint8_t* getTemplateComponentSelf();
            static       uint8_t  getTemplateComponentSelfLength();

            static void setBypassCache(bool value);
            static void setThrowOnEmptyContent(bool value);
            static void setThrowOnMissingEntry(bool value);
            static void setThrowOnCompileDirError(bool value);
            static void setIgnoreBlankPlaintext(bool value);
            static void setLogRenderTime(bool value);
            static void setCloneIterators(bool value);

            static void setWorkingDirectory(const char* value);

            static void setTemplateEscape(char value);

            static void setTemplateStart(const char* value);
            static void setTemplateEnd(const char* value);

            static void setTemplateBodyEnd(const char* value);

            static void setTemplateVoid(const char* value);

            static void setTemplateComment(const char* value);
            static void setTemplateCommentEnd(const char* value);

            static void setTemplateConditionalStart(const char* value);

            static void setTemplateInvertedConditionalStart(const char* value);

            static void setTemplateLoopStart(const char* value);
            static void setTemplateLoopSeparator(const char* value);
            static void setTemplateLoopReverse(const char* value);

            static void setTemplateComponent(const char* value);
            static void setTemplateComponentSeparator(const char* value);
            static void setTemplateComponentSelf(const char* value);

            static void restoreDefaults();
            static void destroy();
    };
}

#endif