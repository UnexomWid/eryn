#ifndef ERYN_BRIDGE_HXX_GUARD
#define ERYN_BRIDGE_HXX_GUARD

#include <string>
#include <memory>
#include <cstdint>

#include "napi.h"
#include <node_api.h>

#include "../def/osh.dxx"
#include "../def/warnings.dxx"
#include "../../lib/buffer.hxx"
#include "../../lib/remem.hxx"

struct BridgeData {
    Napi::Env env;
    Napi::Function eval;
    Napi::Function clone;
    Napi::Object context;
    Napi::Object local;
    Napi::Object shared;

    BridgeData(Napi::Env en, Napi::Object ctx, Napi::Object lcl, Napi::Object shrd, Napi::Function ev, Napi::Function cln) : env(en), context(ctx), local(lcl), shared(shrd), eval(ev), clone(cln) { }
};

typedef Napi::Value BridgeBackup;
typedef Napi::Array BridgeArray;

void evalTemplate(BridgeData& data, const uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(re::free)*>& output, size_t& outputSize, size_t& outputCapacity);
void evalVoidTemplate(BridgeData& data, const uint8_t* templateBytes, size_t templateLength);
bool evalConditionalTemplate(BridgeData& data, const uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(re::free)*>& output, size_t& outputSize, size_t& outputCapacity);
void evalAssignment(BridgeData& data, const std::string& iterator, const std::string& assignment, const std::string& propertyAssignment);
void unassign(BridgeData& data, const std::string& iterator);

size_t initArray(BridgeData& data, const uint8_t* arrayBytes, size_t arraySize, std::string*& propertyArray, int8_t direction);

void buildLoopAssignment(BridgeData& data, std::string& iterator, std::string& assignment, size_t& assignmentUpdateIndex, const uint8_t* it, size_t itSize, const uint8_t* array, size_t arraySize);
void updateLoopAssignment(std::string& assignment, std::string& propertyAssignment, size_t& arrayIndex, std::string*& propertyArray, int8_t direction);
void invalidateLoopAssignment(std::string& assignment, const size_t& assignmentUpdateIndex);

BridgeBackup copyValue(BridgeData& data, const Napi::Value& value);

BridgeBackup backupContext(BridgeData& data);
void initContext(BridgeData& data, const uint8_t* context, size_t contextSize);
void restoreContext(BridgeData& data, BridgeBackup backup);

BridgeBackup backupLocal(BridgeData& data);
void initLocal(BridgeData& data);
void restoreLocal(BridgeData& data, BridgeBackup backup);

#endif