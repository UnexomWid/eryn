#include "napi.h"
#include <node_api.h>

#include <memory>

#include "def/logging.dxx"
#include "def/macro.dxx"

#include "engine/engine.hxx"

#include "../lib/str.hxx"
#include "../lib/path.hxx"
#include "../lib/remem.hxx"

void finalize_buffer(Napi::Env env, uint8_t* data) {
    LOG_DEBUG("Finalizing buffer %p", data);
    REMEM_FREE(data);
}

Eryn::Options update_options(const Eryn::Options& opts, const Napi::Object& data) {
    auto result = opts;
    Napi::Array keys = data.GetPropertyNames();

    for(uint32_t i = 0; i < keys.Length(); ++i) {
        auto& keyVal = static_cast<Napi::Value>(keys[i]);

        if(!keyVal.IsString()) {
            continue;
        }

        auto key = keyVal.As<Napi::String>().Utf8Value();
        const auto& value = data.Get(keyVal);

        #define FLAG_ENTRY(name)                                       \
            if(key == STRINGIFY(name)) {                               \
                if(!value.IsBoolean())                                 \
                    continue;                                          \
                result.flags.##name = value.ToBoolean().Value();       \
            }

        #define TEMPLATE_ENTRY2(name, opt)                             \
            if(key == STRINGIFY(name)) {                               \
                if(!value.IsString()) {                                \
                    continue;                                          \
                }                                                      \
                result.templates.##opt = value.ToString().Utf8Value(); \
            }

        #define TEMPLATE_ENTRY(name) TEMPLATE_ENTRY2(name, name)

        FLAG_ENTRY(bypassCache)
        else FLAG_ENTRY(throwOnEmptyContent)
        else FLAG_ENTRY(throwOnMissingEntry)
        else FLAG_ENTRY(throwOnCompileDirError)
        else FLAG_ENTRY(ignoreBlankPlaintext)
        else FLAG_ENTRY(logRenderTime)
        else FLAG_ENTRY(cloneIterators)
        else FLAG_ENTRY(debugDumpOSH)
        else TEMPLATE_ENTRY2(templateStart, start)
        else TEMPLATE_ENTRY2(templateEnd, end)
        else TEMPLATE_ENTRY(bodyEnd)
        else TEMPLATE_ENTRY(commentStart)
        else TEMPLATE_ENTRY(commentEnd)
        else TEMPLATE_ENTRY(voidStart)
        else TEMPLATE_ENTRY(conditionalStart)
        else TEMPLATE_ENTRY(elseStart)
        else TEMPLATE_ENTRY(elseConditionalStart)
        else TEMPLATE_ENTRY(loopStart)
        else TEMPLATE_ENTRY(loopSeparator)
        else TEMPLATE_ENTRY(loopReverse)
        else TEMPLATE_ENTRY(componentStart)
        else TEMPLATE_ENTRY(componentSeparator)
        else TEMPLATE_ENTRY(componentSelf)
        else if(key == "workingDirectory") {
            if(!value.IsString())
                continue;

            std::string dir = value.ToString().Utf8Value();
            path::normalize(dir);

            result.workingDir = dir;
        } else if(key == "templateEscape") {
            if(!value.IsString() || value.As<Napi::String>().Utf8Value().size() != 1)
                continue;
            result.templates.escape = value.ToString().Utf8Value()[0];
        }

        #undef TEMPLATE_ENTRY
        #undef TEMPLATE_ENTRY2
        #undef FLAG_ENTRY
    }

    return result;
}

