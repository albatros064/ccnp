#include "container.h"

#include <stdlib.h>
#include <stdint.h>

void _lst_increase_size(list *_list) {
	_list->_data = (void **) realloc(_list->_data, sizeof(void *) * (_list->_capacity + _list->_quantum) );
	_list->_capacity += _list->_quantum;
}


list *lst_create(container_free free, uint16_t quantum) {
	list *_list;

	if (quantum < 1) {
		quantum = 8;
	}

	_list = (list *) malloc(sizeof(list) );

	_list->count     = 0;
	_list->_data     = 0;
	_list->_capacity = 0;
	_list->_quantum  = quantum;

	_list->_free     = free;

	_lst_increase_size(_list);

	return _list;
}
int8_t lst_destroy(list *_list) {
	int i;
	for (i = 0; i < _list->count; i++) {
		(_list->_free)(_list->_data[i]);
	}
	free(_list->_data);
	free(_list);

	return 0;
}

list_iterator *lst_iterator(list *_list) {
	list_iterator *iterator;

	iterator = (list_iterator *) malloc(sizeof(list_iterator) );
	iterator->_index = 0;
	iterator->_list = _list;

	return iterator;
}
int8_t lst_iterator_destroy(list_iterator *iterator) {
	free(iterator);
	return 0;
}

void *lst_index(list *_list, uint32_t offset) {
	if (offset < 0 || offset >= _list->count) {
		return 0;
	}

	return _list->_data[offset];
}
void *lst_head(list *_list) {
	if (_list->count > 0) {
		return _list->_data[0];
	}

	return 0;
}

void *lst_tail(list *_list) {
	if (_list->count > 0) {
		return _list->_data[_list->count - 1];
	}

	return 0;
}

void *lst_next(list_iterator *iterator) {
	if (iterator->_index >= iterator->_list->count) {
		return 0;
	}
	if (iterator->_index < 0) {
		iterator->_index = 0;
	}

	return iterator->_list->_data[iterator->_index++];
}

void *lst_prev(list_iterator *iterator) {
	if (iterator->_index <= 0) {
		return 0;
	}

	if (iterator->_index >= iterator->_list->count) {
		iterator->_index = iterator->_list->count - 1;
	}

	return iterator->_list->_data[iterator->_index--];
}

int8_t lst_is_end(list_iterator *iterator) {
	if (iterator->_index >= iterator->_list->count) {
		return 1;
	}

	return 0;
}

int8_t lst_insert(list *_list, void *_datum, uint32_t offset) {
	void *_dat_tmp;
	if (offset >= _list->count) {
		return lst_append(_list, _datum);
	}

	if (offset < 0) {
		offset = 0;
	}

	if (_list->count >= _list->_capacity) {
		_lst_increase_size(_list);
	}

	for (; offset <= _list->count; offset++) {
		_dat_tmp = _list->_data[offset];
		_list->_data[offset] = _datum;
		_datum = _dat_tmp;
	}

	_list->count++;

	return 0;
}

int8_t lst_splice(list *_list, list *_elements, uint32_t offset, uint32_t remove) {
	int32_t length_delta;
	uint32_t new_length;
	uint32_t i, j;
	void *datum;
	
	length_delta = _elements->count - remove;
	new_length = _list->count + length_delta;

	while (_list->_capacity < new_length) {
		_lst_increase_size(_list);
	}

	if (length_delta > 0) {
		for (i = new_length - 1; j > offset + _elements->count; i--) {
			_list->_data[i] = _list->_data[i - length_delta];
		}
	}
	else if (length_delta < 0) {
		for (i = offset + _elements->count; i < new_length; i++) {
			_list->_data[i] = _list->_data[i - length_delta];
		}
	}

	for (i = 0; i < _elements->count; i++) {
		_list->_data[i + offset] = _elements->_data[i];
	}

	_list->count = new_length;

	return 0;
}

int8_t lst_append(list *_list, void *_datum) {
	if (_list->count >= _list->_capacity) {
		_lst_increase_size(_list);
	}

	_list->_data[_list->count++] = _datum;

	return 0;
}

int8_t lst_prepend(list *_list, void *_datum) {
	return lst_insert(_list, _datum, 0);
}



hash_map *hmp_create(container_free free_key, container_free free_datum, container_compare compare, container_hash hash, uint16_t capacity) {
	hash_map *_map;
	int i;

	_map = (hash_map *) malloc(sizeof(hash_map) );

	_map->count       = 0;
	_map->_capacity   = capacity;
	_map->_buckets    = (struct _hash_map_pair **) malloc(sizeof(struct _hash_map_pair *) * capacity);

	_map->_free_key   = free_key;
	_map->_free_datum = free_datum;
	_map->_hash       = hash;
	_map->_compare    = compare;

	for (i = 0; i < capacity; i++) {
		_map->_buckets[i] = 0;
	}

	return _map;
}
int8_t hmp_destroy(hash_map *_map) {
	struct _hash_map_pair *bucket;
	struct _hash_map_pair *d_bucket;
	int i, j;

	for (i = 0; i < _map->_capacity; i++) {
		bucket = _map->_buckets[i];

		while (bucket) {
			d_bucket = bucket->_next;
			(_map->_free_key)(bucket->_pair.key);
			(_map->_free_datum)(bucket->_pair.datum);
			free(bucket);
			bucket = d_bucket;
		}
	}

	free(_map->_buckets);
	free(_map);

	return 0;
}

void *hmp_get(hash_map *_map, void *_key) {
	struct _hash_map_pair *node;
	uint32_t hash;

	hash = (_map->_hash)(_key, _map->_capacity);

	node = _map->_buckets[hash];

	while (node) {
		if ( (_map->_compare)(node->_pair.key, _key) == 0) {
			return node->_pair.datum;
		}
		node = node->_next;
	}

	return 0;
}

struct _hash_map_pair *_hash_map_new(void *_key, void *_datum) {
	struct _hash_map_pair *node;
	node = (struct _hash_map_pair *) malloc(sizeof(struct _hash_map_pair) );

	node->_pair.key   = _key;
	node->_pair.datum = _datum;
	node->_next  = 0;

	return node;
}

int8_t hmp_put(hash_map *_map, void *_key, void *_datum) {
	struct _hash_map_pair **node;
	uint32_t hash;

	hash = (_map->_hash)(_key, _map->_capacity);

	node = &(_map->_buckets[hash]);

	while (*node) {
		if ( (_map->_compare)( (*node)->_pair.key, _key) == 0) {
			// We found the key
			(*node)->_pair.datum = _datum;
			return 1;
		}
		node = &( (*node)->_next);
	}

	*node = _hash_map_new(_key, _datum);

	return 0;
}

int8_t hmp_remove(hash_map *_map, void *_key) {
	
}

hash_map_iterator *hmp_iterator(hash_map *map) {
	hash_map_iterator *iterator;

	iterator = (hash_map_iterator *) malloc(sizeof(hash_map_iterator) );

	iterator->_map = map;
	iterator->_index = -1;
	iterator->_next = 0;

	return iterator;
}

hash_map_pair *hmp_next(hash_map_iterator *iterator) {
	struct _hash_map_pair *node;

	node = iterator->_next;

	if (node) {
		iterator->_next = node->_next;
		return &(node->_pair);
	}

	for (; ++(iterator->_index) < iterator->_map->_capacity;) {
		node = iterator->_map->_buckets[iterator->_index];
		if (node) {
			iterator->_next = node->_next;
			return &(node->_pair);
		}
	}

	iterator->_next = 0;

	return 0;
}
