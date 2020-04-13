#define NAPI_VERSION 5

#include "napi.h"
#include <node_api.h>
#include <cstdio>

#include "global/global.hxx"
#include "compiler/compiler.hxx"
#include "renderer/renderer.hxx"

std::string stringify(const Napi::Env& env, const Napi::Object& object) {
    Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
    Napi::Function stringify = env.Global().Get("JSON").As<Napi::Object>().Get("stringify").As<Napi::Function>();

    return stringify.Call(json, { object }).As<Napi::String>().Utf8Value();
}

Napi::Value eval(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    Napi::String expression = info[0].As<Napi::String>();
    std::string context = stringify(env, info[1].As<Napi::Object>());
    env.RunScript("var context = " + context);
    Napi::Value x = env.RunScript(expression);
    return x;
}

void compile(const Napi::CallbackInfo& info) {
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

void render(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    const char* path = info[0].As<Napi::String>().Utf8Value().c_str();
    const char* outputPath = info[1].As<Napi::String>().Utf8Value().c_str();
    Napi::Object context = info[2].As<Napi::Object>();

    env.RunScript("var context=" + stringify(env, context));

    try {
        renderFile(env, path, outputPath);
    } catch(std::exception &e) {
        throw Napi::Error::New(env, e.what());
    }
}

void destroy(void* args) {
    Global::destroy();
}

Napi::Object init(Napi::Env env, Napi::Object exports) {
    printf("[eryn] Init\n");
    Global::init();

    napi_add_env_cleanup_hook((napi_env) env, destroy, nullptr);

    exports.Set(Napi::String::New(env, "eval"),
                Napi::Function::New(env, eval));
    exports.Set(Napi::String::New(env, "compileFile"),
                Napi::Function::New(env, compile));
    exports.Set(Napi::String::New(env, "renderFile"),
                Napi::Function::New(env, render));
    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, init)