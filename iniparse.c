/**
 * @file iniparse.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-06-10
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "iniparse.h"

#ifndef MAX_LINE_BUFFER
#define MAX_LINE_BUFFER 256
#endif

static struct section_tree *__get_section(const struct ini_tree *ini, const char *name, size_t len)
{
	struct section_tree *s;

	list_for_each_entry(s, &ini->head, node) {
		if (s->namelen != len)
			continue;
		if (!memcmp(s->name, name, len))
			return s;
	}
	return NULL;
}

struct key_value_pair *__get_key(const struct section_tree *section, const char *key, size_t keylen)
{
	struct key_value_pair *pair;

	list_for_each_entry(pair, &section->head, node) {
		if (pair->keylen != keylen)
			continue;
		if (!memcmp(pair->key, key, keylen))
			return pair;
	}
	return NULL;
}

static char *__get_value(const struct section_tree *section, const char *key, size_t keylen)
{
	struct key_value_pair *pair;

	pair = __get_key(section, key, keylen);
	if (pair)
		return pair->value;

	return NULL;
}

void ini_tree_init(struct ini_tree *ini, int flags)
{
	INIT_LIST_HEAD(&ini->head);
	ini->flags = flags;
}

void ini_tree_destroy(struct ini_tree *ini)
{
	struct section_tree *s;
	struct section_tree *tmp_s;

	list_for_each_entry_safe(s, tmp_s, &ini->head, node) {
		struct key_value_pair *pair;
		struct key_value_pair *tmp_p;
		list_for_each_entry_safe(pair, tmp_p, &s->head, node) {
			list_del(&pair->node);
			free(pair->key);
			free(pair->value);
			free(pair);
		}
		list_del(&s->node);
		free(s->name);
		free(s);
	}
}

int ini_tree_load(const char *path, struct ini_tree *ini, int *errlineno)
{
	FILE *file;
	char c, *line;
	int sizeofline = MAX_LINE_BUFFER;
	int j = 0, ret = 0;
	int section_added = 0;
	int key_added = 0;
	int len;

	int flags = 0;
#define FLAG_SECTION (1 << 0)
#define FLAG_COMMENT (1 << 1)
#define FLAG_S_QUOTE (1 << 2)
#define FLAG_D_QUOTE (1 << 3)
#define FLAG_SEC_END (1 << 4)

	struct key_value_pair *tmp_kv = NULL;
	struct section_tree *tmp_s = NULL;

	file = fopen(path, "r");
	if (file == NULL)
		return -ENOENT;

	line = malloc(MAX_LINE_BUFFER);
	if (line == NULL) {
		fclose(file);
		return -errno;
	}

	if (errlineno)
		*errlineno = 1;

	for (int i = 0; (ret = fread(&c, 1, 1, file)) == 1; i++) {
		// Skip non-printable characters (except new line character)
		if ((c < ' ' || c > '~') && (c != '\n'))
			continue;

		if (j >= (sizeofline - 1)) {
			char *tmp;
			sizeofline += MAX_LINE_BUFFER;
			tmp = realloc(line, sizeofline);
			if (tmp == NULL)
				goto error;
			line = tmp;
		}

		// Space character only being parse in quotes
		if (!(flags & (FLAG_S_QUOTE | FLAG_D_QUOTE))) {
			if (c == ' ')
				continue;
			if (c == '\'') {
				if (j != 0) {
					ret = -EINVAL;
					goto error;
				}
				flags |= FLAG_S_QUOTE;
				continue;
			}
			if (c == '"') {
				if (j != 0)
				{
					ret = -EINVAL;
					goto error;
				}
				flags |= FLAG_D_QUOTE;
				continue;
			}
		}
		else {
			if ((flags & FLAG_S_QUOTE) && c == '\'') {
				flags &= ~FLAG_S_QUOTE;
				continue;
			}
			if ((flags & FLAG_D_QUOTE) && c == '"') {
				flags &= ~FLAG_D_QUOTE;
				continue;
			}
			goto parse_next_char;
		}

		// End of line character
		if (c == ';' || c == '#' || c == '\n') {
			if (c != '\n') {
				if (flags & FLAG_SECTION) {
					ret = -EINVAL;
					goto error;
				}
				flags |= FLAG_COMMENT;
				continue;
			}

			if (tmp_s == NULL)
				goto parse_next_line;

			if (tmp_kv == NULL) {
				if (flags & FLAG_SEC_END)
					goto parse_next_line;

				ret = -EINVAL;
				goto error;
			}

			if (!(ini->flags & INI_ALLOW_EMPTY_VALUE) && len == 0) {
				ret = -EINVAL;
				goto error;
			}

			len = j;
			tmp_kv->value = malloc(len + 1);
			if (tmp_kv->value == NULL)
				goto error;

			memcpy(tmp_kv->value, line, len);
			tmp_kv->value[len] = 0;

			if (key_added)
				key_added = 0;
			else
				list_add_tail(&tmp_kv->node, &tmp_s->head);
			tmp_kv = NULL;

		parse_next_line:
			j = 0;
			flags = 0;

			if (errlineno)
				(*errlineno)++;

			continue;
		}

		// Skip comment and end of section parsing
		if (flags & (FLAG_COMMENT | FLAG_SEC_END))
			continue;

		// Parse section
		if (c == '[' && j == 0 && !(flags & FLAG_SECTION)) {
			if (tmp_s) {
				if (section_added)
					section_added = 0;
				else
					list_add_tail(&tmp_s->node, &ini->head);
				tmp_s = NULL;
			}
			flags |= FLAG_SECTION;
			continue;
		}

		if (c == ']' && (flags & FLAG_SECTION) && tmp_s == NULL) {
			if (j == 0) {
				ret = -EINVAL;
				goto error;
			}

			len = j;
			tmp_s = __get_section(ini, line, len);
			if (tmp_s) {
				if (!(ini->flags & INI_ALLOW_SECTION_MERGE))
				{
					tmp_s = NULL;
					ret = -ENOTUNIQ;
					goto error;
				}

				section_added = 1;
				if (!list_is_first(&tmp_s->node, &ini->head))
					list_swap(ini->head.next, &tmp_s->node);
				flags |= FLAG_SEC_END;
				continue;
			}

			tmp_s = calloc(1, sizeof(*tmp_s));
			if (tmp_s == NULL)
				goto error;

			INIT_LIST_HEAD(&tmp_s->head);

			tmp_s->name = malloc(len + 1);
			if (tmp_s->name == NULL)
				goto error;

			memcpy(tmp_s->name, line, len);
			tmp_s->name[len] = 0;
			tmp_s->namelen = len;
			flags |= FLAG_SEC_END;
			continue;
		}

		if (flags & FLAG_SECTION)
			goto parse_next_char;

		if (tmp_s == NULL)
			continue;

		// Parse key and value
		if (c == '=') {
			if (j == 0) {
				ret = -EINVAL;
				goto error;
			}

			len = j;
			tmp_kv = __get_key(tmp_s, line, len);
			if (tmp_kv) {
				key_added = 1;
				if (!(ini->flags & INI_ALLOW_OVERWRITE_VALUE)) {
					tmp_kv = NULL;
					ret = -EEXIST;
					goto error;
				}

				if (tmp_kv->value) {
					free(tmp_kv->value);
					tmp_kv->value = NULL;
				}
				j = 0;
				continue;
			}

			tmp_kv = calloc(1, sizeof(*tmp_kv));
			if (tmp_kv == NULL)
				goto error;

			tmp_kv->key = malloc(len + 1);
			if (tmp_kv->key == NULL)
				goto error;

			memcpy(tmp_kv->key, line, len);
			tmp_kv->key[len] = 0;
			tmp_kv->keylen = len;
			j = 0;
			continue;
		}

	parse_next_char:
		line[j] = c;
		j++;
	}

	if (tmp_s == NULL) {
		free(line);
		fclose(file);
		return 0;
	}

	if (tmp_kv == NULL)
		goto add_last_section;

	len = j;
	tmp_kv->value = malloc(len + 1);
	if (tmp_kv->value == NULL)
		goto error;

	memcpy(tmp_kv->value, line, len);

	if (!key_added)
		list_add_tail(&tmp_kv->node, &tmp_s->head);

add_last_section:
	if (!section_added)
		list_add_tail(&tmp_s->node, &ini->head);

	free(line);
	fclose(file);

	return 0;

error:
	fclose(file);
	if (line)
		free(line);
	if (tmp_kv) {
		if (tmp_kv->key)
			free(tmp_kv->key);
		if (tmp_kv->value)
			free(tmp_kv->value);
		free(tmp_kv);
	}
	if (tmp_s) {
		struct key_value_pair *pair;
		list_for_each_entry_safe(pair, tmp_kv, &tmp_s->head, node) {
			list_del(&pair->node);
			free(pair->key);
			free(pair->value);
			free(pair);
		}
		if (tmp_s->name)
			free(tmp_s->name);
		if (section_added)
			list_del(&tmp_s->node);
		free(tmp_s);
	}
	ini_tree_destroy(ini);
	return (ret) ? ret : -errno;
}

struct section_tree *ini_tree_get_section(const struct ini_tree *ini, const char *name)
{
	return __get_section(ini, name, strlen(name));
}

char *ini_tree_get_value(const struct section_tree *section, const char *key)
{
	return __get_value(section, key, strlen(key));
}
