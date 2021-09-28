/******************************************************
 * Copyright Grégory Mounié 2018                      *
 * This code is distributed under the GLPv3+ licence. *
 * Ce code est distribué sous la licence GPLv3+.      *
 ******************************************************/

#include <stdint.h>
#include <assert.h>
#include "mem.h"
#include "mem_internals.h"

#define MIN(a,b) (((a)<(b))?(a):(b))

unsigned int puiss2(unsigned long size) {
    unsigned int indice_tzl=0;
    size = size -1; // allocation start in 0
    while(size) {  // get the largest bit
	indice_tzl++;
	size >>= 1;
    }
    if (size > (1 << indice_tzl))
	indice_tzl++;
    return indice_tzl;
}

void diviser_bloc(void* ptr, unsigned long size_ptr, unsigned long indice_ptr, unsigned long n) {    
    // tant que n > 0, diviser en deux ptr
    // et placer dans arena.TZL[indice_ptr - 1] la partie droite du bloc
    if (n == 0) {
        return;
    }
    void* bloc_libre = ptr + (size_ptr >> 1);
    // bloc_libre.next = arena.TZL[...]
    *((void **) bloc_libre) = arena.TZL[indice_ptr - 1];
    // arena.TZL[...] = bloc_libre
    arena.TZL[indice_ptr - 1] = bloc_libre;

    return diviser_bloc(ptr, (size_ptr >> 1), indice_ptr - 1, n - 1);
}


void *
emalloc_medium(unsigned long size)
{
    assert(size < LARGEALLOC);
    assert(size > SMALLALLOC);
    unsigned int indice_tzl = puiss2(size);

    // Cas 1: bloc libre à l'indice_tzl
    if (arena.TZL[indice_tzl] != NULL) {
        void* ptr = arena.TZL[indice_tzl];
        // memuser = ptr.next
        void* memuser = *((void **) ptr);
        // arena.TZL[indice_tzl].next = memuser.next
        *((void **) ptr) = *((void **) memuser);
        return mark_memarea_and_get_user_ptr(memuser, size, MEDIUM_KIND);
    }

    // Cas 2: on trouve un bloc libre, puis on le divise jusqu'à la taille qu'on veut
    unsigned int indice_libre = indice_tzl;
    while (indice_libre < FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant
            && arena.TZL[indice_libre] == NULL) {
                indice_libre++;
            }
    if (indice_libre == FIRST_ALLOC_MEDIUM_EXPOSANT + arena.medium_next_exponant) {
        mem_realloc_medium();
    }
    // on a nécessairement arena.TZL[indice_libre] qui contient un bloc libre
    // division récursive du bloc libre à cet indice
    void* ptr = arena.TZL[indice_libre];
    diviser_bloc(ptr, (1 << indice_libre), indice_libre, (indice_libre - indice_tzl));

    // l'adresse à utiliser est "ptr", de taille "size" (voir figure 2)
    return mark_memarea_and_get_user_ptr(ptr, size, MEDIUM_KIND);
}

void efree_medium(Alloc a) {
    // calcul buddy
    void* buddy = (void *) (((unsigned long) a.ptr) ^ a.size);

    // vérifier si buddy présent
    unsigned int indice_tzl = puiss2(a.size);
    void* ptr = arena.TZL[indice_tzl];
    void* old = NULL;
    while (ptr != NULL) {
        // Cas 1: buddy présent
        if (ptr == buddy) {
            // on enlève buddy
            if (old == NULL) {
                // arena.TZL[...] = ptr.next
                arena.TZL[indice_tzl] = *((void **) ptr);
            } else {
                // old.next = ptr.next
                *((void **) old) = *((void **) ptr);
            }
            // on recommence avec le bloc fusionné (de taille double)
            void* min_ptr = MIN(a.ptr, buddy);
            Alloc b = {min_ptr, MEDIUM_KIND, (a.size << 1)};
            return efree_medium(b);
        }

        // ptr = ptr.next
        old = ptr;
        ptr = *((void **) ptr);
    }
    // Cas 2: buddy pas présent
    // a.ptr.next = arena.TZL[...]
    *((void **) a.ptr) = arena.TZL[indice_tzl];
    // arena.TZL[...] = a.ptr
    arena.TZL[indice_tzl] = a.ptr;
}


