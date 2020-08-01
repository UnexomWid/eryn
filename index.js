var binding = require('./build-load')(__dirname);
const v8 = require('v8');

var bridgeOptions = {
    enableDeepCloning: false
}

function bridgeDeepClone(obj) {
    return v8.deserialize(v8.serialize(obj));
}

function bridgeShallowClone(obj) {
    return Object.assign({}, obj);
}

function bridgeEval(script, context, local) {
    return eval(script);
}

const eryn = {
    compile: (path) => {
        if(!(path && (typeof path === 'string' && !(path instanceof String))))
            throw `Invalid argument 'path' (expected: string | found: ${typeof(path)})`

        binding.compile(path);
    },
    compileDir: (dirPath, filters) => {
        if(!dirPath)
            dirPath = "";
        if(!(typeof dirPath === 'string' || (dirPath instanceof String)))
            throw `Invalid argument 'dirPath' (expected: string | found: ${typeof(dirPath)})`
        if(!(filters && (filters instanceof Array)))
            throw `Invalid argument 'filters' (expected: array | found: ${typeof(filters)})`

        binding.compileDir(dirPath, filters);
    },
    compileString: (alias, str) => {
        if(!(alias && (typeof alias === 'string' && !(alias instanceof String))))
            throw `Invalid argument 'alias' (expected: string | found: ${typeof(path)})`
        if(!(str && (typeof str === 'string' && !(str instanceof String))))
            throw `Invalid argument 'str' (expected: string | found: ${typeof(path)})`

        binding.compileString(alias, str);
    },
    express: (path, context, callback) => {
        // Windows is like a girl with mood swings: sometimes it's /, other times it's \.
        // So let's not risk it.
        path = path.replace(/\\/g, '/');
        
        try {
            callback(null, eryn.render(path, context));
        } catch (error) {
            callback(error);
        }
    },
    render: (path, context) => {
        if(!(path && (typeof path === 'string' && !(path instanceof String))))
            throw `Invalid argument 'path' (expected: string | found: ${typeof(path)})`
        if(!context)
            context = {};
        if(!(typeof context === 'object'))
            throw `Invalid argument 'context' (expected: object | found: ${typeof(context)})`

        return binding.render(path, context, {}, bridgeEval, bridgeOptions.enableDeepCloning ? bridgeDeepClone : bridgeShallowClone);
    },
    renderString: (alias, context) => {
        if(!(alias && (typeof alias === 'string' && !(alias instanceof String))))
            throw `Invalid argument 'alias' (expected: string | found: ${typeof(path)})`
        if(!context)
            context = {};
        if(!(typeof context === 'object'))
            throw `Invalid argument 'context' (expected: object | found: ${typeof(context)})`

        return binding.renderString(alias, context, {}, bridgeEval, bridgeOptions.enableDeepCloning ? bridgeDeepClone : bridgeShallowClone);
    },
    setOptions: (options) => {
        if(!(options && (typeof options === 'object')))
            throw `Invalid argument 'options' (expected: object | found: ${typeof(options)})`

        if(options.hasOwnProperty("enableDeepCloning") && ((typeof options.enableDeepCloning) === "boolean"))
            bridgeOptions.enableDeepCloning = options.enableDeepCloning;

        binding.setOptions(options);
    }
};

module.exports = eryn;
module.exports.compile = eryn.compile;
module.exports.compileDir = eryn.compileDir;
module.exports.compileString = eryn.compileString;
module.exports.express = eryn.express;
module.exports.render = eryn.render;
module.exports.renderString = eryn.renderString;
module.exports.setOptions = eryn.setOptions;
