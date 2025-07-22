#ifndef DICT_H
#define DICT_H

#include "../ast/node.h"

struct dict
{
    char *key;
    char *value;
    struct dict *next;
    char **argv;
    struct node *cmd;
}; // init to NULL

struct dict *init_dict_with_argv(char *key, char **value);

// updates value if key already exists
struct dict *dict_add(struct dict *d, char *key, char *value);
struct dict *dict_add_func(struct dict *d, char *key, struct node *cmd);

struct dict *dict_pop(struct dict *d, char *key);

// returns NULL is key not in dictionary
char *dict_lookup(struct dict *d, char *key);
struct node *dict_lookup_func(struct dict *d, char *key);

// void dict_print(struct dict *d);

void free_dict(struct dict *d);

#endif /* ! DICT_H */
