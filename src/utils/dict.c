#include "dict.h"

#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct dict *init_dict_with_argv(char *key, char **value)
{
    struct dict *d = calloc(1, sizeof(struct dict));
    d->key = calloc(strlen(key) + 1, sizeof(char));
    strcpy(d->key, key);
    d->value = NULL;
    d->next = NULL;
    d->argv = value;

    // init $# (argc)
    int argc = 0;
    while (value[argc] != 0)
        argc++;
    char hash[200] = {
        0,
    };
    if (argc > 1)
        sprintf(hash, "%i", argc - 1);
    else
        sprintf(hash, "%i", 0);
    char hash_key[3] = { 0 };
    hash_key[0] = '#';
    d = dict_add(d, hash_key, hash);

    return d;
}

static struct dict *init_dict(char *key, char *value)
{
    struct dict *d = calloc(1, sizeof(struct dict));
    d->key = calloc(strlen(key) + 1, sizeof(char));
    strcpy(d->key, key);
    d->value = calloc((value == NULL) ? 1 : strlen(value) + 1, sizeof(char));
    if (value != NULL)
        strcpy(d->value, value);
    d->next = NULL;
    d->argv = NULL;
    d->cmd = NULL;
    return d;
}

static struct dict *init_dict_func(char *key, struct node *cmd)
{
    struct dict *d = calloc(1, sizeof(struct dict));
    d->key = calloc(strlen(key) + 1, sizeof(char));
    strcpy(d->key, key);
    d->cmd = cmd;
    d->next = NULL;
    d->argv = NULL;
    d->value = NULL;
    return d;
}

struct dict *dict_add(struct dict *d, char *key, char *value)
{
    if (d == NULL)
        return init_dict(key, value);
    struct dict *ret = d;
    while (1)
    {
        if (strcmp(d->key, key) == 0)
        {
            free(d->value);
            if (value != NULL)
            {
                d->value = calloc(strlen(value) + 1, sizeof(char));
                strcpy(d->value, value);
            }
            else
            {
                d->value = NULL;
            }
            return ret;
        }
        if (d->next == NULL)
        {
            d->next = init_dict(key, value);
            return ret;
        }
        d = d->next;
    }
}

struct dict *dict_add_func(struct dict *d, char *key, struct node *cmd)
{
    if (d == NULL)
        return init_dict_func(key, cmd);
    struct dict *ret = d;
    while (1)
    {
        if (strcmp(d->key, key) == 0)
        {
            free_node(d->cmd);
            d->cmd = cmd;
            return ret;
        }
        if (d->next == NULL)
        {
            d->next = init_dict_func(key, cmd);
            return ret;
        }
        d = d->next;
    }
}

struct dict *dict_pop(struct dict *d, char *key)
{
    if (d == NULL)
        return NULL;
    struct dict *ret = d;
    if (strcmp(d->key, key) == 0)
    {
        ret = d->next;
        free(d);
        return ret;
    }
    while (d->next != NULL)
    {
        if (strcmp(d->next->key, key))
        {
            struct dict *tmp = d->next;
            d->next = d->next->next;
            free(tmp);
            return ret;
        }
    }
    return NULL;
}

char *dict_lookup(struct dict *d, char *key)
{
    if (d == NULL)
        return NULL;
    if (strlen(key) >= 1 && isdigit(key[0]))
    {
        if (d->argv == NULL)
        {
            return NULL;
        }
        return d->argv[key[0] - '0'];
    }
    while (d != NULL)
    {
        if (strcmp(d->key, key) == 0)
        {
            return d->value;
        }
        d = d->next;
    }
    return NULL;
}

struct node *dict_lookup_func(struct dict *d, char *key)
{
    if (d == NULL)
        return NULL;
    while (d != NULL)
    {
        if (strcmp(d->key, key) == 0)
            return d->cmd;
        d = d->next;
    }
    return NULL;
}

/*
void dict_print(struct dict *d)
{
    printf("DICTIONARY:\n");
    if (d == NULL)
        printf("(null)\n");
    while (d != NULL)
    {
        if (d->argv != NULL)
        {
            for (int i = 0; d->argv[i] != 0; i++)
                printf("%s ", d->argv[i]);
            printf("\n");
        }
        else
        {
            if (d->cmd != NULL)
            {
                printf("\'%s\' -> func()\n", d->key);
            }
            else
                printf("\'%s\' -> \'%s\'\n", d->key, d->value);
        }
        d = d->next;
    }
}
*/

void free_dict(struct dict *d)
{
    while (d != NULL)
    {
        struct dict *next = d->next;
        free_node(d->cmd);
        free(d->value);
        free(d->key);
        free(d);
        d = next;
    }
}
