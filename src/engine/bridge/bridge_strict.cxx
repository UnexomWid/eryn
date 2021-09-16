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
    size_t accessorIndex;

    if(script.match("context", sizeof("context") - 1)) {
        baseValue = data.context;
        accessorIndex = sizeof("context") - 1;
    } else if(script.match("local", sizeof("local") - 1)) {
        baseValue = data.local;
        accessorIndex = sizeof("local") - 1;
    } else if(script.match("shared"), sizeof("shared") - 1) {
        baseValue = data.shared;
        accessorIndex = sizeof("shared") - 1;
    } else {
        throw Eryn::RenderingException("Template content is too complex", "'strict' mode only supports simple content such as 'context.fieldName'; consider using 'normal' mode", script);
    }

    size_t fieldEnd;

    // If the dot isn't found, check for ["field"] or return the base value itself.
    if(script.match(accessorIndex, ".", sizeof(".") - 1)) {
        accessorIndex += sizeof(".") - 1;
        fieldEnd = script.size - 1;
    } else if(script.match(accessorIndex, "[\"", sizeof("[\"") - 1)) {
        accessorIndex += sizeof("[\"") - 1;

        auto accessorEnd = script.find_index(accessorIndex, "\"]", sizeof("\"]") - 1);

        if(accessorEnd == script.size) {
            throw Eryn::RenderingException("Expected \"] after [\"", "did you forget to write the accessor end?", script);
        }

        // Script ends with "], which is not part of the field.
        fieldEnd = accessorEnd - 1;
    } else {
        // There shouldn't be anything after the object.
        if(accessorIndex != script.size) {
            throw Eryn::RenderingException("Expected end of template", "'strict' mode supports 'context', 'local', 'shared', or a field of those; consider using 'normal' mode", script);
        }

        return baseValue;
    }

    // Accessor found, return the value of the field if the base value is an object.
    if(!baseValue.IsObject()) {
        std::string baseName(reinterpret_cast<const char*>(script.data), accessorIndex);

        throw Eryn::RenderingException("Cannot access field of non-object value", ("make sure '" + baseName + "' is an object").c_str(), script);
    }

    auto base  = baseValue.As<Napi::Object>();
    auto field = Napi::String::New(data.env, std::string(reinterpret_cast<const char*>(script.data + accessorIndex), fieldEnd - accessorIndex));

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

void Eryn::StrictBridge::evalIteratorArrayAssignment(bool cloneIterators, const std::string& iterator, const BridgeIterable& iterable, uint32_t index) {
    if(cloneIterators) {
        data.local[iterator] = call_clone(
            data,
            iterable.Get(index)
        );
    } else {
        data.local[iterator] = iterable.Get(index);
    }
}

void Eryn::StrictBridge::evalIteratorObjectAssignment(bool cloneIterators, const std::string& iterator, const Eryn::BridgeIterable& iterable, const Eryn::BridgeObjectKeys& keys, uint32_t index) {
    auto it = Napi::Object::New(data.env);

    it["key"] = keys[index];
    
    if(cloneIterators) {
        it["value"] = call_clone(
            data,
            iterable.Get(keys[index])
        );
    } else {
        it["value"] = iterable.Get(keys[index]);
    }

    data.local[iterator] = it;
}

bool Eryn::StrictBridge::initLoopIterable(ConstBuffer arrayScript, Eryn::BridgeIterable& iterable, Eryn::BridgeObjectKeys& keys, uint32_t step) {
    Napi::Value result;

    try {
        result = eval(data, arrayScript);
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Loop template error", e.what(), arrayScript);
    }

    bool isSimpleArray = true;
    
    if(!result.IsObject() && !result.IsArray()) {
        throw Eryn::RenderingException("Unsupported loop right operand", "must be Array or Object", arrayScript);
    }

    auto properties = result.ToObject().GetPropertyNames();
    size_t propertiesCount = properties.Length();

    // TODO: render nothing?
    if(propertiesCount == 0) {
        throw RenderingException("Unsupported loop right operand", "length is 0", arrayScript);
    }

    keys.clear();
    keys.reserve(propertiesCount);

    for(uint32_t i = 0; i < properties.Length(); i += step) {
        keys.push_back(((Napi::Value) properties[i]));

        if(((Napi::Value) properties[i]).As<Napi::String>().Utf8Value() != std::to_string(i)) {
            isSimpleArray = false; // Object.
        }
    }

    // TODO: check if this is needed.
    if(result.IsArray()) {
        iterable = result.As<BridgeArray>();
    } else {
        iterable = result.As<BridgeObject>();
    }

    return isSimpleArray;
}

void Eryn::StrictBridge::unassign(const std::string &iterator) {
    
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
            data.context = Napi::Object::New(data.env);
        } else {
            data.context = eval(
                data,
                context
            );//.As<Napi::Object>();
        }
    } catch(std::exception &e) {
        throw Eryn::RenderingException("Component template error", (std::string("context: ") + e.what()).c_str(), context);
    }
}

void Eryn::StrictBridge::initLocal() {
    try {
        data.local = Napi::Object::New(data.env);
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