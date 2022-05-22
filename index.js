var { ErynEngine } = require('./build-load')(__dirname);
const v8 = require('v8');

function bridgeDeepClone(obj) {
    return v8.deserialize(v8.serialize(obj));
}

function bridgeShallowClone(obj) {
    return Object.assign({}, obj);
}

function bridgeEval(script, context, local, shared) {
    return eval(script);
}

class ErynBinding {
    constructor(options = undefined) {
        this.binding = new ErynEngine(options);
        this.bridgeOptions = {
            enableDeepCloning: false
        };

        // Annoying edge case.
        if(options.hasOwnProperty("enableDeepCloning") && ((typeof options.enableDeepCloning) === "boolean")) {
            this.bridgeOptions.enableDeepCloning = options.enableDeepCloning;
        }
    }

    compile(path) {
        if(!(path && (typeof path === 'string' && !(path instanceof String))))
            throw `Invalid argument 'path' (expected: string | found: ${typeof(path)})`

        this.binding.compile(path);
    }

    compileDir(dirPath, filters) {
        if(!dirPath)
            dirPath = "";
        if(!(typeof dirPath === 'string' || (dirPath instanceof String)))
            throw `Invalid argument 'dirPath' (expected: string | found: ${typeof(dirPath)})`
        if(!(filters && (filters instanceof Array)))
            throw `Invalid argument 'filters' (expected: array | found: ${typeof(filters)})`

        this.binding.compileDir(dirPath, filters);
    }

    compileString(alias, str) {
        if(!(alias && (typeof alias === 'string' && !(alias instanceof String))))
            throw `Invalid argument 'alias' (expected: string | found: ${typeof(path)})`
        if(!(str && (typeof str === 'string' && !(str instanceof String))))
            throw `Invalid argument 'str' (expected: string | found: ${typeof(path)})`

        this.binding.compileString(alias, str);
    }

    express(path, context, callback) {
        try {
            callback(null, eryn.render(path, context));
        } catch (error) {
            callback(error);
        }
    }

    render(path, context, shared) {
        if(!(path && (typeof path === 'string' && !(path instanceof String))))
            throw `Invalid argument 'path' (expected: string | found: ${typeof(path)})`
        if(!context)
            context = {};
        if(!shared)
            shared = {};

        return this.binding.render(path, context, {}, shared, bridgeEval, this.bridgeOptions.enableDeepCloning ? bridgeDeepClone : bridgeShallowClone);
    }

    renderString(alias, context, shared) {
        if(!(alias && (typeof alias === 'string' && !(alias instanceof String))))
            throw `Invalid argument 'alias' (expected: string | found: ${typeof(path)})`
        if(!context)
            context = {};
        if(!shared)
            shared = {};

        return this.binding.renderString(alias, context, {}, shared, bridgeEval, this.bridgeOptions.enableDeepCloning ? bridgeDeepClone : bridgeShallowClone);
    }

    renderStringUncached(src, context, shared) {
        if(!(src && (typeof src === 'string' && !(src instanceof String))))
            throw `Invalid argument 'src' (expected: string | found: ${typeof(path)})`
        if(!context)
            context = {};
        if(!shared)
            shared = {};

        this.compileString('__ERYN_uncached', src);

        return this.binding.renderString('__ERYN_uncached', context, {}, shared, bridgeEval, this.bridgeOptions.enableDeepCloning ? bridgeDeepClone : bridgeShallowClone);
    }

    setOptions(options) {
        if(!(options && (typeof options === 'object')))
            throw `Invalid argument 'options' (expected: object | found: ${typeof(options)})`

        if(options.hasOwnProperty("enableDeepCloning") && ((typeof options.enableDeepCloning) === "boolean"))
            this.bridgeOptions.enableDeepCloning = options.enableDeepCloning;

        this.binding.options(options);
    }
}

const eryn = (options = undefined) => {
    return new ErynBinding(options);
};

module.exports = eryn;
