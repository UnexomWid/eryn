#define NAPI_VERSION 5

#include "napi.h"
#include <node_api.h>

#include "global/global.hxx"
#include "compiler/compiler.hxx"

Napi::String JSONStringify(const Napi::Env& env, const Napi::Object& object) {
    Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();

    Napi::Function stringify = env.Global().Get("JSON").As<Napi::Object>().Get("stringify").As<Napi::Function>();
    return stringify.Call(json, { object }).As<Napi::String>();
}

Napi::Value Eval(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::String expression = info[0].As<Napi::String>();
    Napi::String context = JSONStringify(env, info[1].As<Napi::Object>());
    env.RunScript("var context = " + context.Utf8Value());
    Napi::Value x = env.RunScript(expression);
    return x;
}

void Compile(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const char* wd = info[0].As<Napi::String>().Utf8Value().c_str();
    const char* path = info[1].As<Napi::String>().Utf8Value().c_str();
    const char* outputPath = info[2].As<Napi::String>().Utf8Value().c_str();

    try {
        compileFile(wd, path, outputPath);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    }
}

void Destroy(void* args) {
    Global::destroy();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    printf("[eryn] Init\n");
    Global::init();

    napi_add_env_cleanup_hook((napi_env) env, Destroy, nullptr);

    exports.Set(Napi::String::New(env, "eval"),
                Napi::Function::New(env, Eval));
    exports.Set(Napi::String::New(env, "compile"),
                Napi::Function::New(env, Compile));
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)