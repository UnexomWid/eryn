#ifndef ERYN_BRIDGE_HXX_GUARD
#define ERYN_BRIDGE_HXX_GUARD

#include "napi.h"
#include "../../lib/buffer.hxx"

#include <memory>
#include <cstdint>

typedef Napi::Env BridgeData;
typedef Napi::Value BridgeBackup;

void evalTemplate(BridgeData data, uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity);
bool evalConditionalTemplate(BridgeData data, uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity);
void evalAssignment(BridgeData data, const std::string &assignment);
void unassign(BridgeData data, const std::string &assignment, size_t assignmentUnassignIndex);

size_t getArrayLength(BridgeData data, uint8_t* arrayBytes, size_t arraySize);

void buildLoopAssignment(BridgeData data, std::string &assignment, size_t &assignmentUpdateIndex, size_t &assignmentUnassignIndex, uint8_t* iterator, size_t iteratorSize, uint8_t* array, size_t arraySize);
void updateLoopAssignment(std::string &assignment, size_t &arrayIndex);
void invalidateLoopAssignment(std::string &assignment, const size_t &assignmentUpdateIndex);

BridgeBackup backupContext(BridgeData data);
BridgeBackup backupContent(BridgeData data);

void initContext(BridgeData data, uint8_t* context, size_t contextSize);
void initContent(BridgeData data, uint8_t* content, size_t contentSize);

void restoreContext(BridgeData data, BridgeBackup backup);
void restoreContent(BridgeData data, BridgeBackup backup);

#endif