Napi::Object get_options(Napi::Env env, const Eryn::Options& opts) {
    auto result = Napi::Object::New(env);

    #define FLAG_ENTRY(name) \
        result[STRINGIFY(name)] = opts.flags.##name

    #define TEMPLATE_ENTRY2(name, opt) \
        result[STRINGIFY(name)] = opts.templates.##opt

    #define TEMPLATE_ENTRY(name) \
        TEMPLATE_ENTRY2(name, name)

    FLAG_ENTRY(bypassCache);
    FLAG_ENTRY(throwOnEmptyContent);
    FLAG_ENTRY(throwOnMissingEntry);
    FLAG_ENTRY(throwOnCompileDirError);
    FLAG_ENTRY(ignoreBlankPlaintext);
    FLAG_ENTRY(logRenderTime);
    FLAG_ENTRY(cloneIterators);
    FLAG_ENTRY(debugDumpOSH);
    TEMPLATE_ENTRY2(templateEscape, escape);
    TEMPLATE_ENTRY2(templateStart, start);
    TEMPLATE_ENTRY2(templateEnd, end);
    TEMPLATE_ENTRY(bodyEnd);
    TEMPLATE_ENTRY(commentStart);
    TEMPLATE_ENTRY(commentEnd);
    TEMPLATE_ENTRY(voidStart);
    TEMPLATE_ENTRY(conditionalStart);
    TEMPLATE_ENTRY(elseStart);
    TEMPLATE_ENTRY(elseConditionalStart);
    TEMPLATE_ENTRY(loopStart);
    TEMPLATE_ENTRY(loopSeparator);
    TEMPLATE_ENTRY(loopReverse);
    TEMPLATE_ENTRY(componentStart);
    TEMPLATE_ENTRY(componentSeparator);
    TEMPLATE_ENTRY(componentSelf);

    result["workingDirectory"] = opts.workingDir;

    return result;
}

class ErynEngine : public Napi::ObjectWrap<ErynEngine> {
    Eryn::Engine engine;

    Napi::Value options       (const Napi::CallbackInfo& info);
    Napi::Value compile       (const Napi::CallbackInfo& info);
    Napi::Value compile_dir   (const Napi::CallbackInfo& info);
    Napi::Value compile_string(const Napi::CallbackInfo& info);
    Napi::Value render        (const Napi::CallbackInfo& info);
    Napi::Value renedr_string (const Napi::CallbackInfo& info);

  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    ErynEngine(const Napi::CallbackInfo& info);
};

Napi::Object ErynEngine::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function fn = DefineClass(env, "ErynEngine", {
        InstanceMethod<&ErynEngine::options>       ("options"),
        InstanceMethod<&ErynEngine::compile>       ("compile"),
        InstanceMethod<&ErynEngine::compile_dir>   ("compileDir"),
        InstanceMethod<&ErynEngine::compile_string>("compileString"),
        InstanceMethod<&ErynEngine::render>        ("render"),
        InstanceMethod<&ErynEngine::renedr_string> ("renderString")
    });

    auto ctor = new("Eryn ctor function reference") Napi::FunctionReference();
    *ctor = Napi::Persistent(fn);
    exports.Set("ErynEngine", fn);

    env.SetInstanceData<Napi::FunctionReference>(ctor);

    return exports;
}

ErynEngine::ErynEngine(const Napi::CallbackInfo& info)
  : Napi::ObjectWrap<ErynEngine>(info) {
    auto env = info.Env();

    engine.opts = update_options({}, info[0].As<Napi::Object>());
}

Napi::Value ErynEngine::options(const Napi::CallbackInfo& info) {
    auto env = info.Env();

    if(info.Length() == 0) {
        return get_options(env, engine.opts);
    } else {
        engine.opts = update_options(engine.opts, info[0].As<Napi::Object>());
    }
}

void erynSetOptions(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::Object options = info[0].As<Napi::Object>();
    Napi::Array keys = options.GetPropertyNames();

    for(uint32_t i = 0; i < keys.Length(); ++i) {
        Napi::Value keyVal = keys[i];

        if(!keyVal.IsString())
            continue;

        std::string key = keyVal.As<Napi::String>().Utf8Value();
        Napi::Value value = options.Get(keys[i]);
    }
}

