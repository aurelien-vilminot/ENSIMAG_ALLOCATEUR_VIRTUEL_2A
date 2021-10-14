/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

void *
emalloc_small(unsigned long size)
{
    if (arena.chunkpool == NULL || *((void **) arena.chunkpool) == NULL) {
        unsigned long block_size = mem_realloc_small();
        for (unsigned long i = 0; i < block_size - CHUNKSIZE; i += CHUNKSIZE) {
            // On écrit dans le bloc i...
            void **addr_to_write = arena.chunkpool;
            // ... l'adresse du bloc qui le suit (i + chunksize)
            *addr_to_write = (arena.chunkpool + CHUNKSIZE);

            arena.chunkpool += CHUNKSIZE;
        }
        // le dernier bloc pointe vers NULL
        *((void**) arena.chunkpool) = NULL;

        arena.chunkpool -= block_size - CHUNKSIZE;
    }
    void * head = arena.chunkpool;
    arena.chunkpool = *((void **) arena.chunkpool);
    return mark_memarea_and_get_user_ptr(head, CHUNKSIZE, SMALL_KIND);
}

void efree_small(Alloc a) {
    // On place en tête de arena.chunkpool notre bloc a.ptr
    // a.ptr.next = arena.chunkpool
    *((void**) a.ptr) = arena.chunkpool;
    // arena.chunkpool = a.ptr
    arena.chunkpool = a.ptr;
}
