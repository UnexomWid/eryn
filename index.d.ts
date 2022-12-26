// Type definitions for eryn 0.3
// Definitions by UnexomWid <https://uw.exom.dev>

type Hook = (content: Buffer, origin:
    'plaintext'
    | 'template'
    | 'void'
    | 'comment'
    | 'conditional'
    | 'else_conditional'
    | 'loop_iterator'
    | 'loop_iterable'
    | 'component_path'
    | 'component_context') => Buffer;

interface ErynOptions {
    bypassCache?:              boolean,
    throwOnEmptyContent?:      boolean,
    throwOnMissingEntry?:      boolean,
    throwOnCompileDirError?:   boolean,
    ignoreBlankPlaintext?:     boolean,
    logRenderTime?:            boolean,
    enableDeepCloning?:        boolean,
    cloneIterators?:           boolean,
    debugDumpOSH?:             boolean,
    mode?:                     "normal" | "strict",
    workingDirectory?:         string,
    templateEscape?:           string,
    templateStart?:            string,
    templateEnd?:              string,
    bodyEnd?:                  string,
    commentStart?:             string,
    commentEnd?:               string,
    voidTemplate?:             string,
    conditionalStart?:         string,
    elseStart?:                string,
    elseConditionalStart?:     string,
    invertedConditionalStart?: string,
    loopStart?:                string,
    loopSeparator?:            string,
    loopReverse?:              string,
    componentStart?:           string,
    componentSeparator?:       string,
    componentSelf?:            string,
    compileHook?:              Hook
}

declare class ErynBinding {
    constructor(options: ErynOptions | undefined);
    compile(filePath: string): void;
    compileDir(dirPath: string, filters: string[]): void;
    compileString(alias: string, str: string): void;
    express(path: string, context: any, callback: (error: any, rendered: string) => void): void;
    render(filePath: string, context: any, shared: any): Buffer;
    renderString(alias: string, context: any, shared: any): Buffer;
    renderStringUncached(src: string, context: any, shared: any): Buffer;
    setOptions(options: ErynOptions): void;
}

declare function eryn(options: ErynOptions | undefined): ErynBinding;
  
export = eryn;
