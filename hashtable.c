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

void hashtable_init(struct hashtable *table, hashtable_tokey tokey, void *ctx)
{
	*table = (struct hashtable) {
		.tokey	= tokey,
		.ctx	= ctx,
		.len	= 0,
		.cap	= hashtable_initcap,
		.slots	= xmalloc(sizeof(void *) * hashtable_initcap),
	};

	memset(table->slots, 0, sizeof(void *) * hashtable_initcap);
}

void hashtable_free(struct hashtable *table)
{
	free(table->slots);
	table->slots = NULL;
}

static unsigned int hashtable_findslot(const void **slots, unsigned int cap,
				       unsigned int hash)
{
	unsigned i, mask;

	mask = cap - 1;

	for (i = hash & mask; slots[i]; i = (i + 1) & mask) ;

	return i;
}

static void hashtable_grow(struct hashtable *table)
{
	unsigned int newcap = table->cap * 2;
	const void **newslots = xmalloc(sizeof(void *) * newcap);
	unsigned int i;

	memset(newslots, 0, sizeof(void *) * newcap);

	for (i = 0; i < table->cap; i++) {
		unsigned hash, newslot;
		const void *value;
		const char *key;

		value = table->slots[i];
		if (!value)
			continue;

		key = table->tokey(table->ctx, value);
		hash = hashtable_hash(key, strlen(key));
		newslot = hashtable_findslot(newslots, newcap, hash);

		newslots[newslot] = value;
	}

	table->cap	= newcap;
	table->slots	= newslots;
}

void hashtable_set(struct hashtable *table, const char *key, const void *value)
{
	unsigned int hash, i;
	size_t len;

	if (table->cap < table->len * 2)
		hashtable_grow(table);

	len	= strlen(key);
	hash	= hashtable_hash(key, len);

	i = hashtable_findslot(table->slots, table->cap, hash);
	table->slots[i] = value;

	table->len++;
}

const void *hashtable_get(struct hashtable *table, const char *key)
{
	unsigned int hash, i, mask;
	const void *value;
	size_t len;

	len	= strlen(key);
	mask	= table->cap - 1;
	hash	= hashtable_hash(key, len);

	for (i = hash & mask; (value = table->slots[i]); i = (i + 1) & mask) {
		if (streq(key, table->tokey(table->ctx, value)))
			return value;
	}

	return NULL;
}
