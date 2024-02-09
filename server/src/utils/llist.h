#ifndef __UTIL_LLIST_H__
#define __UTIL_LLIST_H__

#include <stdbool.h>
#include <stdlib.h>

typedef struct node {
    void *data;
    struct node *next;
    bool free;
} llnode_t;

typedef struct llist {
    llnode_t *first;
    llnode_t *last;
} llist_t;

/**
 * Initialise empty list. Use like this: 
 *      llist_t linked_list;
 *      llist_init(&linked_list);
 */
llist_t* llist_new();

/**
 * Function appends new node with data to the list. 
 * If the data needs to be freed in destruction, set free_data to true 
 */
int llist_append(llist_t *list, void *data, bool free_data);

/**
 * Destroys list by freeing all nodes, the list, and setting the list to NULL
 */
void llist_destroy(llist_t **list);

llnode_t* llist_get_index(llist_t *list, int index);

/* Clears the list of all the nodes without destroying the list itself */
void llist_clear(llist_t *list);

/** 
 * Convenience macro. Each node in a list is retrieved and CODE is ran with it.
 * llnode_t *node is type and identificator of the variable to work with 
 */
#define llist_foreach(LLIST, CODE){\
    size_t _i;                                                                                     \
    llnode_t *node;                                                                                \
    for (_i = 0, node = llist_get_index(LLIST, 0); node; node = llist_get_index(LLIST, ++_i)) {    \
        CODE                                                                                       \
    }}

#endif // !__UTIL_LLIST_H__

