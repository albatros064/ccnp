#ifndef DATA_COLLECTIONS_H
#define DATA_COLLECTIONS_H


#define COLLECTION_UNIQUE 1
#define COLLECTION_RPLACE 2
#define COLLECTION_SORTED 4
#define COLLECTION_ASCEND 8

#define COLLECTION_ERROR_EXISTS 1

typedef void (*collection_free)(void*);
typedef int (*collection_compare)(void*,void*);


struct _linked_list_node {
    void *data;
    struct _linked_list_node *tail;
};

typedef struct {
    struct _linked_list_node *head;
    struct _linked_list_node *tail;

    unsigned char flags;

    collection_free    free;
    collection_compare compare;
} linked_list;

struct _binary_tree_node {
    void *data;
    struct _binary_tree_node *left;
    struct _binary_tree_node *right;
};
typedef struct {
    struct _binary_tree_node *root;

    unsigned char flags;

    collection_free    free;
    collection_compare compare;
} binary_tree;


/**
 * LINKED LIST
 */
linked_list *ll_create(unsigned char, collection_free, collection_compare);
int ll_destroy(linked_list *);
int ll_append (linked_list *, void *);
int ll_prepend(linked_list *, void *);
int ll_insert (linked_list *, void *);
int ll_count  (linked_list *);
void *ll_find (linked_list *, void *);

/**
 * BINARY TREE
 */
binary_tree *bt_create(unsigned char, collection_free, collection_compare);
int bt_destroy(binary_tree *);
int bt_insert (binary_tree *, void *);
int bt_count  (binary_tree *);
void *bt_find (binary_tree *, void *);

#endif

