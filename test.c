#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "iniparse.h"

void usage(char *progname)
{
	printf("%s <file>\n", progname);
}

void print_ini_tree(struct ini_tree *ini)
{
	struct section_tree *s;
	struct key_value_pair *p;

	printf("{");
	list_for_each_entry(s, &ini->head, node) {
		printf("\"%s\":{", s->name);
		list_for_each_entry(p, &s->head, node) {
			printf("\"%s\":\"%s\"", p->key, p->value);
			if (!list_entry_is_head(list_next_entry(p, node), &s->head, node))
				printf(",");
		}
		printf("}");
		if (!list_entry_is_head(list_next_entry(s, node), &ini->head, node))
				printf(",");
	}
	printf("}");
}

int main(int argc, char *argv[])
{
	struct ini_tree ini;
	int flags = 0;
	int errlineno, ret;

	if (argc < 2) {
		usage(argv[0]);
		return -1;
	}

	ini_tree_init(&ini, flags);

	ret = ini_tree_load(argv[1], &ini, &errlineno);

	if (ret)
		printf("Error: line %d\n%s.\n", errlineno, strerror(-ret));
	else
		print_ini_tree(&ini);

	return ret;
}