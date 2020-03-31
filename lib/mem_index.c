#include "mem_index.h"

void mem_lnchr(const uint8_t* source, size_t index, size_t* ln, size_t* chr) {
    register size_t  l = 1;
    register size_t  c = 1;
    register uint8_t current;

    uint8_t* limit = source + index;

    while(source < limit) {
        current = *(source++);

        if(current == '\n') {
            ++l;
            c = 1;
        } else if(current != '\r') {
            ++c;
        }
    }

    ln = l;
    chr = c;
}