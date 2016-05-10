#ifndef CONTAINER_H
#define CONTAINER_H

#include <stdint.h>

typedef void     (*container_free   )(void *);
typedef int8_t   (*container_compare)(void *, void *);
typedef uint32_t (*container_hash   )(void *, uint32_t);

typedef struct {
    uint32_t count;

    void **_data;

    uint16_t _quantum;
    uint32_t _capacity;
    container_free    _free;
} list;

typedef struct {
    list *_list;
    uint32_t _index;
} list_iterator;

list *lst_create(container_free, uint16_t);
int8_t lst_destroy(list *);

list_iterator *lst_iterator(list *);
int8_t lst_iterator_destroy(list_iterator *);
void *lst_index(list *, uint32_t);
void *lst_head (list *);
void *lst_tail (list *);
void *lst_next (list_iterator *);
void *lst_prev (list_iterator *);
void *lst_peek (list_iterator *, uint32_t);
int8_t lst_is_end(list_iterator *);

int8_t lst_insert (list *, void *, uint32_t);
int8_t lst_splice (list *, list *, uint32_t, uint32_t);
int8_t lst_trim   (list *, uint32_t);
int8_t lst_append (list *, void *);
int8_t lst_prepend(list *, void *);


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

#endif

