#include <stack>
#include <cstdio>
#include <memory>
#include <unordered_set>

#include "engine.hxx"

#include "../def/osh.dxx"
#include "../def/logging.dxx"

#include "../../lib/str.hxx"
#include "../../lib/path.hxx"
#include "../../lib/remem.hxx"
#include "../../lib/buffer.hxx"
#include "../../lib/mem.hxx"
#include "../../lib/chunk.cxx"
#include "../../lib/timer.hxx"

struct LoopStackInfo {
    Eryn::Bridge& bridge;

    size_t arrayIndex;               // Used to iterate over the array.
    size_t arrayLength;              // Used to stop the iteration.
    size_t assignmentUpdateIndex;    // Used to update the assignment string.

    int8_t increment;                // Used to update the iterator.

    bool atEnd;                      // Used to stop a reversed loop.

    std::string  iterator;           // The iterator name.
    std::string  assignment;         // Used to assign a value from the array to a variable.
    std::string  propertyAssignment; // Used to assign a key-value pair from the object to a variable.

    // TODO: use std::vector
    std::string* propertyArray;      // Used for objects.

    LoopStackInfo(Eryn::Bridge& bridge, ConstBuffer iterator, ConstBuffer array, int8_t increment)
        : bridge(bridge), arrayIndex(0), propertyArray(nullptr), increment(increment), atEnd(false) {

        arrayLength = bridge.initArray(array, propertyArray, increment);
        bridge.buildLoopAssignment(this->iterator, assignment, assignmentUpdateIndex, iterator, array);

        if(increment < 0) {
            arrayIndex = arrayLength - 1;
        }

        update();
    };

    LoopStackInfo(const LoopStackInfo& info)
        : bridge(bridge), arrayIndex(info.arrayIndex), arrayLength(info.arrayLength),
          assignmentUpdateIndex(info.assignmentUpdateIndex), iterator(info.iterator),
          assignment(info.assignment), propertyAssignment(info.propertyAssignment),
          increment(info.increment), atEnd(info.atEnd) {
        
        propertyArray = new std::string[info.arrayLength];

        for(size_t i = 0; i < info.arrayLength; ++i)
            propertyArray[i] = info.propertyArray[i];
    }

    LoopStackInfo(LoopStackInfo&& info)
        : bridge(info.bridge), arrayIndex(info.arrayIndex), arrayLength(info.arrayLength),
          assignmentUpdateIndex(info.assignmentUpdateIndex), iterator(info.iterator),
          assignment(info.assignment), propertyAssignment(info.propertyAssignment),
          increment(info.increment), atEnd(info.atEnd) {

        propertyArray = info.propertyArray;
        info.propertyArray = nullptr;
    }

    ~LoopStackInfo() {
        if(propertyArray != nullptr) {
            delete[] propertyArray;
            propertyArray = nullptr;
        }
    }

    void invalidate() {
        bridge.invalidateLoopAssignment(assignment, assignmentUpdateIndex);
    }

    void update() {
        bridge.updateLoopAssignment(assignment, propertyAssignment, arrayIndex, propertyArray, increment);
    }
};

struct ComponentStackInfo {
    bool hasContent;

    const uint8_t* path;
    const uint8_t* context;

    size_t startIndex;
    size_t pathLength;
    size_t contextLength;
};

struct ConditionalStackInfo {
    bool lastConditionalTrue; // Whether or not the last conditional was true.
    size_t lastTrueEndIndex;  // The true end index of the last conditional (used to jump over else).
};

struct Renderer {
    Eryn::Engine&  engine;
    Eryn::Cache&   cache;
    Eryn::Options& opts;
    Eryn::Bridge&  bridge;

    ConstBuffer    input;
    Buffer&        output;
    ConstBuffer    content;

    std::string    meta;

    std::stack<LoopStackInfo>        loopStack;
    std::stack<ComponentStackInfo>   componentStack;
    std::stack<ConditionalStackInfo> conditionalStack;
    std::stack<Eryn::BridgeBackup>   localStack;

    std::unordered_set<std::string>& recompiled;

    bool inputIsString;

    const BDP::Header BDP832 = BDP::Header(8, 32);

    Renderer(Eryn::Engine& engine, Eryn::Bridge& bridge, ConstBuffer input, Buffer& output, std::unordered_set<std::string>& recompiled, std::string meta)
        : engine(engine), cache(engine.cache), bridge(bridge), opts(engine.opts),
          input(input), output(output), recompiled(recompiled), inputIsString(false),
          content(nullptr, 0), meta(meta) { }

