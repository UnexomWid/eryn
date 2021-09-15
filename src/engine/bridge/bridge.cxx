#include "bridge.hxx"
#include "../engine.hxx"

#include "../../def/logging.dxx"
#include "../../def/warnings.dxx"
#include "../../../lib/buffer.hxx"

static std::string stringify(const Napi::Env& env, const Napi::Object& object) {
    Napi::Object json = env.Global().Get("JSON").As<Napi::Object>();
    Napi::Function stringify = env.Global().Get("JSON").As<Napi::Object>().Get("stringify").As<Napi::Function>();

    return stringify.Call(json, { object }).As<Napi::String>().Utf8Value();
}

static Napi::Value call_eval(Eryn::BridgeData& data, const Napi::String& script) {
    return data.eval.Call(std::initializer_list<napi_value>({
        script,
        data.context,
        data.local,
        data.shared
    }));
}

static Napi::Value call_eval(Eryn::BridgeData& data, ConstBuffer script) {
    std::string str = std::string(reinterpret_cast<const char*>(script.data), script.size);
    
    // If the user writes {test: "Test"}, this should be treated as an expression.
    // By default, it's treated as a block, but it will be treated as an expression if it's
    // surrounded by parentheses. If the user truly wants the script to start with a block,
    // they have to place dummy content like /**/ or /* Block */ before the opening bracket.
    if(script.size > 0 && script.data[0] == '{') {
        str ="(" + str + ")";
    }

    return call_eval(data, Napi::String::New(data.env, str));
}

static Napi::Value call_clone(Eryn::BridgeData& data, const Napi::Value& original) {
    return data.clone.Call(std::initializer_list<napi_value>({
        original
    }));
}

Eryn::NormalBridge::NormalBridge(Eryn::BridgeData&& data) : Bridge(std::forward<Eryn::BridgeData>(data)) { }

void Eryn::NormalBridge::evalTemplate(ConstBuffer input, Buffer& output) {
    Napi::Value result;

    try {
        result = call_eval(data, input);
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Template error", e.what(), input);
    }

    if(result.IsUndefined() || result.IsNull())
        return;
    else if(result.IsString()) {
        LOG_DEBUG("    Type: string");

        std::string str = result.As<Napi::String>().Utf8Value();
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        output.write(ptr, str.size());
    } else if(result.IsBuffer()) {
        LOG_DEBUG("    Type: buffer");

        uint8_t* ptr = reinterpret_cast<uint8_t*>(result.As<Napi::Buffer<char>>().Data());
        size_t length = result.As<Napi::Buffer<char>>().Length();

        output.write(ptr, length);
    } else if(result.IsObject()) {
        LOG_DEBUG("    Type: object");

        std::string str = stringify(data.env, result.As<Napi::Object>());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        output.write(ptr, str.size());
    } else if(result.IsArray()) {
        LOG_DEBUG("    Type: array");

        std::string str = stringify(data.env, result.ToObject());
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        output.write(ptr, str.size());
    } else if(result.IsNumber()) {
        LOG_DEBUG("    Type: number");

        std::string str = result.ToString().Utf8Value();
        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        output.write(ptr, str.size());
    } else {
        throw Eryn::RenderingException("Unsupported template return type", "must be String, Number, Object, Array, Buffer, null or undefined", input);
    }
}

void Eryn::NormalBridge::evalVoidTemplate(ConstBuffer input) {
    try {
        call_eval(data, input);
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Void template error", e.what(), input);
    }
}

bool Eryn::NormalBridge::evalConditionalTemplate(ConstBuffer input) {
    Napi::Value result;

    try {
        result = call_eval(data, input);
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Conditional template error", e.what(), input);
    }

    return result.ToBoolean().Value();
}

void Eryn::NormalBridge::evalAssignment(bool cloneIterators, const std::string& iterator, const std::string& assignment, const std::string& propertyAssignment) {
    if(cloneIterators) {
        // Object
        if(propertyAssignment.size() > 0) {
            data.local[iterator] = call_clone(
                data,
                call_eval(data, Napi::String::New(data.env, propertyAssignment + assignment + "})"))
            ).ToObject();
        } else {
            data.local[iterator] = call_clone(
                data,
                call_eval(data, Napi::String::New(data.env, assignment))
            );
        }
    } else {
        // Object.
        if(propertyAssignment.size() > 0) {
            data.local[iterator] = call_eval(
                data,
                Napi::String::New(data.env, propertyAssignment + assignment + "})")
            ).ToObject();
        }
        else {
            data.local[iterator] = call_eval(
                data,
                Napi::String::New(data.env, assignment)
            );
        }
    }
}

