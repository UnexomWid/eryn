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

static Napi::Value call_clone(Eryn::BridgeData& data, const Napi::Value& original) {
    return data.clone.Call(std::initializer_list<napi_value>({
        original
    }));
}

// Evaluates a piece of script to a Napi::Value.
static Napi::Value eval(Eryn::BridgeData& data, ConstBuffer script) {
    Napi::Value baseValue;

    if(script.match("context", sizeof("context") - 1)) {
        baseValue = data.context;
    } else if(script.match("local", sizeof("local") - 1)) {
        baseValue = data.local;
    } else if(script.match("shared"), sizeof("shared") - 1) {
        baseValue = data.shared;
    } else {
        throw Eryn::RenderingException("Template content is too complex", "'strict' mode only supports simple content such as 'context.fieldName'. Consider using 'normal' mode", script);
    }

    auto dot = script.find(".", 1);

    // Dot not found, return the base value itself.
    if(dot == script.end()) {
        return baseValue;
    }

    // Dot found, return the value of the field if the base value is an object.
    if(!baseValue.IsObject()) {
        std::string baseName(reinterpret_cast<const char*>(script.data), dot - script.data);

        throw Eryn::RenderingException("Cannot access field of non-object value", ("make sure '" + baseName + "' is an object").c_str(), script);
    }

    auto base  = baseValue.As<Napi::Object>();
    auto field = Napi::String::New(data.env, std::string(reinterpret_cast<const char*>(dot), script.size - (dot - script.data)));

    return base.Get(field);
}

Eryn::StrictBridge::StrictBridge(Eryn::BridgeData&& data) : Bridge(std::forward<Eryn::BridgeData>(data)) { }

void Eryn::StrictBridge::evalTemplate(ConstBuffer input, Buffer& output) {
    Napi::Value result;

    try {
        result = eval(data, input);
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Template error", e.what(), input);
    }

    if(result.IsUndefined() || result.IsNull()) {
        return;
    } else if(result.IsString()) {
        LOG_DEBUG("    Type: string");

        auto str = result.As<Napi::String>().Utf8Value();
        auto ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        output.write(ptr, str.size());
    } else if(result.IsBuffer()) {
        LOG_DEBUG("    Type: buffer");

        auto ptr = reinterpret_cast<uint8_t*>(result.As<Napi::Buffer<char>>().Data());
        auto length = result.As<Napi::Buffer<char>>().Length();

        output.write(ptr, length);
    } else if(result.IsObject()) {
        LOG_DEBUG("    Type: object");

        auto str = stringify(data.env, result.As<Napi::Object>());
        auto ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        output.write(ptr, str.size());
    } else if(result.IsArray()) {
        LOG_DEBUG("    Type: array");

        auto str = stringify(data.env, result.ToObject());
        auto ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        output.write(ptr, str.size());
    } else if(result.IsNumber()) {
        LOG_DEBUG("    Type: number");

        auto str = result.ToString().Utf8Value();
        auto ptr = reinterpret_cast<const uint8_t*>(str.c_str());

        output.write(ptr, str.size());
    } else {
        throw Eryn::RenderingException("Unsupported template return type", "must be String, Number, Object, Array, Buffer, null or undefined", input);
    }
}

void Eryn::StrictBridge::evalVoidTemplate(ConstBuffer input) {
    throw Eryn::RenderingException("Unsupported template type", "void templates are not supported in 'strict' mode", input);
}

bool Eryn::StrictBridge::evalConditionalTemplate(ConstBuffer input) {
    Napi::Value result;

    try {
        result = eval(data, input);
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Conditional template error", e.what(), input);
    }

    return result.ToBoolean().Value();
}

void Eryn::StrictBridge::evalAssignment(bool cloneIterators, const std::string& iterator, const std::string& assignment, const std::string& propertyAssignment) {
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

void Eryn::StrictBridge::evalIteratorArrayAssignment(bool cloneIterators, const std::string& iterator, const BridgeIterable& iterable, uint32_t index) {
    
}

void Eryn::StrictBridge::evalIteratorObjectAssignment(bool cloneIterators, const std::string& iterator, const Eryn::BridgeIterable& iterable, const Eryn::BridgeObjectKeys& keys, uint32_t index) {
    
}

bool Eryn::StrictBridge::initLoopIterable(ConstBuffer arrayScript, Eryn::BridgeIterable& iterable, Eryn::BridgeObjectKeys& keys, uint32_t step) {
    return false;
}

void Eryn::StrictBridge::unassign(const std::string &iterator) {
    data.local[iterator] = call_eval(
        data,
        Napi::String::New(data.env, "undefined")
    );
}

void Eryn::StrictBridge::buildLoopAssignment(std::string& iterator, std::string& assignment, size_t& assignmentUpdateIndex, ConstBuffer it, ConstBuffer array) {
    iterator.assign(reinterpret_cast<const char*>(it.data), it.size);
    
    assignment.reserve(32);
    assignment.append(reinterpret_cast<const char*>(array.data), array.size);
    assignment += "[";

    assignmentUpdateIndex = assignment.size();
}

// Assignment: iterator = arr[index]
// Assignment: iterator = Object({key: prop, value: obj["prop"]})
void Eryn::StrictBridge::updateLoopAssignment(std::string& assignment, std::string& propertyAssignment, size_t& arrayIndex, std::string*& propertyArray, int8_t direction) {
    if(propertyArray == nullptr) {
        assignment += std::to_string(arrayIndex);
        assignment += "]";
    } else {
        propertyAssignment = "Object({key:\"" + propertyArray[arrayIndex] + "\",value:";
        assignment += "\"" + propertyArray[arrayIndex] + "\"]";
    }
    
    arrayIndex += direction;
}

void Eryn::StrictBridge::invalidateLoopAssignment(std::string& assignment, const size_t& assignmentUpdateIndex) {
    assignment.erase(assignmentUpdateIndex, assignment.size() - assignmentUpdateIndex);
}

// Like call_clone, but this one is exposed by the bridge and also catches any exceptions.
Eryn::BridgeBackup Eryn::StrictBridge::copyValue(const Napi::Value& value) {
    try {
        return call_clone(data, value);
    } catch(std::exception& e) {
        UNREFERENCED(e); // The exception data is used in debug mode. Suppress release warnings.

        LOG_DEBUG("[WARN] Bridge function 'copyValue' caught an exception:\n%s", e.what());
        return Napi::Value();
    }
}

Eryn::BridgeBackup Eryn::StrictBridge::backupContext(bool cloneBackup) {
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

Eryn::BridgeBackup Eryn::StrictBridge::backupLocal(bool cloneBackup) {
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

void Eryn::StrictBridge::initContext(ConstBuffer context) {
    try {
        if(context.size == 0) {
            data.context = call_eval(
                data,
                Napi::String::New(data.env, "Object({})")
            );//.ToObject();
        } else {
            ///data.context = call_eval(
            ///    data,
            ///    context
            ///);//.As<Napi::Object>();
        }
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Component template error", (std::string("context: ") + e.what()).c_str(), context);
    }
}

void Eryn::StrictBridge::initLocal() {
    try {
        data.local = call_eval(
            data,
            Napi::String::New(data.env, "Object({})")
        ).ToObject();
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Component template error", (std::string("cannot init local object") + e.what()).c_str());
    }
}

void Eryn::StrictBridge::restoreContext(Eryn::BridgeBackup backup) {
    data.context = backup;//.ToObject();
}

void Eryn::StrictBridge::restoreLocal(Eryn::BridgeBackup backup) {
    data.local = backup.ToObject();
}