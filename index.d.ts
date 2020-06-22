// Type definitions for eryn 0.2.0
// Definitions by UnexomWid <https://uw.exom.dev>

interface ErynOptions {
    bypassCache?:              boolean,
    throwOnEmptyContent?:      boolean,
    throwOnMissingEntry?:      boolean,
    throwOnCompileDirError?:   boolean,
    ignoreBlankPlaintext?:     boolean,
    logRenderTime?:            boolean,
    enableDeepCloning?:        boolean,
    cloneIterators?:           boolean,
    workingDirectory?:         string,
    templateEscape?:           string,
    templateStart?:            string,
    templateEnd?:              string,
    bodyEnd?:                  string,
    commentStart?:             string,
    commentEnd?:               string,
    voidTemplate?:             string,
    conditionalStart?:         string,
    invertedConditionalStart?: string,
    loopStart?:                string,
    loopSeparator?:            string,
    loopReverse?:              string,
    componentStart?:           string,
    componentSeparator?:       string,
    componentSelf?:            string
}

export function compile(filePath : string) : void;
export function compileDir(dirPath : string, filters : string[]) : void;
export function compileString(alias : string, str : string) : void;
export function render(filePath : string, context : object) : Buffer;
export function renderString(alias : string, context : object) : Buffer;
export function setOptions(options : ErynOptions) : void;

declare const eryn: {
    compile: (filePath : string) => void;
    compileDir: (dirPath : string, filters : string[]) => void;
    compileString: (alias : string, str : string) => void;
    render: (filePath : string, context : object) => Buffer;
    renderString: (alias : string, context : object) => Buffer;
    setOptions: (options : ErynOptions) => void;
};
  
export default eryn;