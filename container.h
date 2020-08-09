#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdint.h>

#define COLLECTION_UNIQUE 1
#define COLLECTION_RPLACE 2
#define COLLECTION_SORTED 4
#define COLLECTION_ASCEND 8

#define COLLECTION_ERROR_EXISTS 1

typedef void     (*container_free   )(void *);
typedef int8_t   (*container_compare)(void *, void *);
typedef uint32_t (*container_hash   )(void *, uint32_t);

void   null_free(void *);
int8_t null_compare(void *, void *);


/* ARRAY LIST */
typedef struct {
    uint32_t count;

    void **_data;

    uint16_t _quantum;
    uint32_t _capacity;
    container_free _free;
} list, stack;

typedef struct {
    list *_list;
    uint32_t _index;
} list_iterator;

list *lst_create(container_free, uint16_t);
int8_t lst_destroy(list *);

list_iterator *lst_iterator(list *);
int8_t lst_iterator_destroy(list_iterator *);
void  *lst_index(list *, uint32_t);
void  *lst_head (list *);
void  *lst_tail (list *);
int8_t lst_empty(list *);
void  *lst_next (list_iterator *);
void  *lst_prev (list_iterator *);
void  *lst_peek (list_iterator *, uint32_t);
void  *lst_seek (list_iterator *, uint32_t);
int8_t lst_is_end(list_iterator *);

int8_t  lst_insert (list *, void *, uint32_t);
int8_t  lst_splice (list *, list *, uint32_t, uint32_t);
int8_t  lst_trim   (list *, uint32_t);
int8_t  lst_append (list *, void *);
int8_t  lst_prepend(list *, void *);
#define lst_push   lst_append
void   *lst_pop    (list *);

#define st_create  lst_create
#define st_destroy lst_destroy
#define st_top     lst_tail
#define st_empty   lst_empty
#define st_push    lst_append
#define st_pop     lst_pop


/* HASH MAP */
typedef struct {
    void *key;
    void *datum;
} hash_map_pair;
struct _hash_map_pair {
    hash_map_pair _pair;
    struct _hash_map_pair *_next;
};
typedef struct {
    uint32_t count;
    struct _hash_map_pair **_buckets;
    uint32_t _capacity;

    container_free    _free_key;
    container_free    _free_datum;
    container_compare _compare;
    container_hash    _hash;
} hash_map;

typedef struct {
    hash_map *_map;
    uint32_t _index;
    struct _hash_map_pair *_next;
} hash_map_iterator;

hash_map *hmp_create(container_free, container_free, container_compare, container_hash, uint16_t);
int8_t hmp_destroy(hash_map *);
void  *hmp_get   (hash_map *, void *);
int8_t hmp_put   (hash_map *, void *, void *);
int8_t hmp_remove(hash_map *, void *);

hash_map_iterator *hmp_iterator(hash_map *);
hash_map_pair *hmp_next(hash_map_iterator *);


/* LINKED LIST */
struct _linked_list_node {
    void *data;
    struct _linked_list_node *tail;
};

typedef struct {
    struct _linked_list_node *head;
    struct _linked_list_node *tail;

    unsigned char flags;

    container_free    free;
    container_compare compare;
} linked_list;

linked_list *ll_create(unsigned char, container_free, container_compare);
int ll_destroy(linked_list *);
int ll_append (linked_list *, void *);
int ll_prepend(linked_list *, void *);
int ll_insert (linked_list *, void *);
int ll_count  (linked_list *);
void *ll_find (linked_list *, void *);


/* BINARY TREE */
typedef struct _binary_tree_node {
    void *data;
    struct _binary_tree_node *left;
    struct _binary_tree_node *right;
} binary_tree_node;
typedef struct _binary_tree {
    binary_tree_node *root;

    unsigned char flags;

    container_free    free;
    container_compare compare;
} binary_tree;

binary_tree *bt_create(unsigned char, container_free, container_compare);
int bt_destroy(binary_tree *);
int bt_insert (binary_tree *, void *);
int bt_count  (binary_tree *);
void *bt_find (binary_tree *, void *);

binary_tree_node *bt_node_create(binary_tree *);

#endif

