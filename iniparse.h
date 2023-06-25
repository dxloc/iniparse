/**
 * @file iniparse.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-10
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef INIPARSE_H
#define INIPARSE_H

#include "list.h"

struct key_value_pair {
	struct list_head node;
	char *key;
	char *value;
	int keylen;
};

struct section_tree {
	struct list_head node;
	struct list_head head;
	char *name;
	int namelen;
};

struct ini_tree {
	struct list_head head;
	int flags;
};

/* Flag definition */
#define INI_ALLOW_EMPTY_VALUE     (1 << 0) /* Allow keys with no value */
#define INI_ALLOW_OVERWRITE_VALUE (1 << 1) /* Allow overwrite a key value */
#define INI_ALLOW_SECTION_MERGE   (1 << 2) /* Allow merging multiple section definitions */

/**
 * @brief Initialize an INI tree
 */
void ini_tree_init(struct ini_tree *ini, int flags);

/**
 * @brief Destroy an initialized INI tree
 */
void ini_tree_destroy(struct ini_tree *ini);

/**
 * @brief Load an INI file to the tree
 * 
 * @param path INI file path
 * @param ini The INI tree
 * @param errlineno If not NULL, indicate error line number in the INI file
 * 
 * @return 0 if success
 */
int ini_tree_load(const char *path, struct ini_tree *ini, int *errlineno);

/**
 * @brief Search a section by name in the ini_tree
 * 
 * @return Section pointer, NULL if not found
 */
struct section_tree *ini_tree_get_section(const struct ini_tree *ini, const char *name);

/**
 * @brief Search a key in a section
 * 
 * @return Value of the key
 */
char *ini_tree_get_value(const struct section_tree *section, const char *key);

#endif
