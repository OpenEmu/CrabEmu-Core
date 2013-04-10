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

/* This linked list code was "ported" from CrabClaw. */

#ifndef CRABCLAW__LIST_H
#define CRABCLAW__LIST_H

#include "CrabEmu.h"

/**
 *  A node in a doubly-linked list.
 *  This object is used in a doubly-linked list to hold one data item.
 *  @since  1.0.0
 */
typedef struct cc_dlist_node {
    /**
     *  The previous node in the list.
     *  @since  1.0.0
     */
    struct cc_dlist_node *prev;

    /**
     *  The next node in the list.
     *  @since  1.0.0
     */
    struct cc_dlist_node *next;

    /**
     *  The data stored by this node.
     *  @since  1.0.0
     */
    void *data;
} cc_dlist_node_t;

/**
 *  A doubly-linked list.
 *  This object is used to hold a doubly-linked list of free-form data items.
 *  All fields in this structure should be considered to be read-only from
 *  outside of CrabClaw.
 *  @since  1.0.0
 */
typedef struct cc_dlist {
    /**
     *  The head of this list.
     *  @since  1.0.0
     */
    struct cc_dlist_node *head;

    /**
     *  The tail of this list.
     *  @since  1.0.0
     */
    struct cc_dlist_node *tail;

    /**
     *  The number of elements in this list.
     *  @since  1.0.0
     */
    uint32 count;
} cc_dlist_t;

/**
 *  Create a new doubly-linked list.
 *  Create and initialize a empty doubly-linked list.
 *  @since  1.0.0
 *  @return         A new <code>cc_dlist_t</code> object on success, NULL on
 *                  failure.
 */
extern cc_dlist_t *cc_dlist_create(void);

/**
 *  Destroy a doubly-linked list.
 *  Destroy an existing doubly-linked list. You are responsible for freeing any
 *  memory used by the data in the list. This function will free only the nodes
 *  used and the list object itself.
 *  @since  1.0.0
 *  @param  list    The list to destroy.
 */
extern void cc_dlist_destroy(cc_dlist_t *list);

/**
 *  Insert a new item at the head of the list.
 *  This function inserts a new node with the given data at the head of the
 *  given list.
 *  @since  1.0.0
 *  @param  list    The list to insert into.
 *  @param  data    The data to add to the list.
 *  @return         0 on success, non-zero on failure.
 */
extern int cc_dlist_insert_head(cc_dlist_t *list, void *data);

/**
 *  Insert a new item at the tail of the list.
 *  This function inserts a new node with the given data at the tail of the
 *  given list.
 *  @since  1.0.0
 *  @param  list    The list to insert into.
 *  @param  data    The data to add to the list.
 *  @return         0 on success, non-zero on failure.
 */
extern int cc_dlist_insert_tail(cc_dlist_t *list, void *data);

/**
 *  Insert a new item after a given node.
 *  This function inserts a new node with the given data after the node passed
 *  into the function.
 *  @since  1.0.0
 *  @param  list    The list to insert into.
 *  @param  node    The node to insert after.
 *  @param  data    The data to add to the list.
 *  @return         0 on success, non-zero on failure.
 */
extern int cc_dlist_insert_after(cc_dlist_t *list, cc_dlist_node_t *node,
                                 void *data);

/**
 *  Remove a given node from a list.
 *  This function removes the given node from the list. You are responsible for
 *  cleaning up any resources used by the data, this function only frees the
 *  memory used by the node itself.
 *  @since  1.0.0
 *  @param  list    The list to remove from.
 *  @param  node    The node to remove.
 *  @return         0 on success, non-zero on failure.
 */
extern int cc_dlist_remove(cc_dlist_t *list, cc_dlist_node_t *node);

/**
 *  Remove the element at a given position in a list.
 *  This function removes the element at a given position in the list passed in.
 *  You are responsible for cleaning up any resources used by the data in the
 *  node, this function only frees the memory used by the node itself.
 *  @since  1.0.0
 *  @param  list    The list to remove from.
 *  @param  pos     The element to remove (zero-based).
 *  @return         0 on success, non-zero on failure.
 */
extern int cc_dlist_remove_at(cc_dlist_t *list, uint32 pos);

void *cc_dlist_get_data_at(cc_dlist_t *list, int idx);

/**
 *  Loop through each element in a given list.
 *  This macro defines a "foreach" style loop for the given list.
 *  @since  1.0.0
 *  @param  list    The list to traverse.
 *  @param  tmp     The name of the temporary <code>cc_dlist_node_t</code>
 *                  pointer to use while traversing the list.
 */
#define CC_DLIST_FOREACH(list, tmp) \
    for(tmp = list->head; tmp != NULL; tmp = tmp->next)

#endif /* !CRABCLAW__LIST_H */
