var binding = require('./build-load')(__dirname);

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
    render: (path, context) => {
        if(!(path && (typeof path === 'string' && !(path instanceof String))))
            throw `Invalid argument 'path' (expected: string | found: ${typeof(path)})`
        if(!context)
            context = {};
        if(!(typeof context === 'object'))
            throw `Invalid argument 'context' (expected: object | found: ${typeof(context)})`

        return binding.render(path, context, {}, bridgeEval);
    },
    renderString: (alias, context) => {
        if(!(alias && (typeof alias === 'string' && !(alias instanceof String))))
            throw `Invalid argument 'alias' (expected: string | found: ${typeof(path)})`
        if(!context)
            context = {};
        if(!(typeof context === 'object'))
            throw `Invalid argument 'context' (expected: object | found: ${typeof(context)})`

        return binding.renderString(alias, context, {}, bridgeEval);
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
    compileString: eryn.compileString,
    render: eryn.render,
    renderString: eryn.renderString,
    setOptions: eryn.setOptions,
    default: eryn
};