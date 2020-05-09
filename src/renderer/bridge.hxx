#ifndef ERYN_BRIDGE_HXX_GUARD
#define ERYN_BRIDGE_HXX_GUARD

#include "napi.h"
#include <node_api.h>

#include "../def/osh.dxx"
#include "../def/warnings.dxx"

#include "../../lib/buffer.hxx"

#include <memory>
#include <cstdint>

struct BridgeData {
    Napi::Env env;
    Napi::Function eval;
    Napi::Object context;
    Napi::Object local;

    BridgeData(Napi::Env en, Napi::Object ctx, Napi::Object lcl, Napi::Function ev) : env(en), context(ctx), local(lcl), eval(ev) { }
};
typedef Napi::Value BridgeBackup;

void evalTemplate(BridgeData& data, const uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity);
bool evalConditionalTemplate(BridgeData& data, const uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity);
void evalAssignment(BridgeData& data, const std::string& iterator, const std::string& assignment);
void unassign(BridgeData& data, const std::string &iterator);

size_t getArrayLength(BridgeData& data, const uint8_t* arrayBytes, size_t arraySize);

void buildLoopAssignment(BridgeData& data, std::string& iterator, std::string& assignment, size_t &assignmentUpdateIndex, const uint8_t* it, size_t itSize, const uint8_t* array, size_t arraySize);
void updateLoopAssignment(std::string &assignment, size_t &arrayIndex);
void invalidateLoopAssignment(std::string &assignment, const size_t &assignmentUpdateIndex);

BridgeBackup copyValue(BridgeData& data, const Napi::Value& value);

BridgeBackup backupContext(BridgeData& data);
void initContext(BridgeData& data, const uint8_t* context, size_t contextSize);
void restoreContext(BridgeData& data, BridgeBackup backup);

BridgeBackup backupLocal(BridgeData& data);
void initLocal(BridgeData& data);
void restoreLocal(BridgeData& data, BridgeBackup backup);

#endif