    Renderer(const Renderer& renderer)
    : input({ nullptr, 0 }), output(renderer.output), content({ nullptr, 0 }), engine(renderer.engine),
      cache(renderer.cache), opts(renderer.opts), bridge(renderer.bridge), recompiled(renderer.recompiled),
      inputIsString(renderer.inputIsString) { }

    void render();

  private:
    void error(const char* msg, const char* description);
    void error(const char* msg, const char* description, ConstBuffer token);

    void render_component(ConstBuffer component, const Buffer& content);
};

ConstBuffer Eryn::Engine::render(Eryn::Bridge& bridge, const char* path) {
    LOG_DEBUG("===> Rendering '%s'", path);

    CHRONOMETER chrono = time_now();

    std::unordered_set<std::string> recompiled;

    if(opts.flags.bypassCache) {
        compile(path);
        recompiled.insert(std::string(path));
    } else if(!cache.has(path)) {
        if(opts.flags.throwOnMissingEntry) {
            throw Eryn::RenderingException("Item does not exist in cache", "did you forget to compile this?", path);
        }

        compile(path);
    }

    Buffer output;

    auto entry = cache.get(path);

    Renderer renderer(*this, bridge, entry, output, recompiled, path);
    renderer.render();

    if(opts.flags.logRenderTime) {
        LOG_INFO("Rendered in %s\n", getf_exec_time_mis(chrono).c_str());
    }

    return output.finalize();
}

ConstBuffer Eryn::Engine::render_string(Eryn::Bridge& bridge, const char* alias) {
    LOG_DEBUG("===> Rendering '%s'", alias);

    CHRONOMETER chrono = time_now();

    std::unordered_set<std::string> recompiled;

    if(!cache.has(alias)) {
        throw Eryn::RenderingException("Item does not exist in cache", "did you forget to compile this?", alias);
    }

    Buffer output;

    auto entry = cache.get(alias);

    Renderer renderer(*this, bridge, entry, output, recompiled, alias);
    renderer.inputIsString = true;

    renderer.render();

    if(opts.flags.logRenderTime) {
        LOG_INFO("Rendered in %s\n", getf_exec_time_mis(chrono).c_str());
    }

    return output.finalize();
}

void Renderer::error(const char* msg, const char* description) {
    throw Eryn::RenderingException(msg, description, meta.c_str());
}

void Renderer::error(const char* msg, const char* description, ConstBuffer token) {
    throw Eryn::RenderingException(msg, description, meta.c_str(), token);
}

void Renderer::render_component(ConstBuffer component, const Buffer& content) {
    std::string path(reinterpret_cast<const char*>(component.data), component.size);

    LOG_DEBUG("===> Rendering component '%s'", path.c_str());

    if(inputIsString) {
        if(!cache.has(path)) {
            error(("Item '" + path + "' does not exist in cache").c_str(), "did you forget to compile this?");
        }
    } else {
        if(opts.flags.bypassCache) {
            // Don't recompile the same file twice.
            if(recompiled.find(path) == recompiled.end()) {
                engine.compile(path.c_str());
                recompiled.insert(path);
            }
        } else if(!cache.has(path)) {
            if(opts.flags.throwOnMissingEntry) {
                error(("Item '" + path + "' does not exist in cache").c_str(), "did you forget to compile this?");
            }
            engine.compile(path.c_str());
        }
    }

    auto entry = cache.get(path);

    auto subrenderer    = *this;
    subrenderer.input   = entry;
    subrenderer.content = ConstBuffer(content.data, content.size);
    subrenderer.meta    = path;

    subrenderer.render();

    LOG_DEBUG("===> Done\n");
}

