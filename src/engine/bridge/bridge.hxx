#ifndef ERYN_BRIDGE_HXX_GUARD
#define ERYN_BRIDGE_HXX_GUARD

#include "napi.h"
#include <node_api.h>

#include <variant>

#include "../../def/warnings.dxx"
#include "../../../lib/buffer.hxx"

namespace Eryn {
typedef Napi::Value  BridgeBackup;
typedef Napi::Array  BridgeArray;
typedef Napi::Object BridgeObject;
typedef Napi::Object BridgeIterable;
typedef std::vector<Napi::Value> BridgeObjectKeys;

// Contains data necessary for the bridge, such as the context and local objects.
// Also includes references to needed functions such as eval.
struct BridgeData {
    Napi::Env      env;
    Napi::Function eval;
    Napi::Function clone;
    Napi::Value    context;
    Napi::Object   local;
    Napi::Value    shared;

    BridgeData(Napi::Env env, Napi::Value context, Napi::Object local, Napi::Value shared, Napi::Function eval, Napi::Function clone)
        : env(env), context(context), local(local), shared(shared), eval(eval), clone(clone) { }
};

class Bridge {
  protected:
    BridgeData data;

  public:
    Bridge(BridgeData&& data) : data(std::forward<BridgeData>(data)) {}

    // Declare all bridge methods as pure virtual.
    // See the bridge_methods.dxx file for the declarations.
    #define BRIDGE_METHOD(decl) virtual decl = 0
        #include "bridge_methods.dxx"
    #undef BRIDGE_METHOD
};

class NormalBridge : public Bridge {
  public:
    NormalBridge(BridgeData&& data);

    // Declare all bridge methods with override.
    #define BRIDGE_METHOD(decl) decl override
        #include "bridge_methods.dxx"
    #undef BRIDGE_METHOD
};

class StrictBridge : public Bridge {
  public:
    StrictBridge(BridgeData&& data);

    // Declare all bridge methods with override.
    #define BRIDGE_METHOD(decl) decl override
        #include "bridge_methods.dxx"
    #undef BRIDGE_METHOD
};
}

#endif