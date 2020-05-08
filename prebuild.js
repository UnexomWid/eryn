var nrp = require("npm-run-path");
var spawn = require("cross-spawn");

console.log("[PREBUILD] Generating prebuilds");
console.log("[PREBUILD] Go get some coffee; this may take a while\n");

console.log("[PREBUILD] [ia32] Generating")

// This is a customized version; provide your own prebuild generator.
var ps32 = spawn("prebuildify", [
    "--napi",
    "--arch", "ia32",
], {
    env: nrp.env()
});

ps32.stdout.pipe(process.stdout);
ps32.stderr.pipe(process.stderr);

ps32.on("exit", function (statusCode) {
    if(statusCode != 0) {
        console.log("[PREBUILD] [ia32] Failed with status code " + statusCode);
        process.exit(statusCode);
    }

    console.log("[PREBUILD] [ia32] Done\n");
    console.log("[PREBUILD] [x64] Generating")

    var ps64 = spawn("prebuildify", [
        "--napi",
        "--arch", "x64",
    ], {
        env: nrp.env()
    });

    ps64.stdout.pipe(process.stdout);
    ps64.stderr.pipe(process.stderr);

    ps64.on("exit", function (statusCode) {
        if(statusCode != 0) {
            console.log("[PREBUILD] [x64] Failed with status code " + statusCode);
            process.exit(statusCode);
        }

        console.log("[PREBUILD] [x64] Done\n");
        console.log("[PREBUILD] Done");
        process.exit(statusCode);
    });
});