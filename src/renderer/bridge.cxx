#include "bridge.hxx"
#include "../../lib/buffer.hxx"
#include "../def/logging.dxx"
#include "../except/rendering.hxx"

#include <string>

std::string stringify(const Napi::Env& env, const Napi::Object& object) {
    Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
    Napi::Function stringify = env.Global().Get("JSON").As<Napi::Object>().Get("stringify").As<Napi::Function>();

    return stringify.Call(json, { object }).As<Napi::String>().Utf8Value();
}

void evalTemplate(BridgeData data, const uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity) {
    Napi::Value result;

    try {
        result = data.RunScript(std::string(reinterpret_cast<const char*>(templateBytes), templateLength));
    } catch(std::exception &e) {
        throw RenderingException("Template error", e.what(), templateBytes, templateLength);
    }

    if(result.IsUndefined() || result.IsNull())
        return;
    else if(result.IsString()) {
        LOG_DEBUG("    Type: string");

        std::string str = result.As<Napi::String>().Utf8Value();
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        while(outputSize + str.size() > outputCapacity) {
            uint8_t* newOutput = qexpand(output.get(), outputCapacity);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, str.size());
        outputSize += str.size();
    } else if(result.IsObject()) {
        LOG_DEBUG("    Type: object");

        std::string str = stringify(data, result.As<Napi::Object>());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        while(outputSize + str.size() > outputCapacity) {
            uint8_t* newOutput = qexpand(output.get(), outputCapacity);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, str.size());
        outputSize += str.size();
    } else if(result.IsArray()) {
        LOG_DEBUG("    Type: array");

        std::string str = stringify(data, result.ToObject());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        while(outputSize + str.size() > outputCapacity) {
            uint8_t* newOutput = qexpand(output.get(), outputCapacity);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, str.size());
        outputSize += str.size();
    } else if(result.IsNumber()) {
        LOG_DEBUG("    Type: number");

        std::string str = result.ToString().Utf8Value();
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        while(outputSize + str.size() > outputCapacity) {
            uint8_t* newOutput = qexpand(output.get(), outputCapacity);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, str.size());
        outputSize += str.size();
    }else if(result.IsArrayBuffer()) {
        LOG_DEBUG("    Type: array buffer");

        uint8_t* ptr = (uint8_t*) result.As<Napi::ArrayBuffer>().Data();
        size_t length = result.As<Napi::ArrayBuffer>().ByteLength();

        while(outputSize + length > outputCapacity) {
            uint8_t* newOutput = qexpand(output.get(), outputCapacity);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, length);
        outputSize += length;
    } else throw RenderingException("Unsupported template return type", "must be String, Number, Object, Array, ArrayBuffer, null or undefined", templateBytes, templateLength);
}

bool evalConditionalTemplate(BridgeData data, const uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(qfree)*> &output, size_t &outputSize, size_t &outputCapacity) {
    Napi::Value result;

    try {
        result = data.RunScript(std::string(reinterpret_cast<const char*>(templateBytes), templateLength));
    } catch(std::exception &e) {
        throw RenderingException("Conditional template error", e.what(), templateBytes, templateLength);
    }

    return (bool) result.ToBoolean();
}

void evalAssignment(BridgeData data, const std::string &assignment) {
    data.RunScript(assignment);
}

void unassign(BridgeData data, const std::string &assignment, size_t assignmentUnassignIndex) {
    data.RunScript(assignment.substr(0, assignmentUnassignIndex) + "=undefined");
}

size_t getArrayLength(BridgeData data, const uint8_t* arrayBytes, size_t arraySize) {
    Napi::Value result;

    try {
        result = data.RunScript(std::string(reinterpret_cast<const char*>(arrayBytes), arraySize));
    } catch(std::exception &e) {
        throw RenderingException("Loop template error", e.what(), arrayBytes, arraySize);
    }

    if(result.IsArray())
        return result.As<Napi::Array>().Length();
    if(!result.IsObject())
        throw RenderingException("Unsupported loop right operand", "must be Array", arrayBytes, arraySize);

    Napi::Array properties = result.ToObject().GetPropertyNames();

    for(uint32_t i = 0; i < properties.Length(); ++i)
        if(((Napi::Value) properties[i]).As<Napi::String>().Utf8Value() != std::to_string(i))
            throw RenderingException("Unsupported loop right operand", "must be Array", arrayBytes, arraySize);
    return properties.Length();
}

void buildLoopAssignment(BridgeData data, std::string &assignment, size_t &assignmentUpdateIndex, size_t &assignmentUnassignIndex, const uint8_t* iterator, size_t iteratorSize, const uint8_t* array, size_t arraySize) {
    data.RunScript("var " + std::string(reinterpret_cast<const char*>(iterator), iteratorSize));
    
    assignment.reserve(256);
    assignment.append(reinterpret_cast<const char*>(iterator), iteratorSize);
    assignment += "=";

    assignmentUnassignIndex = assignment.size() - 1;

    assignment.append(reinterpret_cast<const char*>(array), arraySize);
    assignment += "[";

    assignmentUpdateIndex = assignment.size();
}

void updateLoopAssignment(std::string &assignment, size_t &arrayIndex) {
    assignment += std::to_string(arrayIndex);
    assignment += "]";
    arrayIndex++;
}

void invalidateLoopAssignment(std::string &assignment, const size_t &assignmentUpdateIndex) {
    assignment.erase(assignmentUpdateIndex, assignment.size() - assignmentUpdateIndex);
}

BridgeBackup backupContext(BridgeData data) {
    try {
        return data.RunScript("context");
    } catch(std::exception&) {
        return Napi::Value();
    }
}

void initContext(BridgeData data, const uint8_t* context, size_t contextSize) {
    try {
        if(contextSize == 0)
            data.RunScript("context=undefined");
        else data.RunScript("context=" + std::string(reinterpret_cast<const char*>(context), contextSize));
    } catch(std::exception &e) {
        throw RenderingException("Component template error", (std::string("context: ") + e.what()).c_str(), context, contextSize);
    }
}

void restoreContext(BridgeData data, BridgeBackup backup) {
    if(backup.IsUndefined())
        data.RunScript("context=undefined");
    else if(backup.IsNull())
        data.RunScript("context=null");
    else data.RunScript("context=" + stringify(data, backup.ToObject()));
}