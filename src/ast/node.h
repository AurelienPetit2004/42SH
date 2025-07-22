#ifndef NODE_H
#define NODE_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lexer/token.h"

enum node_type
{
    NODE_SIMPLE_COMMAND = 1,
    NODE_COMMAND,
    NODE_ELEMENT,
    NODE_IF,
    NODE_LIST,
    NODE_IONB,
    NODE_REDIRECTION,
    NODE_PIPE,
    NODE_NEG,
    NODE_WHILE,
    NODE_UNTIL,
    NODE_AND_OR,
    NODE_ASSIGN,
    NODE_FOR,
    NODE_FUNC,
    NODE_SUBSHELL
};

struct node
{
    struct token tok;
    enum node_type type;
    char expanded[16384];
    struct node *first_child;
    struct node *sibling;
};

struct node *create_node(struct token tok, enum node_type type);
void free_node(struct node *node);
// void print_ast(struct node *node);

#endif /* !NODE_H */
