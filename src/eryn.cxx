#include "napi.h"
#include <node_api.h>

#include <memory>
#include <cctype>

#include "def/logging.dxx"
#include "def/macro.dxx"

#include "engine/engine.hxx"

#include "engine/bridge/bridge.hxx"

#include "../lib/str.hxx"
#include "../lib/path.hxx"
#include "../lib/remem.hxx"

void finalize_buffer(Napi::Env, uint8_t* data) {
    LOG_DEBUG("Finalizing buffer %p", data);
    REMEM_FREE(data);
}

void update_options(Eryn::Options& opts, const Napi::Object& data) {
    auto&       result = opts;
    Napi::Array keys   = data.GetPropertyNames();

    for (uint32_t i = 0; i < keys.Length(); ++i) {
        auto keyVal = static_cast<Napi::Value>(keys[i]);

        if (!keyVal.IsString()) {
            continue;
        }

        auto key   = keyVal.As<Napi::String>().Utf8Value();
        auto value = data.Get(keyVal);

#define FLAG_ENTRY(name)                                                                                                                             \
    if (key == STRINGIFY(name)) {                                                                                                                    \
        if (!value.IsBoolean()) {                                                                                                                    \
            continue;                                                                                                                                \
        }                                                                                                                                            \
        result.flags.name = value.ToBoolean().Value();                                                                                               \
    }

#define TEMPLATE_ENTRY2(name, opt)                                                                                                                   \
    if (key == STRINGIFY(name)) {                                                                                                                    \
        if (!value.IsString()) {                                                                                                                     \
            continue;                                                                                                                                \
        }                                                                                                                                            \
        result.templates.opt = value.ToString().Utf8Value();                                                                                         \
    }

// clang-format off
        #define TEMPLATE_ENTRY(name) TEMPLATE_ENTRY2(name, name)

        FLAG_ENTRY(bypassCache)
        else FLAG_ENTRY(throwOnEmptyContent)
        else FLAG_ENTRY(throwOnMissingEntry)
        else FLAG_ENTRY(throwOnCompileDirError)
        else FLAG_ENTRY(ignoreBlankPlaintext)
        else FLAG_ENTRY(logRenderTime)
        else FLAG_ENTRY(cloneIterators)
        else FLAG_ENTRY(cloneBackups)
        else FLAG_ENTRY(cloneLocalInLoops)
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
        else if (key == "workingDirectory") {
            if (!value.IsString()) {
                continue;
            }

            std::string dir = value.ToString().Utf8Value();
            path::normalize(dir);

            result.workingDir = dir;
        } else if (key == "templateEscape") {
            if (!value.IsString() || value.As<Napi::String>().Utf8Value().size() != 1) {
                continue;
            }

            result.templates.escape = value.ToString().Utf8Value()[0];
        } else if (key == "mode") {
            if (!value.IsString()) {
                continue;
            }

            auto mode = value.ToString().Utf8Value();

            for (auto& c : mode) {
                c = std::tolower(c);
            }

            if (mode == "normal") {
                result.mode = Eryn::EngineMode::NORMAL;
            } else if (mode == "strict") {
                result.mode = Eryn::EngineMode::STRICT;
            }
        }  else if (key == "compileHook") {
            if (!value.IsFunction()) {
                continue;
            }

            result.compileHook = Napi::Persistent(value.As<Napi::Function>());
        }
        // clang-format on

#undef TEMPLATE_ENTRY
#undef TEMPLATE_ENTRY2
#undef FLAG_ENTRY
    }
}

