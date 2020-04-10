#ifndef ERYN_BRIDGE_HXX_GUARD
#define ERYN_BRIDGE_HXX_GUARD

#include "napi.h"
#include "../../lib/buffer.hxx"

#include <memory>
#include <cstdint>

typedef Napi::Env BridgeData;

void evalTemplate(BridgeData data, uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity);
bool evalConditionalTemplate(BridgeData data, uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity);

#endif