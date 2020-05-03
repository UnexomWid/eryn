var binding = require('./build-load')(__dirname);

module.exports = {
    compile: binding.compile,
    compileDir: binding.compileDir,
    render: binding.render,
    setOptions: binding.setOptions,
    default: binding
};