#ifndef ERYN_BRIDGE_HXX_GUARD
#define ERYN_BRIDGE_HXX_GUARD

#include "napi.h"

#include <cstdint>

typedef Napi::Env BridgeData;

uint8_t* evalTemplate(BridgeData data, uint8_t* templateBytes, size_t templateLength);

#endif