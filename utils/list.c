/*
    This file is part of CrabClaw.

    Copyright (C) 2008, 2009 Lawrence Sebald

    CrabClaw is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    CrabClaw is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CrabClaw; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "list.h"

#include <stdlib.h>

cc_dlist_t *cc_dlist_create(void)   {
    cc_dlist_t *rv = (cc_dlist_t *)malloc(sizeof(cc_dlist_t));

    if(!rv) {
        return NULL;
    }

    rv->head = rv->tail = NULL;
    rv->count = 0;

    return rv;
}

void cc_dlist_destroy(cc_dlist_t *list) {
    /* Remove any nodes still in the list. The user is responsible for freeing
       any data associated with the nodes. */
    while(list->head)   {
        cc_dlist_remove(list, list->head);
    }

    free(list);
}

int cc_dlist_remove(cc_dlist_t *list, cc_dlist_node_t *node)   {
    /* Make sure the node and list are valid. */
    if(!list || !node)  {
        return -1;
    }

    if(list->head == node)  {
        /* Case 1: The node is the head node, advance the head to the next
           node in the list. */
        list->head = node->next;
    }

    if(list->tail == node) {
        /* Case 2: The node is the tail node, point the tail pointer at the
           previous node in the list. If this is the only item in the list, this
           can happen at the same time as case 1 above. */
        list->tail = node->prev;
    }
    else    {
        /* Case 3: The node is in the middle of the list. Remove the node from
           the chain and readjust some pointers. */
        node->prev->next = node->next;
        node->next->prev = node->prev;

        free(node);
    }

    free(node);
    --list->count;

    return 0;
}

int cc_dlist_remove_at(cc_dlist_t *list, uint32 pos)   {
    cc_dlist_node_t *node = list->head;

    /* Make sure the user has passed us a valid position to remove. */
    if(!list || list->count <= pos) {
        return -1;
    }

    /* Grab the node that the user has requested removed. */
    while(pos-- && node != NULL)    {
        node = node->next;
    }

    /* We're guaranteed to have a node at this point, assuming the user hasn't
       screwed around with the internals of the list. The check at the beginning
       of cc_dlist_remove will take care of the case if the user has screwed
       with the innards. */
    return cc_dlist_remove(list, node);
}

int cc_dlist_insert_head(cc_dlist_t *list, void *data)  {
    cc_dlist_node_t *node;

    if(!list)   {
        return -1;
    }

    /* Make sure we allocate memory properly. */
    if(!(node = (cc_dlist_node_t *)malloc(sizeof(cc_dlist_node_t))))    {
        return -1;
    }

    /* Fill in the node structure. */
    node->data = data;
    node->prev = NULL;
    node->next = list->head;

    /* Update the head of the list and increment the list count. */
    if(list->head)  {
        list->head->prev = node;
    }

    list->head = node;

    if(!list->tail) {
        list->tail = node;
    }

    ++list->count;

    return 0;
}

int cc_dlist_insert_tail(cc_dlist_t *list, void *data)  {
    cc_dlist_node_t *node;

    if(!list)   {
        return -1;
    }

    /* Make sure we allocate memory properly. */
    if(!(node = (cc_dlist_node_t *)malloc(sizeof(cc_dlist_node_t))))    {
        return -1;
    }

    /* Fill in the node structure. */
    node->data = data;
    node->prev = list->tail;
    node->next = NULL;

    /* Update the tail of the list and increment the list count. */
    if(list->tail)  {
        list->tail->next = node;
    }

    list->tail = node;

    if(!list->head) {
        list->head = node;
    }

    ++list->count;

    return 0;
}

int cc_dlist_insert_after(cc_dlist_t *list, cc_dlist_node_t *node,
                          void *data)   {
    cc_dlist_node_t *n;

    if(!list || !node)  {
        return -1;
    }

    /* Make sure we allocate memory properly. */
    if(!(n = (cc_dlist_node_t *)malloc(sizeof(cc_dlist_node_t))))   {
        return -1;
    }

    /* Fill in the node structure. */
    n->data = data;
    n->prev = node;
    n->next = node->next;

    /* Update the node passed in and increment the list count. */
    node->next->prev = n;
    node->next = n;
    ++list->count;

    return 0;
}

void *cc_dlist_get_data_at(cc_dlist_t *list, int idx)   {
    cc_dlist_node_t *tmp;

    CC_DLIST_FOREACH(list, tmp) {
        if(!(idx--))
            return tmp->data;
    }

    return NULL;
}
