#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <alpm.h>
#include <jsmn.h>

#include "logger/logger.h"
#include "hashtable.h"

#include "aur_pkg_parse.h"

static jsmntok_t* find_next_sibling(jsmntok_t* current_token, jsmntok_t* token_end, jsmntok_t* hint) {
	int new_start = current_token -> end;
	for (jsmntok_t* i = hint == NULL ? current_token + 1 : hint; i < token_end; i++) {
		if (i -> start >= new_start) {
			return i;
		}
	}
	return NULL;
}

enum __update_type {
	NAME,
	VERSION,
	BASE_PKG
};

static char* __update_info_from_json(char* mutable_json_str, enum __update_type* __restrict__ update_type_out, jsmntok_t* prop_tok) {
	// Mutate the string
	char tmp_char = mutable_json_str[prop_tok -> end];
	mutable_json_str[prop_tok -> end] = '\0';

	char* prop_name = mutable_json_str + prop_tok -> start;

	if (strcmp(prop_name, "Name") == 0) {
		// Mutate the string
		mutable_json_str[prop_tok[1].end] = '\0';
		char* prop_value = mutable_json_str + prop_tok[1].start;

		*update_type_out = NAME;

		mutable_json_str[prop_tok -> end] = tmp_char;
		return prop_value;
	}

	if (strcmp(prop_name, "Version") == 0) {
		// Mutate the string
		mutable_json_str[prop_tok[1].end] = '\0';
		char* prop_value = mutable_json_str + prop_tok[1].start;

		*update_type_out = VERSION;

		mutable_json_str[prop_tok -> end] = tmp_char;
		return prop_value;
	}

	if (strcmp(prop_name, "PackageBase") == 0) {
		// Mutate the string
		mutable_json_str[prop_tok[1].end] = '\0';
		char* prop_value = mutable_json_str + prop_tok[1].start;

		*update_type_out = BASE_PKG;

		mutable_json_str[prop_tok -> end] = tmp_char;
		return prop_value;
	}

	mutable_json_str[prop_tok -> end] = tmp_char;
	return NULL;
}

jsmntok_t* find_and_update_pkg_info(char* mutable_json_str, hashtable_t* map, jsmntok_t* pkg_tok, jsmntok_t* token_end) {
	char* version_info = NULL;
	char* pkg_name     = NULL;
	char* base_pkg     = NULL;

	jsmntok_t* i;
	for (i = pkg_tok + 1; i != NULL && i < token_end; i = find_next_sibling(i, token_end, NULL)) {
		enum __update_type upd_type;
		char* info = __update_info_from_json(mutable_json_str, &upd_type, i);
		if (info == NULL) {
			info++;
			continue;
		}

		switch (upd_type) {
			case NAME:
				pkg_name = info;
				break;
			case VERSION:
				version_info = info;
				break;
			case BASE_PKG:
				base_pkg = info;
		}
		info++;
	}

	if (pkg_name == NULL) {
		return i;
	}

	struct hashtable_node* node = hashtable_find_inside_map(*map, pkg_name);

	node -> updated_ver  = version_info;
	node -> is_non_aur   = 0;
	node -> package_base = base_pkg;

	if (node -> installed_ver == NULL) {
		node -> update_type = UPGRADE;
	} else {
		int vercmp = alpm_pkg_vercmp(node -> installed_ver, node -> updated_ver);
		node -> update_type = vercmp == 0 ? UP_TO_DATE : (vercmp < 0 ? UPGRADE : DOWNGRADE);
	}

	return i;
}

void process_response_json(char* mutable_json_str, hashtable_t* map) {
	size_t json_str_len = strlen(mutable_json_str);
	jsmn_parser parser;
	jsmn_init(&parser);
	size_t n_tokens = jsmn_parse(&parser, mutable_json_str, json_str_len, NULL, 0);

	jsmn_init(&parser);

	jsmntok_t tokens[n_tokens];

	(void) jsmn_parse(&parser, mutable_json_str, json_str_len, tokens, n_tokens);

	jsmntok_t* token_end     = tokens + n_tokens;
	jsmntok_t* results_token = tokens;

	char results_found = 0;
	
	for (results_token = tokens + 1; results_token < token_end; results_token = find_next_sibling(results_token, token_end, NULL)) {
		if (results_token == NULL) {
			break;
		}
		char* key_name = mutable_json_str + results_token -> start;
		char  tmp_char = mutable_json_str[results_token -> end];
		mutable_json_str[results_token -> end] = '\0';

		if (strcmp(key_name, "results") == 0) {
			results_found = 1;
			mutable_json_str[results_token -> end] = tmp_char;
			break;
		}

		mutable_json_str[results_token -> end] = tmp_char;
		results_token++;
	}

	if (!results_found) {
		(void) error_printf(" Cannot find the key \"results\" inside response JSON.\n");
		return;
	}

	results_token++;

	assert(results_token -> type == JSMN_ARRAY);

	results_token++;

	for (jsmntok_t* i = results_token, *hint = NULL; i != NULL && i < token_end; i = find_next_sibling(i, token_end, hint)) {
		hint = i;
		hint = find_and_update_pkg_info(mutable_json_str, map, i, token_end);
	}
}
