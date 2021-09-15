/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <sys/mman.h>
#include <assert.h>
#include <stdint.h>
#include "mem.h"
#include "mem_internals.h"

// Droit ou pas ?
#include <string.h>

unsigned long knuth_mmix_one_round(unsigned long in)
{
    return in * 6364136223846793005UL % 1442695040888963407UL;
}

unsigned long magic_number(void *ptr, MemKind k) {
    unsigned long magic = knuth_mmix_one_round((unsigned long) ptr);
    return (magic & ~(0b11UL)) ^ ((unsigned long) k);
}

void *mark_memarea_and_get_user_ptr(void *ptr, unsigned long size, MemKind k)
{
    unsigned long magic = magic_number(ptr, k);
    size_t inc = sizeof(unsigned long);

    // Copier dans les 8o taille
    memcpy(ptr, &size, inc);
    // Copier dans les 8o (suivant taille) magic
    ptr += inc;
    memcpy(ptr, &magic, inc);
    // Copier dans les 8o (suivant delta) magic
    ptr += size - 3*inc;
    memcpy(ptr, &magic, inc);
    // Copier dans les 8o (suivant magic) taille
    ptr += inc;
    memcpy(ptr, &size, inc);
    // Positionner ptr sur la mémoire utilisateur
    ptr -= size - 3*inc;
    return ptr;
}

Alloc
mark_check_and_get_alloc(void *ptr)
{
    Alloc a = {};

    // Positionner pointeur sur taille
    size_t inc = sizeof(unsigned long);
    ptr -= 2*inc;
    a.ptr = ptr;
    // Copier dans a.size les 8o de ptr
    memcpy((void *) &a.size, ptr, inc);
    // Récupérer le nombre magique
    ptr += inc;
    unsigned long magic;
    memcpy((void *) &magic, ptr, inc);
    // Récupérer et vérifier MemKind
    a.kind = (MemKind) (magic & (0&11UL));
    // Vérification nombre magique
    assert(magic == magic_number(a.ptr, a.kind));
    // Vérification 16 premiers o = 16 derniers o
    unsigned long magic_end;
    unsigned long size_end;
    ptr += a.size - 3*inc;
    memcpy((void *) &magic_end, ptr, inc);
    ptr += inc;
    memcpy((void *) &size_end, ptr, inc);
    assert(magic == magic_end);
    assert(a.size == size_end);
    return a;
}


unsigned long
mem_realloc_small() {
    assert(arena.chunkpool == 0);
    unsigned long size = (FIRST_ALLOC_SMALL << arena.small_next_exponant);
    arena.chunkpool = mmap(0,
			   size,
			   PROT_READ | PROT_WRITE | PROT_EXEC,
			   MAP_PRIVATE | MAP_ANONYMOUS,
			   -1,
			   0);
    if (arena.chunkpool == MAP_FAILED)
	handle_fatalError("small realloc");
    arena.small_next_exponant++;
    return size;
}

unsigned long
mem_realloc_medium() {
    uint32_t indice = FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant;
    assert(arena.TZL[indice] == 0);
    unsigned long size = (FIRST_ALLOC_MEDIUM << arena.medium_next_exponant);
    assert( size == (1 << indice));
    arena.TZL[indice] = mmap(0,
			     size*2, // twice the size to allign
			     PROT_READ | PROT_WRITE | PROT_EXEC,
			     MAP_PRIVATE | MAP_ANONYMOUS,
			     -1,
			     0);
    if (arena.TZL[indice] == MAP_FAILED)
	handle_fatalError("medium realloc");
    // align allocation to a multiple of the size
    // for buddy algo
    arena.TZL[indice] += (size - (((intptr_t)arena.TZL[indice]) % size));
    arena.medium_next_exponant++;
    return size; // lie on allocation size, but never free
}


// used for test in buddy algo
unsigned int
nb_TZL_entries() {
    int nb = 0;
    
    for(int i=0; i < TZL_SIZE; i++)
	if ( arena.TZL[i] )
	    nb ++;

    return nb;
}
