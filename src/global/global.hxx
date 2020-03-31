#ifndef SERA_GLOBAL_HXX_GUARD
#define SERA_GLOBAL_HXX_GUARD

#include "../../lib/bdp.hxx"

namespace Global {

    extern BDP::Header* BDP832;

    void init();
    void destroy();
}

#endif