void erynCompile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    std::string absPath;
    std::string pathString = info[0].As<Napi::String>().Utf8Value();

    if(pathIsRelative(pathString.c_str(), pathString.length())) {
        absPath = Options::getWorkingDirectory();

        if(pathString.size() > 0)
            absPath += ('/' + pathString);
    } else absPath = pathString;

    for(size_t i = 0; i < absPath.size(); ++i)
        if(absPath[i] == '\\')
            absPath[i] = '/';

    try {
        compile(absPath.c_str());

        if(Options::getDebugDumpOSH()) {
            FILE* dump = fopen((absPath + std::string(".osh")).c_str(), "wb");
            fwrite(Global::Cache::getEntry(absPath.c_str()).data, 1, Global::Cache::getEntry(absPath.c_str()).size, dump);
            fclose(dump);
        }
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    }
}

void erynCompileDir(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    std::string absPath;
    std::string pathString = info[0].As<Napi::String>().Utf8Value();

    if(pathIsRelative(pathString.c_str(), pathString.length())) {
        absPath = Options::getWorkingDirectory();

        if(pathString.size() > 0)
            absPath += ('/' + pathString);
    } else absPath = pathString;

    for(size_t i = 0; i < absPath.size(); ++i)
        if(absPath[i] == '\\')
            absPath[i] = '/';

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
        compileDir(absPath.c_str(), filters);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    }
}

void erynCompileString(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    try {
        std::unique_ptr<char, decltype(re::free)*> alias(
            strDup(info[0].As<Napi::String>().Utf8Value().c_str()), re::free);

        std::unique_ptr<char, decltype(re::free)*> str(
            strDup(info[1].As<Napi::String>().Utf8Value().c_str()), re::free);

        compileString(alias.get(), str.get());
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    }
}

Napi::Buffer<uint8_t> erynRender(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    std::string absPath;
    std::string pathString = info[0].As<Napi::String>().Utf8Value();

    if(pathIsRelative(pathString.c_str(), pathString.length())) {
        absPath = Options::getWorkingDirectory();

        if(pathString.size() > 0)
            absPath += ('/' + pathString);
    } else absPath = pathString;

    for(size_t i = 0; i < absPath.size(); ++i)
        if(absPath[i] == '\\')
            absPath[i] = '/';

    try {
        BinaryData rendered = render(BridgeData(env, info[1].As<Napi::Value>(), info[2].As<Napi::Object>(), info[3].As<Napi::Value>(), info[4].As<Napi::Function>(), info[5].As<Napi::Function>()), absPath.c_str());

        return Napi::Buffer<uint8_t>::New<decltype(finalizeBuffer)*>(
                   env, (uint8_t*) rendered.data, rendered.size, finalizeBuffer);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, ((std::string("Rendering error in '") + absPath.c_str()) + "'\n") + e.what());
    }
}

Napi::Buffer<uint8_t> erynRenderString(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    std::unique_ptr<char, decltype(re::free)*> alias(
            strDup(info[0].As<Napi::String>().Utf8Value().c_str()), re::free);

    try {
        BinaryData rendered = renderString(BridgeData(env, info[1].As<Napi::Value>(), info[2].As<Napi::Object>(), info[3].As<Napi::Value>(), info[4].As<Napi::Function>(), info[5].As<Napi::Function>()), alias.get());

        return Napi::Buffer<uint8_t>::New<decltype(finalizeBuffer)*>(
                   env, (uint8_t*) rendered.data, rendered.size, finalizeBuffer);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, ((std::string("Rendering error in '") + alias.get()) + "'\n") + e.what());
    }
}

void destroy(void* args) {
    LOG_DEBUG("Destroying...");

    #ifdef REMEM_ENABLE_MAPPING
        re::memPrint();
    #endif
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
    LOG_DEBUG("Init...\n");

    ErynEngine::Init(env, exports);

    napi_add_env_cleanup_hook((napi_env) env, destroy, nullptr);

    return exports;
}

NODE_API_MODULE(MODULE_NAME, init)