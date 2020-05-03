#include "napi.h"
#include <node_api.h>

#include "def/logging.dxx"

#include "global/cache.hxx"
#include "global/global.hxx"
#include "global/options.hxx"

#include "compiler/compiler.hxx"
#include "renderer/renderer.hxx"

#include <memory>

void bufferFinalizer(Napi::Env env, uint8_t* data) {
    LOG_DEBUG("Finalizing buffer %p", data);
    qfree(data);
}

std::string jsonStringify(const Napi::Env& env, const Napi::Object& object) {
    Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
    Napi::Function stringify = env.Global().Get("JSON").As<Napi::Object>().Get("stringify").As<Napi::Function>();

    return stringify.Call(json, { object }).As<Napi::String>().Utf8Value();
}

void erynSetOptions(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if(info.Length() != 1)
        throw Napi::Error::New(env, "Invalid argument count (expected 1: Object)");
    if(!info[0].IsObject())
        throw Napi::Error::New(env, "Invalid arguments (expected: Object)");

    Napi::Object options = info[0].As<Napi::Object>();
    Napi::Array keys = options.GetPropertyNames();

    for(uint32_t i = 0; i < keys.Length(); ++i) {
        Napi::Value keyVal = keys[i];

        if(!keyVal.IsString())
            continue;

        std::string key = keyVal.As<Napi::String>().Utf8Value();
        Napi::Value value = options.Get(keys[i]);

        if(key == "bypassCache") {
            if(!value.IsBoolean())
                continue;
            Global::Options::setBypassCache(value.ToBoolean().Value());
        } else if(key == "throwOnEmptyContent") {
            if(!value.IsBoolean())
                continue;
            Global::Options::setThrowOnEmptyContent(value.ToBoolean().Value());
        } else if(key == "throwOnMissingEntry") {
            if(!value.IsBoolean())
                continue;
            Global::Options::setThrowOnMissingEntry(value.ToBoolean().Value());
        } else if(key == "ignoreBlankPlaintext") {
            if(!value.IsBoolean())
                continue;
            Global::Options::setIgnoreBlankPlaintext(value.ToBoolean().Value());
        } else if(key == "logRenderTime") {
            if(!value.IsBoolean())
                continue;
            Global::Options::setLogRenderTime(value.ToBoolean().Value());
        } else if(key == "templateEscape") {
            if(!value.IsString() || value.As<Napi::String>().Utf8Value().size() != 1)
                continue;
            Global::Options::setTemplateEscape(*value.ToString().Utf8Value().c_str());
        } else if(key == "templateStart") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateStart(value.ToString().Utf8Value().c_str());
        } else if(key == "templateEnd") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateEnd(value.ToString().Utf8Value().c_str());
        } else if(key == "conditionalStart") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateConditionalStart(value.ToString().Utf8Value().c_str());
        } else if(key == "conditionalEnd") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateConditionalEnd(value.ToString().Utf8Value().c_str());
        } else if(key == "invertedConditionalStart") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateInvertedConditionalStart(value.ToString().Utf8Value().c_str());
        } else if(key == "invertedConditionalEnd") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateInvertedConditionalEnd(value.ToString().Utf8Value().c_str());
        } else if(key == "loopStart") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateLoopStart(value.ToString().Utf8Value().c_str());
        } else if(key == "loopEnd") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateLoopEnd(value.ToString().Utf8Value().c_str());
        } else if(key == "loopSeparator") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateLoopSeparator(value.ToString().Utf8Value().c_str());
        } else if(key == "componentStart") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateComponent(value.ToString().Utf8Value().c_str());
        } else if(key == "componentSeparator") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateComponentSeparator(value.ToString().Utf8Value().c_str());
        } else if(key == "componentSelf") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateComponentSelf(value.ToString().Utf8Value().c_str());
        } else if(key == "componentEnd") {
            if(!value.IsString())
                continue;
            Global::Options::setTemplateComponentEnd(value.ToString().Utf8Value().c_str());
        }
    }
}

void erynCompile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if(info.Length() != 1)
        throw Napi::Error::New(env, "Invalid argument count (expected 1: String)");
    if(!info[0].IsString())
        throw Napi::Error::New(env, "Invalid arguments (expected: String)");

    std::unique_ptr<char, decltype(qfree)*> path(
        qstrdup(info[0].As<Napi::String>().Utf8Value().c_str()), qfree);

    try {
        compile(path.get());

        #ifdef DUMP_OSH_FILES_ON_COMPILE
            FILE* f = fopen((path.get() + std::string(".osh")).c_str(), "wb");
            fwrite(Global::Cache::getEntry(path.get()).data, 1, Global::Cache::getEntry(path.get()).size, f);
            fclose(f);
        #endif
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    }
}

void erynCompileDir(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if(info.Length() != 2)
        throw Napi::Error::New(env, "Invalid argument count (expected 2: String, Array)");
    if(!info[0].IsString() || !info[1].IsArray())
        throw Napi::Error::New(env, "Invalid arguments (expected: String, Array)");

    std::unique_ptr<char, decltype(qfree)*> path(
        qstrdup(info[0].As<Napi::String>().Utf8Value().c_str()), qfree);

    std::vector<std::string> filters;

    Napi::Array filterArray = info[1].As<Napi::Array>();

    uint32_t length = filterArray.Length();

    for(uint32_t i = 0; i < length; ++i) {
        Napi::Value item = filterArray[i];
        if(!item.IsString())
            throw Napi::Error::New(env, "Invalid filter array (expected array of strings)");

        filters.push_back(item.As<Napi::String>().Utf8Value());
    }

    try {
        compileDir(path.get(), filters);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    }
}

Napi::Buffer<uint8_t> erynRender(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if(info.Length() != 2)
        throw Napi::Error::New(env, "Invalid argument count (expected 2: String, Object)");
    if(!info[0].IsString() || !info[1].IsObject())
        throw Napi::Error::New(env, "Invalid arguments (expected: String, Object)");

    std::unique_ptr<char, decltype(qfree)*> path(
        qstrdup(info[0].As<Napi::String>().Utf8Value().c_str()), qfree);
    Napi::Object context = info[1].As<Napi::Object>();

    env.RunScript("var context=" + jsonStringify(env, context));

    try {
        BinaryData rendered = render(env, path.get());

        env.RunScript("context=undefined");

        return Napi::Buffer<uint8_t>::New<decltype(bufferFinalizer)*>(
                   env, (uint8_t*) rendered.data, rendered.size, bufferFinalizer);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, ((std::string("Rendering error in '") + path.get()) + "'\n") + e.what());
    }
}

void destroy(void* args) {  
    LOG_DEBUG("Destroying...");

    Global::destroy();
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
    LOG_DEBUG("Init...\n");

    Global::init();

    napi_add_env_cleanup_hook((napi_env) env, destroy, nullptr);

    exports["compile"] = Napi::Function::New(env, erynCompile);
    exports["compileDir"] = Napi::Function::New(env, erynCompileDir);
    exports["render"] = Napi::Function::New(env, erynRender);
    exports["setOptions"] = Napi::Function::New(env, erynSetOptions);

    return exports;
}

NODE_API_MODULE(MODULE_NAME, init)