Napi::Object get_options(Napi::Env env, const Eryn::Options& opts) {
    auto result = Napi::Object::New(env);

#define FLAG_ENTRY(name) result[STRINGIFY(name)] = opts.flags.name

#define TEMPLATE_ENTRY2(name, opt) result[STRINGIFY(name)] = opts.templates.opt

#define TEMPLATE_ENTRY(name) TEMPLATE_ENTRY2(name, name)

    FLAG_ENTRY(bypassCache);
    FLAG_ENTRY(throwOnEmptyContent);
    FLAG_ENTRY(throwOnMissingEntry);
    FLAG_ENTRY(throwOnCompileDirError);
    FLAG_ENTRY(ignoreBlankPlaintext);
    FLAG_ENTRY(logRenderTime);
    FLAG_ENTRY(cloneIterators);
    FLAG_ENTRY(cloneBackups);
    FLAG_ENTRY(cloneLocalInLoops);
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
    result["mode"]             = (opts.mode == Eryn::EngineMode::NORMAL) ? "normal" : "strict";

    return result;
}

class ErynEngine : public Napi::ObjectWrap<ErynEngine> {
    Eryn::Engine engine;

    Napi::Value options(const Napi::CallbackInfo& info);
    Napi::Value compile(const Napi::CallbackInfo& info);
    Napi::Value compile_dir(const Napi::CallbackInfo& info);
    Napi::Value compile_string(const Napi::CallbackInfo& info);
    Napi::Value render(const Napi::CallbackInfo& info);
    Napi::Value render_string(const Napi::CallbackInfo& info);

    public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    ErynEngine(const Napi::CallbackInfo& info);
};

Napi::Object ErynEngine::Init(Napi::Env env, Napi::Object exports) {
    Napi::Function fn = DefineClass(env, "ErynEngine",
                                    { InstanceMethod<&ErynEngine::options>("options"), InstanceMethod<&ErynEngine::compile>("compile"),
                                      InstanceMethod<&ErynEngine::compile_dir>("compileDir"), InstanceMethod<&ErynEngine::compile_string>("compileString"),
                                      InstanceMethod<&ErynEngine::render>("render"), InstanceMethod<&ErynEngine::render_string>("renderString") });

    auto ctor = new("Eryn ctor function reference") Napi::FunctionReference();
    *ctor     = Napi::Persistent(fn);
    exports.Set("ErynEngine", fn);

    env.SetInstanceData<Napi::FunctionReference>(ctor);

    return exports;
}

