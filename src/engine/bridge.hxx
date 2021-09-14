#ifndef ERYN_BRIDGE_HXX_GUARD
#define ERYN_BRIDGE_HXX_GUARD

#include "napi.h"
#include <node_api.h>

#include "../def/warnings.dxx"
#include "../../lib/buffer.hxx"

namespace Eryn {
typedef Napi::Value BridgeBackup;
typedef Napi::Array BridgeArray;

struct BridgeData {
    Napi::Env env;
    Napi::Function eval;
    Napi::Function clone;
    Napi::Value  context;
    Napi::Object local;
    Napi::Value  shared;

    BridgeData(Napi::Env env, Napi::Value context, Napi::Object local, Napi::Value shared, Napi::Function eval, Napi::Function clone);
};

struct Bridge {
    BridgeData data;

    Bridge(BridgeData&& data);

    void evalTemplate(ConstBuffer input, Buffer& output);
    void evalVoidTemplate(ConstBuffer input);
    bool evalConditionalTemplate(ConstBuffer input);
    void evalAssignment(bool cloneIterators, const std::string& iterator, const std::string& assignment, const std::string& propertyAssignment);
    void unassign(const std::string& iterator);

    size_t initArray(ConstBuffer array, std::string*& propertyArray, int8_t direction);

    void buildLoopAssignment(std::string& iterator, std::string& assignment, size_t& assignmentUpdateIndex, ConstBuffer it, ConstBuffer array);
    void updateLoopAssignment(std::string& assignment, std::string& propertyAssignment, size_t& arrayIndex, std::string*& propertyArray, int8_t direction);
    void invalidateLoopAssignment(std::string& assignment, const size_t& assignmentUpdateIndex);

    BridgeBackup copyValue(const Napi::Value& value);

    BridgeBackup backupContext(bool cloneBackup);
    void initContext(ConstBuffer context);
    void restoreContext(BridgeBackup backup);

    BridgeBackup backupLocal(bool cloneBackup);
    void initLocal();
    void restoreLocal(BridgeBackup backup);
};
}

#endif