#include "global.hxx"
#include "cache.hxx"
#include "options.hxx"

#include "../../lib/bdp.hxx"
#include "../../lib/remem.hxx"

BDP::Header* Global::BDP832 = nullptr;

void Global::init() {
    Global::BDP832 = new("BDP832 Header") BDP::Header[1]{ { 8, 32 } };

    Global::Options::restoreDefaults();
}

void Global::destroy() {
    delete[] Global::BDP832;

    Global::Cache::destroy();
    Global::Options::destroy();
}

