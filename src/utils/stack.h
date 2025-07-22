#ifndef STACK_H
#define STACK_H

#include "../ast/node.h"
#include "dict.h"

struct stack
{
    struct dict *dict;
    struct stack *next;
}; // init to NULL

struct stack *stack_push(struct stack *d, struct dict *value);

struct stack *stack_pop(struct stack *d);

struct dict *stack_peek(struct stack *d);

// returns NULL if key not in stackionary
struct dict *stack_lookup(struct stack *d, char *key);
struct stack *stack_get_root(struct stack *d);
struct dict *stack_lookup_func(struct stack *d, char *key);

// void stack_print(struct stack *d);

void free_stack(struct stack *d);

#endif /* ! STACK_H */
