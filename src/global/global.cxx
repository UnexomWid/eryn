#include "global.hxx"
#include "cache.hxx"
#include "options.hxx"
#include "../../lib/bdp.hxx"

BDP::Header* Global::BDP832 = nullptr;

void Global::init() {
    Global::BDP832 = new BDP::Header(8, 32);

    Global::Options::restoreDefaults();
}

void Global::destroy() {
    printf("\nDestroying...\n");
    delete Global::BDP832;

    Global::Cache::destroy();
    Global::Options::destroy();
}

