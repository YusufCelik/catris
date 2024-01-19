#include <stdlib.h>
#include "hash.h"

#define TABLE_SIZE 100

unsigned int hash_function(const char *str) {
	unsigned long hash = 5381;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c;
	return hash % TABLE_SIZE;
}

ht_item *create_item(const char *key, int value) {
	ht_item *item = malloc(sizeof(ht_item));
	item->key = strdup(key);
	item->value = value;
	return item;
}

ht_hash_table *create_table() {
	ht_hash_table *table = malloc(sizeof(ht_hash_table));
	table->items = calloc(TABLE_SIZE, sizeof(ht_item*));
	return table;
}

void insert(ht_hash_table *table, const char *key, int value) {
	unsigned int index = hash_function(key);
	ht_item *item = create_item(key, value);
	if (table->items[index] != NULL) {
		free(table->items[index]);
	}
	table->items[index] = item;
}

int search(ht_hash_table *table, const char *key) {
	unsigned int index = hash_function(key);
	if (table->items[index] != NULL && strcmp(table->items[index]->key, key) == 0) {
		return table->items[index]->value;
	}
	return -1; // Not found
}

void deleteEntry(ht_hash_table *table, const char *key) {
	unsigned int index = hash_function(key);
	if (table->items[index] != NULL) {
		free(table->items[index]);
		table->items[index] = NULL;
	}
}

void free_table(ht_hash_table *table) {
	for (int i = 0; i < TABLE_SIZE; i++) {
		if (table->items[i]) {
			free(table->items[i]);
		}
	}
	free(table->items);
	free(table);
}
