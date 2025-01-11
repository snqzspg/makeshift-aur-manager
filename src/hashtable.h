#ifndef HASHTABLE_H_INCLUDED
#define HASHTABLE_H_INCLUDED

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

hashtable_t hashtable_init(struct hashtable_node** map, size_t map_size, struct hashtable_node* nodes_stash, size_t nodes_stash_size);
struct hashtable_node* hashtable_find_inside_map(hashtable_t map, const char* key);
void hashtable_set_item(hashtable_t* __restrict__ map, char* key, char* installed_ver, char* new_ver, char is_non_aur);

#endif // HASHTABLE_H_INCLUDED
