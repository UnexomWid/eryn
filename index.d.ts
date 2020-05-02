// Type definitions for eryn 0.1.0
// Definitions by UnexomWid <https://uw.exom.dev>

export function compile(filePath : string) : void;
export function compileDir(dirPath : string, filters : string[]) : void;
export function render(filePath : string, context : object) : ArrayBuffer;
export function setOptions(options : object) : void;