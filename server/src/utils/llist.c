#include "llist.h"
#include "cclog_macros.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

llist_t* llist_new()
{
        llist_t *list = calloc(1, sizeof(llist_t));
        if_null(list, exit);
     
        list->last = NULL;
        list->last = NULL;
exit:
        return list;
}

int llist_append(llist_t *list, void *data, bool free_data)
{
        int rv = -1;

        if (!list || !data)
               return rv;

        llnode_t *node = calloc(1, sizeof(llnode_t));
        if_null(node, exit);

        node->data = data;
        node->free = free_data;
        node->next = NULL;

        if (!list->first) {
                list->first = node;
                list->last = node;
        } else {
                list->last->next = node;
                list->last = node;
        }

        rv = 0;
exit:
        return rv;
}

void llist_destroy(llist_t **list_ptr)
{
        if (!list_ptr || !(*list_ptr))
                return;

        llist_t *list = *list_ptr;

        llnode_t *node = list->first;
        llnode_t *tmp = list->first;
        while (node) {
                if (node->free)
                        free(node->data);
                
                tmp = node;
                node = node->next;
                free(tmp);
        }

        free(list);
        *list_ptr = NULL;
}

llnode_t* llist_get_index(llist_t *list, int index)
{
        if (!list || index < 0)
                return NULL;

        llnode_t *node = list->first;
        for (int i = 0; i < index ; i++) {
                if (node->next)
                        node = node->next;
                else 
                        return NULL;
        }

        return node;
}

