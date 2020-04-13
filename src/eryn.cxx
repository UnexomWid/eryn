#include "napi.h"
#include <node_api.h>

#include "def/logging.dxx"
#include "global/global.hxx"
#include "compiler/compiler.hxx"
#include "renderer/renderer.hxx"

void bufferFinalizer(Napi::Env env, uint8_t* data) {
    qfree(data);
}

std::string jsonStringify(const Napi::Env& env, const Napi::Object& object) {
    Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
    Napi::Function stringify = env.Global().Get("JSON").As<Napi::Object>().Get("stringify").As<Napi::Function>();

    return stringify.Call(json, { object }).As<Napi::String>().Utf8Value();
}

Napi::Value erynEval(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::String expression = info[0].As<Napi::String>();
    std::string context = jsonStringify(env, info[1].As<Napi::Object>());
    env.RunScript("var context = " + context);
    Napi::Value x = env.RunScript(expression);
    return x;
}

#include "global/cache.hxx"

void erynCompile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const char* wd = info[0].As<Napi::String>().Utf8Value().c_str();
    const char* path = info[1].As<Napi::String>().Utf8Value().c_str();

    try {
        compile(wd, path);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    }
}

Napi::Buffer<uint8_t> erynRender(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const char* path = info[0].As<Napi::String>().Utf8Value().c_str();
    Napi::Object context = info[1].As<Napi::Object>();

    env.RunScript("var context=" + jsonStringify(env, context));

    try {
        BinaryData rendered = render(env, path);
        return Napi::Buffer<uint8_t>::New<decltype(bufferFinalizer)*>(env, (uint8_t*) rendered.data, rendered.size, bufferFinalizer);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
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

    exports.Set(Napi::String::New(env, "eval"),
                Napi::Function::New(env, erynEval));
    exports.Set(Napi::String::New(env, "compile"),
                Napi::Function::New(env, erynCompile));
    exports.Set(Napi::String::New(env, "render"),
                Napi::Function::New(env, erynRender));
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, init)