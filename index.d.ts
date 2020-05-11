// Type definitions for eryn 0.2.0
// Definitions by UnexomWid <https://uw.exom.dev>

interface ErynOptions {
    bypassCache?:              bool,
    throwOnEmptyContent?:      bool,
    throwOnMissingEntry?:      bool,
    throwOnCompileDirError?:   bool,
    ignoreBlankPlaintext?:     bool,
    logRenderTime?:            bool,
    workingDirectory?:         string,
    templateEscape?:           string,
    templateStart?:            string,
    templateEnd?:              string,
    conditionalStart?:         string,
    conditionalEnd?:           string,
    invertedConditionalStart?: string,
    invertedConditionalEnd?:   string,
    loopStart?:                string,
    loopEnd?:                  string,
    loopSeparator?:            string,
    componentStart?:           string,
    componentEnd?:             string,
    componentSeparator?:       string,
    componentSelf?:            string
}

export function compile(filePath : string) : void;
export function compileDir(dirPath : string, filters : string[]) : void;
export function render(filePath : string, context : object) : Buffer;
export function setOptions(options : ErynOptions) : void;

declare const eryn: {
    compile: (filePath : string) => void;
    compileDir: (dirPath : string, filters : string[]) => void;
    render: (filePath : string, context : object) => Buffer;
    setOptions: (options : ErynOptions) => void;
};
  
export default eryn;