void Eryn::NormalBridge::unassign(const std::string &iterator) {
    data.local[iterator] = call_eval(
        data,
        Napi::String::New(data.env, "undefined")
    );
}

size_t Eryn::NormalBridge::initArray(ConstBuffer array, std::string*& propertyArray, int8_t direction) {
    Napi::Value result;

    try {
        result = call_eval(data, array);
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Loop template error", e.what(), array);
    }

    bool isSimpleArray = true;

    if(result.IsArray()) {
        return result.As<Napi::Array>().Length();
    }
    
    if(!result.IsObject()) {
        throw Eryn::RenderingException("Unsupported loop right operand", "must be Array or Object", array);
    }

    Napi::Array properties = result.ToObject().GetPropertyNames();

    // TODO: render nothing?
    if(properties.Length() == 0) {
        throw RenderingException("Unsupported loop right operand", "length is 0", array);
    }

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

void Eryn::NormalBridge::buildLoopAssignment(std::string& iterator, std::string& assignment, size_t& assignmentUpdateIndex, ConstBuffer it, ConstBuffer array) {
    iterator.assign(reinterpret_cast<const char*>(it.data), it.size);
    
    assignment.reserve(32);
    assignment.append(reinterpret_cast<const char*>(array.data), array.size);
    assignment += "[";

    assignmentUpdateIndex = assignment.size();
}

// Assignment: iterator = arr[index]
// Assignment: iterator = Object({key: prop, value: obj["prop"]})
void Eryn::NormalBridge::updateLoopAssignment(std::string& assignment, std::string& propertyAssignment, size_t& arrayIndex, std::string*& propertyArray, int8_t direction) {
    if(propertyArray == nullptr) {
        assignment += std::to_string(arrayIndex);
        assignment += "]";
    } else {
        propertyAssignment = "Object({key:\"" + propertyArray[arrayIndex] + "\",value:";
        assignment += "\"" + propertyArray[arrayIndex] + "\"]";
    }
    
    arrayIndex += direction;
}

void Eryn::NormalBridge::invalidateLoopAssignment(std::string& assignment, const size_t& assignmentUpdateIndex) {
    assignment.erase(assignmentUpdateIndex, assignment.size() - assignmentUpdateIndex);
}

// Like call_clone, but this one is exposed by the bridge and also catches any exceptions.
Eryn::BridgeBackup Eryn::NormalBridge::copyValue(const Napi::Value& value) {
    try {
        return call_clone(data, value);
    } catch(std::exception& e) {
        UNREFERENCED(e); // The exception data is used in debug mode. Suppress release warnings.

        LOG_DEBUG("[WARN] Bridge function 'copyValue' caught an exception:\n%s", e.what());
        return Napi::Value();
    }
}

Eryn::BridgeBackup Eryn::NormalBridge::backupContext(bool cloneBackup) {
    try {
        if(cloneBackup) {
            return call_clone(data, data.context);
        }
        return data.context;
    } catch(std::exception& e) {
        UNREFERENCED(e); // The exception data is used in debug mode. Suppress release warnings.

        LOG_DEBUG("[WARN] Bridge function 'backupContext' caught an exception:\n%s", e.what());
        return Napi::Value();
    }
}

Eryn::BridgeBackup Eryn::NormalBridge::backupLocal(bool cloneBackup) {
    try {
        if(cloneBackup) {
            return call_clone(data, data.local);
        }
        return data.local;
    } catch(std::exception& e) {
        UNREFERENCED(e); // The exception data is used in debug mode. Suppress release warnings.

        LOG_DEBUG("[WARN] Bridge function 'backupLocal' caught an exception:\n%s", e.what());
        return Napi::Value();
    }
}

void Eryn::NormalBridge::initContext(ConstBuffer context) {
    try {
        if(context.size == 0) {
            data.context = call_eval(
                data,
                Napi::String::New(data.env, "Object({})")
            );//.ToObject();
        } else {
            data.context = call_eval(
                data,
                context
            );//.As<Napi::Object>();
        }
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Component template error", (std::string("context: ") + e.what()).c_str(), context);
    }
}

void Eryn::NormalBridge::initLocal() {
    try {
        data.local = call_eval(
            data,
            Napi::String::New(data.env, "Object({})")
        ).ToObject();
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Component template error", (std::string("cannot init local object") + e.what()).c_str());
    }
}

void Eryn::NormalBridge::restoreContext(Eryn::BridgeBackup backup) {
    data.context = backup;//.ToObject();
}

void Eryn::NormalBridge::restoreLocal(Eryn::BridgeBackup backup) {
    data.local = backup.ToObject();
}