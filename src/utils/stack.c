#include "stack.h"

#include <ctype.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct stack *stack_init(struct dict *value)
{
    struct stack *ret = calloc(1, sizeof(struct stack));
    ret->dict = value;
    ret->next = NULL;
    return ret;
}

struct stack *stack_push(struct stack *d, struct dict *value)
{
    if (d == NULL)
        return stack_init(value);
    else
    {
        struct stack *new = stack_init(value);
        new->next = d;
        return new;
    }
}

struct stack *stack_pop(struct stack *d)
{
    if (d == NULL)
        return NULL;
    else
    {
        struct stack *tmp = d->next;
        free_dict(d->dict);
        free(d);
        return tmp;
    }
}

struct dict *stack_peek(struct stack *d)
{
    if (d == NULL)
        return NULL;
    return d->dict;
}

struct dict *stack_lookup(struct stack *d, char *key)
{
    char *ret = NULL;
    if (strlen(key) >= 1 && isdigit(key[0]))
    {
        while (d != NULL)
        {
            struct dict *dic = d->dict;
            while (dic != NULL)
            {
                if (dic->argv != NULL)
                {
                    return d->dict;
                }
                dic = dic->next;
            }
            d = d->next;
        }
        return NULL;
    }
    while (d != NULL)
    {
        ret = dict_lookup(d->dict, key);
        if (ret != NULL)
            return d->dict;
        d = d->next;
    }
    return NULL;
}

// returns NULL is key not in stackionary
struct dict *stack_lookup_func(struct stack *d, char *key)
{
    struct node *ret = NULL;
    while (d != NULL)
    {
        ret = dict_lookup_func(d->dict, key);
        if (ret != NULL)
            return d->dict;
        d = d->next;
    }
    return NULL;
}

/*
void stack_print(struct stack *d)
{
    printf("===STACK===\n");
    while (d != NULL)
    {
        dict_print(d->dict);
        d = d->next;
        printf("---\n");
    }
}
*/

struct stack *stack_get_root(struct stack *d)
{
    if (d == NULL)
        return NULL;
    while (d->next != NULL)
        d = d->next;
    return d;
}

void free_stack(struct stack *d)
{
    while (d != NULL)
    {
        struct stack *tmp = d->next;
        free_dict(d->dict);
        free(d);
        d = tmp;
    }
}
