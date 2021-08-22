#include "engine.hxx"

Eryn::Options::Options() {
    flags.bypassCache              = false;
    flags.throwOnEmptyContent      = false;
    flags.throwOnMissingEntry      = false;
    flags.throwOnCompileDirError   = false;
    flags.ignoreBlankPlaintext     = false;
    flags.logRenderTime            = false;
    flags.cloneIterators           = false;
    flags.debugDumpOSH             = false;

    workingDir                     = ".";

    templates.escape               = '\\';
    templates.start                = "[|";
    templates.end                  = "|]";
    templates.bodyEnd              = "end";
    templates.voidStart            = "#";
    templates.commentStart         = "//";
    templates.commentEnd           = "//|]";
    templates.conditionalStart     = "?";
    templates.elseStart            = ":";
    templates.elseConditionalStart = ":?";
    templates.loopStart            = "@";
    templates.loopSeparator        = ":";
    templates.loopReverse          = "~";
    templates.componentStart       = "%";
    templates.componentSeparator   = ":";
    templates.componentSelf        = "/";
}