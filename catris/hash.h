#ifndef HASH_H
#define HASH_H

#define TABLE_SIZE 100

typedef struct {
	char *key;
	int value;
} ht_item;

typedef struct {
	ht_item **items;
} ht_hash_table;

unsigned int hash_function(const char *str);
ht_item *create_item(const char *key, int value);
ht_hash_table *create_table();
void insert(ht_hash_table *table, const char *key, int value);
int search(ht_hash_table *table, const char *key);
void deleteEntry(ht_hash_table *table, const char *key);
void free_table(ht_hash_table *table);

#endif