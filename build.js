var nrp = require("npm-run-path");
var spawn = require("cross-spawn");
var argv = require("minimist")(process.argv.slice(2));

console.log(argv);

const runtime = argv["runtime"] || "node";
const target  = argv["target"]  || process.versions.node;
const abi     = argv["abi"]     || process.versions.modules;
const arch    = argv["arch"]    || process.arch;

console.log(`\n[BUILD] ${runtime} v${target} | ABI ${abi} | arch ${arch}`);

var ps = spawn("cmake-js", [
    "rebuild",
    "-r", runtime,
    "-v", target,
    "--abi", abi,
    "--arch", arch
], {
    env: nrp.env()
});

ps.stdout.pipe(process.stdout);
ps.stderr.pipe(process.stderr);

ps.on("exit", function (statusCode) {
    console.log("[BUILD] Done");
    process.exit(statusCode);
});