ErynEngine::ErynEngine(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ErynEngine>(info) {
    auto env = info.Env();

    if (info.Length() > 0) {
        update_options(engine.opts, info[0].As<Napi::Object>());
    }
}

Napi::Value ErynEngine::options(const Napi::CallbackInfo& info) {
    auto env = info.Env();

    if (info.Length() == 0) {
        return get_options(env, engine.opts);
    } else {
        update_options(engine.opts, info[0].As<Napi::Object>());
        return get_options(env, engine.opts);
    }
}

Napi::Value ErynEngine::compile(const Napi::CallbackInfo& info) {
    auto env = info.Env();

    auto pathString = info[0].As<Napi::String>().Utf8Value();
    auto absPath    = path::append_or_absolute(engine.opts.workingDir, pathString);
    path::normalize(absPath);

    try {
        Eryn::BridgeCompileData bridge(env);

        engine.compile(bridge, absPath.c_str());

        if (engine.opts.flags.debugDumpOSH) {
            FILE* dump = fopen((absPath + std::string(".osh")).c_str(), "wb");

            auto& entry = engine.cache.get(absPath);

            fwrite(entry.data, sizeof(uint8_t), entry.size, dump);
            fclose(dump);
        }

        return env.Undefined();
    } catch (std::exception& e) {
        throw Napi::Error::New(env, e.what());
    }
}

Napi::Value ErynEngine::compile_dir(const Napi::CallbackInfo& info) {
    auto env = info.Env();

    auto pathString = info[0].As<Napi::String>().Utf8Value();
    auto absPath    = path::append_or_absolute(engine.opts.workingDir, pathString);
    path::normalize(absPath);

    std::vector<std::string> filters;
    auto                     filterArray = info[1].As<Napi::Array>();

    uint32_t length = filterArray.Length();

    for (uint32_t i = 0; i < length; ++i) {
        Napi::Value item = filterArray[i];

        if (!item.IsString()) {
            throw Napi::Error::New(env, "Invalid filter array (expected array of strings)");
        }

        filters.push_back(item.As<Napi::String>().Utf8Value());
    }

    try {
        Eryn::BridgeCompileData bridge(env);
        engine.compile_dir(bridge, absPath.c_str(), filters);

        return env.Undefined();
    } catch (std::exception& e) {
        throw Napi::Error::New(env, e.what());
    }
}

Napi::Value ErynEngine::compile_string(const Napi::CallbackInfo& info) {
    auto env = info.Env();

    try {
        Eryn::BridgeCompileData bridge(env);

        auto alias = info[0].As<Napi::String>().Utf8Value();
        auto str   = info[1].As<Napi::String>().Utf8Value();

        engine.compile_string(bridge, alias.c_str(), str.c_str());

        return env.Undefined();
    } catch (std::exception& e) {
        throw Napi::Error::New(env, e.what());
    }

    return env.Undefined();
}

Napi::Value ErynEngine::render(const Napi::CallbackInfo& info) {
    auto env = info.Env();

    auto pathString = info[0].As<Napi::String>().Utf8Value();
    auto absPath    = path::append_or_absolute(engine.opts.workingDir, pathString);
    path::normalize(absPath);

    try {
        ConstBuffer rendered;

        if (engine.opts.mode == Eryn::EngineMode::NORMAL) {
            Eryn::NormalBridge bridge({ env, info[1].As<Napi::Value>(), info[2].As<Napi::Object>(), info[3].As<Napi::Value>(),
                                        info[4].As<Napi::Function>(), info[5].As<Napi::Function>() });

            rendered = engine.render(bridge, absPath.c_str());
        } else {
            Eryn::StrictBridge bridge({ env, info[1].As<Napi::Value>(), info[2].As<Napi::Object>(), info[3].As<Napi::Value>(),
                                        info[4].As<Napi::Function>(), info[5].As<Napi::Function>() });

            rendered = engine.render(bridge, absPath.c_str());
        }

        return Napi::Buffer<uint8_t>::New<decltype(finalize_buffer)*>(env, (uint8_t*) rendered.data, rendered.size, finalize_buffer);
    } catch (std::exception& e) {
        // TODO: remove the path from RenderingException
        throw Napi::Error::New(env, ((std::string("Rendering error in '") + absPath.c_str()) + "'\n") + e.what());
    }
}

Napi::Value ErynEngine::render_string(const Napi::CallbackInfo& info) {
    auto env   = info.Env();
    auto alias = info[0].As<Napi::String>().Utf8Value();

    try {
        Eryn::NormalBridge bridge({ env, info[1].As<Napi::Value>(), info[2].As<Napi::Object>(), info[3].As<Napi::Value>(),
                                    info[4].As<Napi::Function>(), info[5].As<Napi::Function>() });

        auto rendered = engine.render_string(bridge, alias.c_str());

        return Napi::Buffer<uint8_t>::New<decltype(finalize_buffer)*>(env, (uint8_t*) rendered.data, rendered.size, finalize_buffer);
    } catch (std::exception& e) {
        // TODO: remove the path from RenderingException
        throw Napi::Error::New(env, ((std::string("Rendering error in '") + alias.c_str()) + "'\n") + e.what());
    }
}

void destroy(void*) {
    LOG_DEBUG("Destroying...");

#ifdef REMEM_ENABLE_MAPPING
    re::mem_print();
#endif
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
    LOG_DEBUG("Init...\n");

    ErynEngine::Init(env, exports);

    napi_add_env_cleanup_hook((napi_env) env, destroy, nullptr);

    return exports;
}

NODE_API_MODULE(MODULE_NAME, init)