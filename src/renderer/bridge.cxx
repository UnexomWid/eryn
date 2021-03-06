#include "bridge.hxx"

#include "../def/logging.dxx"
#include "../except/rendering.hxx"
#include "../global/options.hxx"

std::string stringify(const Napi::Env& env, const Napi::Object& object) {
    Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
    Napi::Function stringify = env.Global().Get("JSON").As<Napi::Object>().Get("stringify").As<Napi::Function>();

    return stringify.Call(json, { object }).As<Napi::String>().Utf8Value();
}

void evalTemplate(BridgeData& data, const uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(re::free)*>& output, size_t& outputSize, size_t& outputCapacity) {
    Napi::Value result;

    try {
        result = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, reinterpret_cast<const char*>(templateBytes), templateLength), data.context, data.local, data.shared }));
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
            uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, str.size());
        outputSize += str.size();
    } else if(result.IsBuffer()) {
        LOG_DEBUG("    Type: buffer");

        uint8_t* ptr = reinterpret_cast<uint8_t*>(result.As<Napi::Buffer<char>>().Data());
        size_t length = result.As<Napi::Buffer<char>>().Length();

        while(outputSize + length > outputCapacity) {
            uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, length);
        outputSize += length;
    } else if(result.IsObject()) {
        LOG_DEBUG("    Type: object");

        std::string str = stringify(data.env, result.As<Napi::Object>());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        while(outputSize + str.size() > outputCapacity) {
            uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, str.size());
        outputSize += str.size();
    } else if(result.IsArray()) {
        LOG_DEBUG("    Type: array");

        std::string str = stringify(data.env, result.ToObject());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        while(outputSize + str.size() > outputCapacity) {
            uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
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
            uint8_t* newOutput = (uint8_t*) re::expand(output.get(), outputCapacity, __FILE__, __LINE__);
            output.release();
            output.reset(newOutput);
        }

        memcpy(output.get() + outputSize, ptr, str.size());
        outputSize += str.size();
    } else throw RenderingException("Unsupported template return type", "must be String, Number, Object, Array, Buffer, null or undefined", templateBytes, templateLength);
}

void evalVoidTemplate(BridgeData& data, const uint8_t* templateBytes, size_t templateLength) {
    try {
        data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, reinterpret_cast<const char*>(templateBytes), templateLength), data.context, data.local, data.shared }));
    } catch(std::exception &e) {
        throw RenderingException("Void template error", e.what(), templateBytes, templateLength);
    }
}

bool evalConditionalTemplate(BridgeData& data, const uint8_t* templateBytes, size_t templateLength, std::unique_ptr<uint8_t, decltype(re::free)*>& output, size_t& outputSize, size_t& outputCapacity) {
    Napi::Value result;

    try {
        result = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, reinterpret_cast<const char*>(templateBytes), templateLength), data.context, data.local, data.shared }));
    } catch(std::exception &e) {
        throw RenderingException("Conditional template error", e.what(), templateBytes, templateLength);
    }

    return result.ToBoolean().Value();
}

void evalAssignment(BridgeData& data, const std::string& iterator, const std::string& assignment, const std::string& propertyAssignment) {
    if(Global::Options::getCloneIterators()) {
        if(propertyAssignment.size() > 0) {
            data.local[iterator] = data.clone.Call(std::initializer_list<napi_value>({
                                       data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, propertyAssignment + assignment + "})"), data.context, data.local, data.shared }))
                                   })).ToObject();
        } else {
            data.local[iterator] = data.clone.Call(std::initializer_list<napi_value>({
                                       data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, assignment), data.context, data.local, data.shared }))
                                   }));
        }
    } else {
        if(propertyAssignment.size() > 0)
            data.local[iterator] = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, propertyAssignment + assignment + "})"), data.context, data.local, data.shared })).ToObject();
        else data.local[iterator] = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, assignment), data.context, data.local, data.shared }));
    }
    
}

void unassign(BridgeData& data, const std::string &iterator) {
    data.local[iterator] = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, "undefined"), data.context, data.local, data.shared }));
}

