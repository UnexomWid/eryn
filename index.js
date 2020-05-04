var binding = require('./build-load')(__dirname);

function bridgeEval(script, context) {
    return eval(script);
}

const eryn = {
    compile: (path) => {
        if(!(path && (typeof path === 'string' && !(path instanceof String))))
            throw `Invalid argument 'path' (expected: string | found: ${typeof(path)})`

        binding.compile(path);
    },
    compileDir: (dirPath, filters) => {
        if(!(dirPath && (typeof dirPath === 'string' && !(dirPath instanceof String))))
            throw `Invalid argument 'dirPath' (expected: string | found: ${typeof(dirPath)})`
        if(!(filters && (filters instanceof Array)))
            throw `Invalid argument 'filters' (expected: array | found: ${typeof(filters)})`

        binding.compileDir(dirPath, filters);
    },
    render: (path, context) => {
        if(!(path && (typeof path === 'string' && !(path instanceof String))))
            throw `Invalid argument 'path' (expected: string | found: ${typeof(path)})`
        if(!context)
            context = {};
        if(!(typeof context === 'object'))
            throw `Invalid argument 'context' (expected: object | found: ${typeof(context)})`

        return binding.render(path, context, bridgeEval);
    },
    setOptions: (options) => {
        if(!(options && (typeof options === 'object')))
            throw `Invalid argument 'options' (expected: object | found: ${typeof(options)})`

        binding.setOptions(options);
    }
};

module.exports = {
    compile: eryn.compile,
    compileDir: eryn.compileDir,
    render: eryn.render,
    setOptions: eryn.setOptions,
    default: eryn
};