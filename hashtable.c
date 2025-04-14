/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright Yao Zi <ziyao@disroot.org> 2025.
 *
 * An open-addressing hashtable, optimized for indexing objects that carries
 * their own keys.
 */

#include "dtc.h"

static unsigned int hashtable_hash(const char *str, size_t len)
{
	unsigned int hash = (unsigned int)len;

	for (; len > 0; len--)
		hash ^= (hash << 5) + (hash >> 2) + str[len - 1];

	return hash;
}

/*
 * Must be 2^n to ensure masking the hash with (hashtable.cap - 1) results
 * in the slot index.
 */
#define hashtable_initcap	256

void hashtable_init(struct hashtable *table)
{
	size_t size = sizeof(struct hashtable_slot) * hashtable_initcap;

	*table = (struct hashtable) {
		.len	= 0,
		.cap	= hashtable_initcap,
		.slots	= xmalloc(size),
	};

	memset(table->slots, 0, size);
}

void hashtable_free(struct hashtable *table)
{
	free(table->slots);
	table->slots = NULL;
}

static unsigned int hashtable_findslot(struct hashtable_slot *slots,
				       unsigned int cap, unsigned int hash)
{
	unsigned i, mask;

	mask = cap - 1;

	for (i = hash & mask; slots[i].key; i = (i + 1) & mask) ;

	return i;
}

static void hashtable_grow(struct hashtable *table)
{
	unsigned int newcap = table->cap * 2;
	size_t newsize = sizeof(struct hashtable_slot) * newcap;
	struct hashtable_slot *newslots = xmalloc(newsize);
	unsigned int i;

	memset(newslots, 0, newsize);

	for (i = 0; i < table->cap; i++) {
		struct hashtable_slot *slot;
		unsigned hash, newslot;
		const char *key;

		slot = &table->slots[i];
		key = slot->key;
		if (!key)
			continue;

		hash = hashtable_hash(key, strlen(key));
		newslot = hashtable_findslot(newslots, newcap, hash);

		newslots[newslot] = *slot;
	}

	table->cap	= newcap;
	table->slots	= newslots;
}

void hashtable_append(struct hashtable *table, const char *key, void *value)
{
	unsigned int hash, i;
	size_t len;

	if (table->cap < table->len * 2)
		hashtable_grow(table);

	len	= strlen(key);
	hash	= hashtable_hash(key, len);

	i = hashtable_findslot(table->slots, table->cap, hash);
	table->slots[i] = (struct hashtable_slot) {
		.key	= key,
		.value	= value,
	};

	table->len++;
}

void *hashtable_get(struct hashtable *table, const char *key)
{
	struct hashtable_slot *slot;
	unsigned int hash, i, mask;
	size_t len;

	len	= strlen(key);
	mask	= table->cap - 1;
	hash	= hashtable_hash(key, len);

	for (i = hash & mask;
	     (slot = &table->slots[i])->key;
	     i = (i + 1) & mask) {
		if (streq(key, slot->key))
			return slot->value;
	}

	return NULL;
}
