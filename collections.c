#include "collections.h"

#include <stdlib.h>

linked_list *ll_create(char flags, collection_free free, collection_compare compare) {
    linked_list *list;
    list = (linked_list *) malloc(sizeof(linked_list) );

    list->head    = 0;
    list->tail    = 0;
    list->flags   = flags;
    list->free    = free;
    list->compare = compare;

    return list;
}

int ll_destroy(linked_list *list) {
    struct _linked_list_node *node;
    struct _linked_list_node *n;
    node = list->head;

    while (node) {
        (list->free)(node->data);
        n = node->tail;
        free(node);
        node = n;
    }
    
    free(list);

    return 0;
}

struct _linked_list_node *_ll_new(void *data) {
    struct _linked_list_node *new_node;

    new_node = (struct _linked_list_node *)  malloc(sizeof(struct _linked_list_node) );
    new_node->tail = 0;
    new_node->data = data;

    return new_node;
}

int ll_append(linked_list *list, void *data) {
    struct _linked_list_node *new_node;

    new_node = _ll_new(data);

    if (list->tail) {
        list->tail->tail = new_node;
    }

    list->tail = new_node;

    if (!list->head) {
        list->head = list->tail;
    }

    return 0;
}

int ll_prepend(linked_list *list, void *data) {
    struct _linked_list_node *new_node;

    new_node = _ll_new(data);

    if (list->head) {
        new_node->tail = list->head;
    }

    list->head = new_node;

    if (!list->tail) {
        list->tail = new_node;
    }

    return 0;
}

void *ll_find(linked_list *list, void *needle) {
    struct _linked_list_node *node;
    int result;

    node = list->head;

    while (node) {
        result = (list->compare)(node->data, needle);
        if (!result) {
            return node->data;
        }
        if (list->flags & COLLECTION_SORTED) {
            if (result > 0 && !(list->flags & COLLECTION_ASCEND) ) {
                return 0;
            }
            if (result < 0 && list->flags & COLLECTION_ASCEND) {
                return 0;
            }
        }

        node = node->tail;
    }

    return 0;
}

void *ll_get(linked_list *list, int offset) {
    struct _linked_list_node *node;
    int result;

    node = list->head;
    while (node && offset >= 0) {
        if (!offset) {
            return node->data;
        }

        offset--;
        node = node->tail;
    }

    return 0;
}

int ll_insert(linked_list *list, void *data) {
    struct _linked_list_node *last;
    struct _linked_list_node *node;
    unsigned char flags;
    int result;

    last = 0;
    flags = list->flags;

    if (!(flags & COLLECTION_SORTED) ) {
        if (flags & COLLECTION_ASCEND) {
            return ll_append(list, data);
        }
        return ll_prepend(list, data);
    }

    node = list->head;

    while (node) {
        result = (list->compare)(node->data, data);
        if (!result && flags & COLLECTION_UNIQUE) {
            if (flags & COLLECTION_RPLACE) {
                // TODO: Free old data?
                node->data = data;
                return 0;
            }
            return COLLECTION_ERROR_EXISTS;
        }
        if ( (result < 0 && flags & COLLECTION_ASCEND) || (result > 0 && !(flags & COLLECTION_ASCEND) ) ) {
            if (!last) {
                list->flags = 0;
                result = ll_prepend(list, data);
                list->flags = flags;
            }
            else {
                last->tail = _ll_new(data);
                last->tail->tail = node;
                result = 0;
            }

            return result;
        }

        last = node;
        node = node->tail;
    }
    list->flags = 0;
    result = ll_append(list, data);
    list->flags = flags;

    return result;
}

int ll_count(linked_list *list) {
    struct _linked_list_node *node;
    int count;

    count = 0;
    node = list->head;

    while (node) {
        count++;
        node = node->tail;
    }

    return count;
}










binary_tree *bt_create(unsigned char flags, collection_free free, collection_compare compare) {
    binary_tree *tree;
    tree = (binary_tree *) malloc(sizeof(binary_tree) );

    tree->root    = 0;
    tree->flags   = flags;
    tree->free    = free;
    tree->compare = compare;

    return tree;
}

int _bt_destroy(binary_tree *tree, struct _binary_tree_node *node) {
    if (!node) {
        return 0;
    }

    _bt_destroy(node->left );
    _bt_destroy(node->right);

    (tree->free)(node->data);
    free(tree);

    return 0;
}

int bt_destroy(binary_tree *tree) {
    _bt_destroy(tree, tree->root);
    free(tree);

    return 0;
}

int bt_insert(binary_tree *tree, void *data) {
    
}
