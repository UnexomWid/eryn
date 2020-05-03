var fs = require('fs');
var os = require('os');
var path = require('path');

const runtime = getRuntime();
const abi = process.versions.modules;
const platform = os.platform();
const arch = os.arch();

function load (dir, prebuildMatcher, prebuildDirBuilder) {
    return require(getPath(dir, prebuildMatcher, prebuildDirBuilder));
}

function getPath(dir, prebuildMatcher, prebuildDirBuilder) {
    dir = path.resolve(dir || '.');

    var release = findFirst(path.join(dir, 'build/Release'), matchBuild);
    if (release)
        return release;

    var debug = findFirst(path.join(dir, 'build/Debug'), matchBuild);
    if (debug)
        return debug;

    var prebuild = findFirst(path.join(dir, (prebuildDirBuilder || buildDir)(runtime, abi, platform, arch)), prebuildMatcher || matchNapiPrebuild);
    if (prebuild)
        return prebuild;

    throw new Error(`No native build was found for runtime ${runtime} | ABI ${abi} | ${platform} platform | arch ${arch}`);
}

function buildDir(runtime, abi, platform, arch) {
    return `prebuilds/${platform}-${arch}`;
}

function matchNapiPrebuild(name) {
    let str = name.split('.')
    return str[0] === runtime && str[1] === 'napi' && str[2] == 'node';
}

function findFirst(dir, filter) {
    try {
        let files = fs.readdirSync(dir).filter(filter);
        return files[0] && path.join(dir, files[0])
    } catch (err) {
        return null;
    }
}

function matchBuild(name) {
    return name.endsWith('.node');
}

function getRuntime() {
    if(process.versions && process.versions.electron)
        return 'electron';
    return (typeof window !== 'undefined'
            && window.process
            && window.process.type === 'renderer') ?
                'electron' : 'node';
}

module.exports = load;