size_t initArray(BridgeData& data, const uint8_t* arrayBytes, size_t arraySize, std::string*& propertyArray, int8_t direction) {
    Napi::Value result;

    try {
        result = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, std::string(reinterpret_cast<const char*>(arrayBytes), arraySize)), data.context, data.local, data.shared }));
    } catch(std::exception &e) {
        throw RenderingException("Loop template error", e.what(), arrayBytes, arraySize);
    }

    bool isSimpleArray = true;

    if(result.IsArray())
        return result.As<Napi::Array>().Length();
    if(!result.IsObject())
        throw RenderingException("Unsupported loop right operand", "must be Array", arrayBytes, arraySize);

    Napi::Array properties = result.ToObject().GetPropertyNames();

    if(properties.Length() == 0)
        throw RenderingException("Unsupported loop right operand", "length is 0", arrayBytes, arraySize);

    propertyArray = new std::string[properties.Length()];

    if(direction > 0) {
        for(uint32_t i = 0; i < properties.Length(); ++i) {
            propertyArray[i] = ((Napi::Value) properties[i]).As<Napi::String>().Utf8Value();

            if(((Napi::Value) properties[i]).As<Napi::String>().Utf8Value() != std::to_string(i))
                isSimpleArray = false; // Object.
        }
    } else {
        for(uint32_t i = properties.Length() - 1; ; --i) {
            propertyArray[i] = ((Napi::Value) properties[i]).As<Napi::String>().Utf8Value();

            if(((Napi::Value) properties[i]).As<Napi::String>().Utf8Value() != std::to_string(i))
                isSimpleArray = false; // Object.

            if(i == 0) // Unsignedness causes overflow (always >= 0), so break manually.
                break;
        }
    }

    if(isSimpleArray) {
        delete[] propertyArray;
        propertyArray = nullptr;
    }

    return properties.Length();
}

void buildLoopAssignment(BridgeData& data, std::string& iterator, std::string& assignment, size_t& assignmentUpdateIndex, const uint8_t* it, size_t itSize, const uint8_t* array, size_t arraySize) {
    iterator.assign(reinterpret_cast<const char*>(it), itSize);
    
    assignment.reserve(256);
    assignment.append(reinterpret_cast<const char*>(array), arraySize);
    assignment += "[";

    assignmentUpdateIndex = assignment.size();
}

void updateLoopAssignment(std::string& assignment, std::string& propertyAssignment, size_t& arrayIndex, std::string*& propertyArray, int8_t direction) {
    if(propertyArray == nullptr) {
        assignment += std::to_string(arrayIndex);
        assignment += "]";
    } else {
        propertyAssignment = "Object({key:\"" + propertyArray[arrayIndex] + "\",value:";
        assignment += "\"" + propertyArray[arrayIndex] + "\"]";
    }
    
    arrayIndex += direction;
}

void invalidateLoopAssignment(std::string& assignment, const size_t& assignmentUpdateIndex) {
    assignment.erase(assignmentUpdateIndex, assignment.size() - assignmentUpdateIndex);
}

BridgeBackup copyValue(BridgeData& data, const Napi::Value& value) {
    try {
        return data.clone.Call(std::initializer_list<napi_value>({ value }));
    } catch(std::exception&) {
        return Napi::Value();
    }
}

BridgeBackup backupContext(BridgeData& data) {
    try {
        return data.clone.Call(std::initializer_list<napi_value>({ data.context }));
    } catch(std::exception&) {
        return Napi::Value();
    }
}

BridgeBackup backupLocal(BridgeData& data) {
    try {
        return data.clone.Call(std::initializer_list<napi_value>({ data.local }));
    } catch(std::exception&) {
        return Napi::Value();
    }
}

void initContext(BridgeData& data, const uint8_t* context, size_t contextSize) {
    try {
        if(contextSize == 0)
            data.context = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, "Object({})"), data.context, data.local, data.shared })).ToObject();
        else data.context = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, "Object(" + std::string(reinterpret_cast<const char*>(context), contextSize) + ")"), data.context, data.local, data.shared })).As<Napi::Object>();
    } catch(std::exception &e) {
        throw RenderingException("Component template error", (std::string("context: ") + e.what()).c_str(), context, contextSize);
    }
}

void initLocal(BridgeData& data) {
    try {
        data.local = data.eval.Call(std::initializer_list<napi_value>({ Napi::String::New(data.env, "Object({})"), data.context, data.local, data.shared })).ToObject();
    } catch(std::exception &e) {
        throw RenderingException("Component template error", (std::string("cannot init local object") + e.what()).c_str());
    }
}

void restoreContext(BridgeData& data, BridgeBackup backup) {
    data.context = backup.ToObject();
}

void restoreLocal(BridgeData& data, BridgeBackup backup) {
    data.local = backup.ToObject();
}