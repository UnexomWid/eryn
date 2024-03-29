#include <initializer_list>

#include "bridge.hxx"

#include "../../def/logging.dxx"

Eryn::BridgeCompileData Eryn::Bridge::to_compile_data() {
    return Eryn::BridgeCompileData(data.env);
}

bool Eryn::Bridge::call_hook(BridgeCompileData data, BridgeHook& hook, Buffer& input, const char* origin) {
    auto buff = Napi::Buffer<uint8_t>::New(data.env, (uint8_t*) input.data, input.size);
    auto originStr = Napi::String::New(data.env, origin);

    auto result = hook.Call(std::initializer_list<napi_value>({ buff, originStr }));

    if (result.IsString()) {
        LOG_DEBUG("Hook result is string");

        auto str = result.As<Napi::String>().Utf8Value();
        auto ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        input.clear();
        input.write(ptr, str.size());

        return true;
    } else if (result.IsBuffer()) {
        LOG_DEBUG("Hook result is buffer");

        auto ptr    = reinterpret_cast<uint8_t*>(result.As<Napi::Buffer<char>>().Data());
        auto length = result.As<Napi::Buffer<char>>().Length();

        input.clear();
        input.write(ptr, length);

        return true;
    } else if (!result.IsUndefined() && !result.IsNull()) {
        LOG_DEBUG("Hook result is invalid");
        return false;
    }

    LOG_DEBUG("Hook result is undefined or null");

    return true;
}