void Renderer::render() {
    size_t inputIndex  = 0;
    size_t nameLength  = 0;
    size_t valueLength = 0;

    const uint8_t* name;
    const uint8_t* value;

    while(inputIndex < input.size) {
        BDP::bytesToLength(nameLength, input.data + inputIndex, BDP832.NAME_LENGTH_BYTE_SIZE);
        inputIndex += BDP832.NAME_LENGTH_BYTE_SIZE;
        name = input.data + inputIndex;
        inputIndex += nameLength;

        BDP::bytesToLength(valueLength, input.data + inputIndex, BDP832.VALUE_LENGTH_BYTE_SIZE);
        inputIndex += BDP832.VALUE_LENGTH_BYTE_SIZE;
        value = input.data + inputIndex;
        inputIndex += valueLength;

        uint8_t nameByte = *name;
        
        // TODO: bytecode
        switch(nameByte) {
            case *OSH_PLAINTEXT: {
                LOG_DEBUG("--> Found plaintext");

                output.write(value, valueLength);
                break;
            }
            case *OSH_TEMPLATE: {
                LOG_DEBUG("--> Found template");

                if(valueLength == OSH_TEMPLATE_CONTENT_LENGTH && mem::cmp(value, OSH_TEMPLATE_CONTENT_MARKER, valueLength)) {
                    if(content.size == 0) {
                        if(opts.flags.throwOnEmptyContent) {
                            error("No content", "there is no content for this component", { value, valueLength });
                        }
                    } else {
                        output.write(content);
                    }
                } else {
                    bridge.evalTemplate({ value, valueLength }, output);
                }

                break;
            }
            case *OSH_TEMPLATE_VOID: {
                LOG_DEBUG("--> Found void template");

                bridge.evalVoidTemplate({ value, valueLength });
                break;
            }
            case *OSH_TEMPLATE_CONDITIONAL_START: {
                LOG_DEBUG("--> Found conditional template start");

                size_t conditionalEnd;
                BDP::bytesToLength(conditionalEnd, input.data + inputIndex, OSH_FORMAT);
                inputIndex += OSH_FORMAT;

                size_t trueConditionalEnd;
                BDP::bytesToLength(trueConditionalEnd, input.data + inputIndex, OSH_FORMAT);
                inputIndex += OSH_FORMAT;

                if(!bridge.evalConditionalTemplate({ value, valueLength }, output)) {
                    inputIndex += conditionalEnd;

                    ConditionalStackInfo info;
                    info.lastConditionalTrue = false;
                    info.lastTrueEndIndex = 0;
                    conditionalStack.push(info);
                } else {
                    ConditionalStackInfo info;
                    info.lastConditionalTrue = true;
                    info.lastTrueEndIndex = trueConditionalEnd;
                    conditionalStack.push(info);
                }
                
                break;
            }
            case *OSH_TEMPLATE_ELSE_CONDITIONAL_START: {
                LOG_DEBUG("--> Found else conditional template start");

                if(conditionalStack.empty()) {
                    error("PANIC",
                          ((std::string("else conditional template does not have a preceding conditional template on the stack: OSH marker '") +
                          ((char) nameByte)) + "' (REPORT THIS TO THE DEVS)").c_str(), { name, nameLength });
                }

                // Because the cursor is after the Name/Value pair, the lengths must be deducted from the true end index (which is relative to the beginning).
                if(conditionalStack.top().lastConditionalTrue) {
                    inputIndex += conditionalStack.top().lastTrueEndIndex - valueLength - BDP832.VALUE_LENGTH_BYTE_SIZE - nameLength - BDP832.NAME_LENGTH_BYTE_SIZE;  
                    conditionalStack.pop();
                    break;
                }

                conditionalStack.pop();

                size_t conditionalEnd;
                BDP::bytesToLength(conditionalEnd, input.data + inputIndex, OSH_FORMAT);
                inputIndex += OSH_FORMAT;

                size_t trueConditionalEnd;
                BDP::bytesToLength(trueConditionalEnd, input.data + inputIndex, OSH_FORMAT);
                inputIndex += OSH_FORMAT;

                if(!bridge.evalConditionalTemplate({ value, valueLength }, output)) {
                    inputIndex += conditionalEnd;

                    ConditionalStackInfo info;
                    info.lastConditionalTrue = false;
                    info.lastTrueEndIndex = 0;
                    conditionalStack.push(info);
                } else {
                    ConditionalStackInfo info;
                    info.lastConditionalTrue = true;
                    info.lastTrueEndIndex = trueConditionalEnd;
                    conditionalStack.push(info);
                }
                break;
            }
            case *OSH_TEMPLATE_ELSE_START: {
                LOG_DEBUG("--> Found else template start");

                if(conditionalStack.empty()) {
                    error("PANIC",
                          ((std::string("else template does not have a preceding conditional template on the stack: OSH marker '") +
                          ((char) nameByte)) + "' (REPORT THIS TO THE DEVS)").c_str(), { name, nameLength });
                }

                // Because the cursor is after the Name/Value pair, the lengths must be deducted from the true end index (which is relative to the beginning).
                if(conditionalStack.top().lastConditionalTrue) {
                    inputIndex += conditionalStack.top().lastTrueEndIndex - valueLength - BDP832.VALUE_LENGTH_BYTE_SIZE - nameLength - BDP832.NAME_LENGTH_BYTE_SIZE;             
                    conditionalStack.pop(); // Jumping skips the end template, so pop the stack.
                }

                break;
            }
            case *OSH_TEMPLATE_CONDITIONAL_BODY_END: {
                LOG_DEBUG("--> Found conditional template end");

                if(conditionalStack.empty()) {
                    error("PANIC",
                          ((std::string("conditional body end does not have a preceding conditional template on the stack: OSH marker '") +
                          ((char) nameByte)) + "' (REPORT THIS TO THE DEVS)").c_str(), { name, nameLength });
                }

                conditionalStack.pop();
                continue;
            }
            case *OSH_TEMPLATE_INVERTED_CONDITIONAL_START: {
                LOG_DEBUG("--> Found inverted conditional template start");

                size_t conditionalEnd;
                BDP::bytesToLength(conditionalEnd, input.data + inputIndex, OSH_FORMAT);
                inputIndex += OSH_FORMAT;

                size_t trueConditionalEnd;
                BDP::bytesToLength(trueConditionalEnd, input.data + inputIndex, OSH_FORMAT);
                inputIndex += OSH_FORMAT;

                if(bridge.evalConditionalTemplate({ value, valueLength }, output)) {
                    inputIndex += conditionalEnd;

                    ConditionalStackInfo info;
                    info.lastConditionalTrue = false;
                    info.lastTrueEndIndex = 0;
                    conditionalStack.push(info);
                } else {
                    ConditionalStackInfo info;
                    info.lastConditionalTrue = true;
                    info.lastTrueEndIndex = trueConditionalEnd;
                    conditionalStack.push(info);
                }
                break;
            }
            case *OSH_TEMPLATE_LOOP_START: {
                LOG_DEBUG("--> Found loop template start");

                size_t leftLength;
                size_t rightLength;

                const uint8_t* left;
                const uint8_t* right;

                inputIndex -= valueLength;

                left = input.data + inputIndex + BDP832.VALUE_LENGTH_BYTE_SIZE;
                BDP::bytesToLength(leftLength, input.data + inputIndex, BDP832.VALUE_LENGTH_BYTE_SIZE);
                inputIndex += BDP832.VALUE_LENGTH_BYTE_SIZE + leftLength;

                BDP::bytesToLength(rightLength, input.data + inputIndex, BDP832.VALUE_LENGTH_BYTE_SIZE);
                inputIndex += BDP832.VALUE_LENGTH_BYTE_SIZE;
                right = input.data + inputIndex;
                inputIndex += rightLength;

                // 1 = direction (+1 increments)
                loopStack.push(LoopStackInfo(bridge, { left, leftLength }, { right, rightLength }, 1));

                if(loopStack.top().arrayLength == 0) {
                    size_t loopEnd;
                    BDP::bytesToLength(loopEnd, input.data + inputIndex, OSH_FORMAT);

                    inputIndex += OSH_FORMAT + loopEnd;
                    loopStack.pop();
                } else {
                    inputIndex += OSH_FORMAT;
                    localStack.push(bridge.backupLocal());
                    bridge.evalAssignment(opts.flags.cloneIterators, loopStack.top().iterator, loopStack.top().assignment, loopStack.top().propertyAssignment);
                }

                break;
            }
            case *OSH_TEMPLATE_LOOP_REVERSE_START: {
                LOG_DEBUG("--> Found reverse loop template start");

                size_t leftLength;
                size_t rightLength;

                const uint8_t* left;
                const uint8_t* right;

                inputIndex -= valueLength;

                left = input.data + inputIndex + BDP832.VALUE_LENGTH_BYTE_SIZE;
                BDP::bytesToLength(leftLength, input.data + inputIndex, BDP832.VALUE_LENGTH_BYTE_SIZE);
                inputIndex += BDP832.VALUE_LENGTH_BYTE_SIZE + leftLength;

                BDP::bytesToLength(rightLength, input.data + inputIndex, BDP832.VALUE_LENGTH_BYTE_SIZE);
                inputIndex += BDP832.VALUE_LENGTH_BYTE_SIZE;
                right = input.data + inputIndex;
                inputIndex += rightLength;

                loopStack.push(LoopStackInfo(bridge, { left, leftLength }, { right, rightLength }, -1));

                if(loopStack.top().arrayLength == 0) {
                    size_t loopEnd;
                    BDP::bytesToLength(loopEnd, input.data + inputIndex, OSH_FORMAT);

                    inputIndex += OSH_FORMAT + loopEnd;
                    loopStack.pop();
                } else {
                    inputIndex += OSH_FORMAT;
                    localStack.push(bridge.backupLocal());
                    bridge.evalAssignment(opts.flags.cloneIterators, loopStack.top().iterator, loopStack.top().assignment, loopStack.top().propertyAssignment);
                }

                break;
            }
            case *OSH_TEMPLATE_LOOP_BODY_END: {
                LOG_DEBUG("--> Found loop template end");

                if(loopStack.top().arrayIndex < loopStack.top().arrayLength && !loopStack.top().atEnd) {
                    if(loopStack.top().arrayIndex == 0 && loopStack.top().increment < 0) {// Mark this as the last iteration (the index will overflow).
                        loopStack.top().atEnd = true;
                    }

                    loopStack.top().invalidate();
                    loopStack.top().update();

                    // TODO: add option to disable cloning
                    bridge.restoreLocal(bridge.copyValue(localStack.top())); // In case the array uses the parent local object.
                    bridge.evalAssignment(opts.flags.cloneIterators, loopStack.top().iterator, loopStack.top().assignment, loopStack.top().propertyAssignment);

                    size_t loopStart;

                    BDP::bytesToLength(loopStart, input.data + inputIndex, OSH_FORMAT);

                    inputIndex += OSH_FORMAT;
                    inputIndex -= loopStart;
                } else {
                    inputIndex += OSH_FORMAT;

                    // TODO: discard this for improved performance?
                    bridge.unassign(loopStack.top().iterator);
                    loopStack.pop();

                    bridge.restoreLocal(localStack.top());
                    localStack.pop();
                }

                break;
            }
            case *OSH_TEMPLATE_COMPONENT: {
                LOG_DEBUG("--> Found component template\n");

                inputIndex -= valueLength;

                ComponentStackInfo info;

                info.path = input.data + inputIndex + BDP832.VALUE_LENGTH_BYTE_SIZE;
                BDP::bytesToLength(info.pathLength, input.data + inputIndex, BDP832.VALUE_LENGTH_BYTE_SIZE);
                inputIndex += BDP832.VALUE_LENGTH_BYTE_SIZE + info.pathLength;

                BDP::bytesToLength(info.contextLength, input.data + inputIndex, BDP832.VALUE_LENGTH_BYTE_SIZE);
                inputIndex += BDP832.VALUE_LENGTH_BYTE_SIZE;
                info.context = input.data + inputIndex;
                inputIndex += info.contextLength;

                size_t contentLength;

                BDP::bytesToLength(contentLength, input.data + inputIndex, OSH_FORMAT);

                inputIndex += OSH_FORMAT;

                if(contentLength == 0) {
                    info.hasContent = false;

                    auto contextBackup = bridge.backupContext();
                    auto localBackup   = bridge.backupLocal();

                    bridge.initContext({ info.context, info.contextLength });
                    bridge.initLocal();

                    render_component({ info.path, info.pathLength }, { nullptr, 0 });

                    bridge.restoreContext(contextBackup);
                    bridge.restoreLocal(localBackup);
                } else {
                    info.hasContent = true;
                    info.startIndex = output.size;
                }

                // Even if the component has no content and it has already been rendered, push it.
                // When the component end will be encountered, it will be popped directly.
                componentStack.push(info);
                break;
            }
            case *OSH_TEMPLATE_COMPONENT_BODY_END: {
                LOG_DEBUG("--> Found component template end");

                ComponentStackInfo info = componentStack.top();

                if(info.hasContent) {
                    // TODO: maybe write directly to a content buffer?

                    auto contentLength = output.size - info.startIndex;
                    Buffer content;

                    content.write(output.data + info.startIndex, contentLength);

                    output.size = info.startIndex;

                    auto contextBackup = bridge.backupContext();
                    auto localBackup   = bridge.backupLocal();

                    bridge.initContext({ info.context, info.contextLength });
                    bridge.initLocal();

                    render_component({ info.path, info.pathLength }, content);
                    
                    bridge.restoreContext(contextBackup);
                    bridge.restoreLocal(localBackup);
                }

                componentStack.pop();
                break;
            }
            default:
                LOG_DEBUG("%zu", name - input.data);
                error("Not supported",
                      (((std::string("this template type is not supported: OSH marker '") + ((char) nameByte)) + "' at index ") +
                      std::to_string(name - input.data)).c_str(), { name, nameLength });
        }
    }
}