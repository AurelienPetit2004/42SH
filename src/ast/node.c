#include "node.h"

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "node.h"

struct node *create_node(struct token tok, enum node_type type)
{
    struct node *new = calloc(sizeof(struct node), 1);
    if (new == NULL)
    {
        errx(1, "Failed to calloc (node)");
        return NULL;
    }
    memset(new->expanded, 0, 16384);
    new->tok = tok;
    new->type = type;
    return new;
}

void free_node(struct node *node)
{
    if (node == NULL)
        return;
    if (node->first_child != NULL)
    {
        free_node(node->first_child);
        node->first_child = NULL;
    }
    if (node->sibling != NULL)
    {
        free_node(node->sibling);
        node->sibling = NULL;
    }
    free(node);
}

/*
void print_ast(struct node *node)
{
    if (node == NULL)
    {
        return;
    }
    struct node *p = NULL;
    switch (node->type)
    {
    case NODE_LIST:
        printf("Start LIST [ \n");
        print_ast(node->first_child);
        p = node->first_child->sibling;
        while (p != NULL)
        {
            printf(" ; ");
            print_ast(p);
            p = p->sibling;
        }
        printf("] \n");
        break;
    case NODE_SIMPLE_COMMAND:
        printf("(SIMPLE COMMAND: %s ", node->tok.data);
        print_ast(node->first_child);
        printf(")\n");
        break;
    case NODE_REDIRECTION:
        printf("(REDIRECTION: %s %s ", node->tok.data,
               node->first_child->tok.data);
        print_ast(node->sibling);
        printf(")");
        break;
    case NODE_ASSIGN:
        printf("VAR %s=%s \n", node->tok.data, node->first_child->tok.data);
        print_ast(node->sibling);
        break;
    case NODE_ELEMENT:
        printf("(ELEMENT: %s ", node->tok.data);
        print_ast(node->sibling);
        printf(")");
        break;
    case NODE_IF:
        p = node->first_child;
        printf("IF ");
        print_ast(p);
        p = p->sibling;
        printf("THEN [");
        print_ast(p);
        printf("]");
        if (p->sibling != NULL)
        {
            if (p->sibling->type == NODE_IF)
            {
                printf(" ELIF ");
            }
            else
            {
                printf("ELSE");
            }
            print_ast(p->sibling);
        }
        break;
    case NODE_COMMAND:
        print_ast(node->first_child);
        break;
    case NODE_PIPE:
        print_ast(node->first_child);
        p = node->first_child->sibling;
        while (p != NULL)
        {
            printf(" | ");
            print_ast(p);
            p = p->sibling;
        }
        break;
    case NODE_NEG:
        printf("! ");
        print_ast(node->first_child);
        break;
    case NODE_WHILE:
        printf("WHILE { ");
        print_ast(node->first_child);
        printf(" }\nDO { ");
        print_ast(node->first_child->sibling);
        printf(" }\n");
        break;
    case NODE_UNTIL:
        printf("UNTIL { ");
        print_ast(node->first_child);
        printf(" }\nDO { ");
        print_ast(node->first_child->sibling);
        printf(" }\n");
        break;
    case NODE_AND_OR:
        print_ast(node->first_child);
        if (node->first_child->sibling != NULL)
        {
            printf("\n%s ", node->tok.data);
            print_ast(node->first_child->sibling);
        }
        break;
    case NODE_FOR:
        p = node->first_child;
        printf("FOR ");
        printf("%s ", p->tok.data);
        p = p->first_child;
        printf(" IN ");
        while (p != NULL)
        {
            printf("%s;", p->tok.data);
            p = p->sibling;
        }
        printf("\nDO ");
        print_ast(node->first_child->sibling);
        break;
    case NODE_SUBSHELL:
        printf("Defining subshell: \n");
        print_ast(node->first_child);
        break;
    case NODE_FUNC:
        printf("Defining function %s:\n", node->tok.data);
        print_ast(node->first_child);
        break;
    default:
        printf("ERROR\n");
        break;
    }
}
*/
