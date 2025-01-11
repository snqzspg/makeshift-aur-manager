#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hashtable.h"

struct hashtable_node {
	char* pkg_name;
	char* installed_ver;
	char* updated_ver;
	struct hashtable_node* next_node;
	char  is_non_aur;
};

typedef struct {
	struct hashtable_node** map;
	size_t map_size;

	struct hashtable_node*  nodes_stash;
	size_t nodes_stash_size;

	size_t n_items;
} hashtable_t;

size_t additive_hash(const char* s) {
	size_t r = 0;
	for (char* i = s; *i != '\0'; i++) {
		r += *i;
	}
	return r;
}

#define hashtable_init_stack_alloc(map_var_name, stash_var_name, size) struct hashtable_node* map_ver_name[size]; \
	struct hashtable_node stash_var_name[size];

hashtable_t hashtable_init(struct hashtable_node** map, size_t map_size, struct hashtable_node* nodes_stash, size_t nodes_stash_size) {
	hashtable_t new_map = {
		.map              = map,
		.map_size         = map_size,
		.nodes_stash      = nodes_stash,
		.nodes_stash_size = nodes_stash_size,
		.n_items          = 0
	};

	for (size_t i = 0; i < map_size; i++) {
		map[i] = NULL;
	}

	return new_map;
}

struct hashtable_node* hashtable_find_inside_map(hashtable_t map, const char* key) {
	size_t hash_no = additive_hash(key) % map.map_size;

	if (map.map[hash_no] == NULL) {
		return NULL;
	}

	for (struct hashtable_node* i = map.map[hash_no]; i != NULL; i = i -> next_node) {
		if (strcmp(key, i -> pkg_name) == 0) {
			return i;
		}
	}

	return NULL;
}

static void __unmap_node(hashtable_t* __restrict__ map, struct hashtable_node* node) {
	const char* key = node -> pkg_name;
	size_t hash_no = additive_hash(key) % map -> map_size;

	if (map -> map[hash_no] == node) {
		map -> map[hash_no] = node -> next_node;
		node -> next_node = NULL;
		return;
	}

	struct hashtable_node* prev_node = map -> map[hash_no];
	for (; prev_node -> next_node != node; prev_node = prev_node -> next_node) {
		if (prev_node -> next_node == NULL) {
			return;
		}
	}

	assert(prev_node -> next_node == node);
	prev_node -> next_node = node -> next_node;
	node -> next_node = NULL;
}

static void __remap_node(hashtable_t* __restrict__ map, struct hashtable_node* node) {
	const char* key = node -> pkg_name;
	size_t hash_no = additive_hash(key) % map -> map_size;

	struct hashtable_node* existing_mapped_node = hashtable_find_inside_map(*map, key);
	if (existing_mapped_node != NULL) {
		if (existing_mapped_node == node) {
			return;
		}

		__unmap_node(map, existing_mapped_node);
	}

	if (map -> map[hash_no] == NULL) {
		map -> map[hash_no] = node;
		node -> next_node = NULL;
		return;
	}

	struct hashtable_node* prev_node = map -> map[hash_no];
	for (; prev_node -> next_node != NULL; prev_node = prev_node -> next_node);

	prev_node -> next_node = node;
	node -> next_node = NULL;
}

void hashtable_set_item(hashtable_t* __restrict__ map, char* key, char* installed_ver, char* new_ver, char is_non_aur) {
	struct hashtable_node* existing_node = hashtable_find_inside_map(*map, key);

	if (existing_node == NULL) {
		if (map -> n_items >= map -> nodes_stash_size) {
			return;
		}

		existing_node = map -> nodes_stash + map -> n_items;
		map -> n_items++;
	}

	existing_node -> pkg_name      = key;
	existing_node -> installed_ver = installed_ver;
	existing_node -> updated_ver   = new_ver;
	existing_node -> is_non_aur    = is_non_aur;
}
