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
    if (arena.chunkpool == NULL) {
        unsigned long block_size = mem_realloc_small();
    }
    void * head = arena.chunkpool;
    arena.chunkpool += CHUNKSIZE;
    return mark_memarea_and_get_user_ptr(head, CHUNKSIZE, SMALL_KIND);
}

void efree_small(Alloc a) {
    /* ecrire votre code ici */
}
