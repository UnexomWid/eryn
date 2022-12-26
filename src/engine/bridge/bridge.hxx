#ifndef ERYN_BRIDGE_HXX_GUARD
#define ERYN_BRIDGE_HXX_GUARD

#include "napi.h"
#include <node_api.h>

#include <variant>

#include "../../def/warnings.dxx"
#include "../../../lib/buffer.hxx"

namespace Eryn {
typedef Napi::FunctionReference  BridgeHook;
typedef Napi::Buffer<uint8_t>    BridgeHookResult;
typedef Napi::Value              BridgeBackup;
typedef Napi::Array              BridgeArray;
typedef Napi::Object             BridgeObject;
typedef Napi::Object             BridgeIterable;
typedef std::vector<Napi::Value> BridgeObjectKeys;

// Contains data necessary for the bridge, such as the context and local objects.
// Also includes references to needed functions such as eval.
struct BridgeRenderData {
    Napi::Env      env;
    Napi::Function eval;
    Napi::Function clone;
    Napi::Value    context;
    Napi::Object   local;
    Napi::Value    shared;

    BridgeRenderData(Napi::Env env, Napi::Value context, Napi::Object local, Napi::Value shared, Napi::Function eval, Napi::Function clone) :
        env(env),
        context(context),
        local(local),
        shared(shared),
        eval(eval),
        clone(clone) {
    }
};

struct BridgeCompileData {
    public:
    Napi::Env env;

    BridgeCompileData(Napi::Env env) : env(env) {
    }
};

class Bridge {
    protected:
    BridgeRenderData data;

    public:
    Bridge(BridgeRenderData&& data) : data(std::forward<BridgeRenderData>(data)) {
    }

    // TODO: do this better
    // Currently, the renderer holds BridgeRenderData, and has to pass the Napi::Env to the compiler.
    // So, use this function for that.
    BridgeCompileData to_compile_data();

    // The buffer passed to the hook, and will be overwritten with the hook result
    // (only if the result is a Buffer or String)
    // Origin - where the hook was called from (normal template, conditional template, component path, component context, etc)
    static bool call_hook(BridgeCompileData data, BridgeHook& hook, Buffer& input, const char* origin);

// Declare all bridge methods as pure virtual.
// See the bridge_methods.dxx file for the declarations.
#define BRIDGE_METHOD(decl) virtual decl = 0
#include "bridge_methods.dxx"
#undef BRIDGE_METHOD
};

class NormalBridge : public Bridge {
    public:
    NormalBridge(BridgeRenderData&& data);

// Declare all bridge methods with override.
#define BRIDGE_METHOD(decl) decl override
#include "bridge_methods.dxx"
#undef BRIDGE_METHOD
};

class StrictBridge : public Bridge {
    public:
    StrictBridge(BridgeRenderData&& data);

// Declare all bridge methods with override.
#define BRIDGE_METHOD(decl) decl override
#include "bridge_methods.dxx"
#undef BRIDGE_METHOD
};
} // namespace Eryn

#endif