#include "expansion.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int is_dbq = 0;

static void expand_quote(char **p, char *buff, size_t *ind)
{
    (*p)++;
    while (**p != '\'')
    {
        buff[*ind] = **p;
        (*p)++;
        *ind += 1;
    }
    (*p)++;
}

static void expand_backslash(char **p, char *buff, size_t *ind)
{
    char *next = *p + 1;
    if (*next == '\\')
    {
        buff[*ind] = '\\';
        goto incr;
    }
    else if (*next == '\"')
    {
        buff[*ind] = '"';
        goto incr;
    }
    else if (*next == '$')
    {
        buff[*ind] = '$';
        goto incr;
    }
    else if (*next == '`')
    {
        buff[*ind] = '`';
        goto incr;
    }
    else if (*next == '\0')
    {
        *p = next;
        return;
    }
    else
    {
        if (is_dbq)
        {
            buff[*ind] = **p;
            *ind += 1;
        }
        buff[*ind] = *next;
    }

incr:
    *p = next + 1;
    *ind += 1;
}

static int is_delimiter(char *c)
{
    switch (*c)
    {
    case '\0':
    case ' ':
    case '%':
    case '\\':
    case ';':
    case '\'':
    case '\"':
    case '$':
    case ':':
        return 1;
    default:
        return 0;
    }
    return 1;
}

static void add_expand(struct node *ast, char *argv, size_t *ind)
{
    for (int i = 0; argv[i] != '\0'; i++)
    {
        ast->expanded[*ind] = argv[i];
        *ind = *ind + 1;
    }
}

static char **get_argv(struct stack *s)
{
    struct stack *tmp = s;
    while (tmp->next != NULL)
    {
        struct dict *d = tmp->dict;
        while (d != NULL)
        {
            if (d->argv != NULL)
            {
                return d->argv;
            }
            d = d->next;
        }
        tmp = tmp->next;
    }
    return tmp->dict->argv;
}

static void itoa(struct node **ast, size_t *ind, int r)
{
    int len = 0;
    int div = 10;
    while ((r / div) > 0)
    {
        len++;
        div = div * 10;
    }
    int l = len;
    while (l >= 0)
    {
        (*ast)->expanded[*ind + l] = (r % 10) + '0';
        r = r / 10;
        l--;
    }
    *ind = *ind + len + 1;
}

static int aux_1(char **p, struct node **ast, size_t *ind, struct stack *s)
{
    char **argv = get_argv(s);
    int size = 0;
    while (argv[size] != NULL)
        size++;
    if (size > 1)
    {
        size = 0;
        add_expand(*ast, argv[1], ind);
        struct node *ast_bis = *ast;
        struct node *next = (*ast)->first_child;
        for (size_t k = 2; argv[k] != NULL; k++)
        {
            struct token tok;
            memset(tok.data, 0, 16384);
            for (size_t i = 0; argv[k][i] != '\0'; i++)
                tok.data[i] = argv[k][i];
            tok.type = TOKEN_WORD;
            struct node *new_node = create_node(tok, NODE_ELEMENT);
            memset(new_node->expanded, 0, 16384);
            for (size_t i = 0; argv[k][i] != '\0'; i++)
                new_node->expanded[i] = argv[k][i];
            new_node->sibling = next;
            if (k == 2)
            {
                ast_bis->first_child = new_node;
                ast_bis = ast_bis->first_child;
            }
            else
            {
                ast_bis->sibling = new_node;
                ast_bis = ast_bis->sibling;
            }
            size++;
            *ind = strlen(argv[k]);
        }
        (*p)++;
        if (**p == '"')
            (*p)++;
        return size;
    }
    (*p)++;
    if (**p == '"')
        (*p)++;
    return 0;
}

static int aux_2(char **p, struct node **ast, size_t *ind, struct stack *s)
{
    char **argv = get_argv(s);
    int size = 0;
    while (argv[size] != NULL)
        size++;
    if (size > 1)
    {
        size = 0;
        add_expand(*ast, argv[1], ind);
        struct node *ast_bis = *ast;
        struct node *next = (*ast)->sibling;
        for (size_t k = 2; argv[k] != NULL; k++)
        {
            struct token tok;
            memset(tok.data, 0, 16384);
            for (size_t i = 0; argv[k][i] != '\0'; i++)
                tok.data[i] = argv[k][i];
            tok.type = TOKEN_WORD;
            struct node *new_node = create_node(tok, NODE_ELEMENT);
            memset(new_node->expanded, 0, 16384);
            for (size_t i = 0; argv[k][i] != '\0'; i++)
                new_node->expanded[i] = argv[k][i];
            new_node->sibling = next;
            ast_bis->sibling = new_node;
            ast_bis = ast_bis->sibling;
            size++;
            *ind = strlen(argv[k]);
        }
        *ast = ast_bis;
        (*p)++;
        if (**p == '"')
            (*p)++;
        return size;
    }
    (*p)++;
    if (**p == '"')
        (*p)++;
    return 0;
}

static int aux_3(char **p, struct node **ast, size_t *ind, struct stack *s)
{
    char **argv = get_argv(s);
    int size = 0;
    while (argv[size] != NULL)
        size++;
    if (size > 1)
    {
        for (size_t k = 1; argv[k] != NULL; k++)
        {
            size_t l = 0;
            while (argv[k][l] != '\0')
            {
                (*ast)->expanded[*ind] = argv[k][l];
                *ind = *ind + 1;
                l++;
            }
            if (argv[k + 1] != NULL)
            {
                (*ast)->expanded[*ind] = ' ';
                *ind = *ind + 1;
            }
        }
        (*p)++;
        if (**p == '"')
            (*p)++;
        return 0;
    }
    (*p)++;
    if (**p == '"')
        (*p)++;
    return 0;
}

static int aux_aux_4(struct node **ast, size_t *ind, char *buf,
                     struct node **ast_bis)
{
    int size = 0;
    int loop = 0;
    int index = 0;
    struct node *next = (*ast)->first_child;
    while (buf[index] != '\0')
    {
        struct token tok;
        memset(tok.data, 0, 16384);
        tok.type = TOKEN_WORD;
        size_t j = 0;
        while (isspace(buf[index]) == 0 && buf[index] != '\0')
        {
            tok.data[j] = buf[index];
            j++;
            index++;
        }
        if (loop == 0)
            add_expand(*ast, tok.data, ind);
        else if (loop == 1)
        {
            struct node *new_node = create_node(tok, NODE_ELEMENT);
            memset(new_node->expanded, 0, 16384);
            for (size_t m = 0; tok.data[m] != '\0'; m++)
                new_node->expanded[m] = tok.data[m];
            new_node->sibling = next;
            (*ast_bis)->first_child = new_node;
            *ast_bis = (*ast_bis)->first_child;
            size++;
            *ind = strlen(tok.data);
        }
        else
        {
            struct node *new_node = create_node(tok, NODE_ELEMENT);
            memset(new_node->expanded, 0, 16384);
            for (size_t m = 0; tok.data[m] != '\0'; m++)
                new_node->expanded[m] = tok.data[m];
            new_node->sibling = next;
            (*ast_bis)->sibling = new_node;
            *ast_bis = (*ast_bis)->sibling;
            size++;
            *ind = strlen(tok.data);
        }
        loop++;
        if (buf[index] != '\0')
            index++;
    }
    return size;
}

static int aux_4(char **p, struct node **ast, size_t *ind, struct stack *s)
{
    char **argv = get_argv(s);
    int size = 0;
    while (argv[size] != NULL)
        size++;
    if (size > 1)
    {
        char buf[16384] = { 0 };
        int index = 0;
        for (size_t k = 1; argv[k] != NULL; k++)
        {
            size_t l = 0;
            while (argv[k][l] != '\0')
            {
                buf[index] = argv[k][l];
                index = index + 1;
                l++;
            }
            if (argv[k + 1] != NULL)
            {
                buf[index] = ' ';
                index = index + 1;
            }
        }
        struct node *ast_bis = *ast;
        size = aux_aux_4(ast, ind, buf, &ast_bis);
        *ast = ast_bis;
        (*p)++;
        return size;
    }
    (*p)++;
    return 0;
}

static void aux_aux_5(char **argv, char *buf)
{
    int index = 0;
    for (size_t k = 1; argv[k] != NULL; k++)
    {
        size_t l = 0;
        while (argv[k][l] != '\0')
        {
            buf[index] = argv[k][l];
            index = index + 1;
            l++;
        }
        if (argv[k + 1] != NULL)
        {
            buf[index] = ' ';
            index = index + 1;
        }
    }
}

static size_t aux_aux_5_bis(char **argv)
{
    size_t size = 0;
    while (argv[size] != NULL)
    {
        size++;
    }
    return size;
}

static int aux_5(char **p, struct node **ast, size_t *ind, struct stack *s)
{
    char **argv = get_argv(s);
    int size = aux_aux_5_bis(argv);
    if (size > 1)
    {
        size = 0;
        char buf[16384] = { 0 };
        aux_aux_5(argv, buf);
        int index = 0;
        int loop = 0;
        struct node *ast_bis = *ast;
        struct node *next = (*ast)->sibling;
        while (buf[index] != '\0')
        {
            struct token tok;
            memset(tok.data, 0, 16384);
            tok.type = TOKEN_WORD;
            size_t j = 0;
            while (isspace(buf[index]) == 0 && buf[index] != '\0')
            {
                tok.data[j] = buf[index];
                j++;
                index++;
            }
            if (loop == 0)
                add_expand(*ast, tok.data, ind);
            else
            {
                struct node *new_node = create_node(tok, NODE_ELEMENT);
                memset(new_node->expanded, 0, 16384);
                for (size_t m = 0; tok.data[m] != '\0'; m++)
                    new_node->expanded[m] = tok.data[m];
                new_node->sibling = next;
                ast_bis->sibling = new_node;
                ast_bis = ast_bis->sibling;
                size++;
                *ind = strlen(tok.data);
            }
            loop++;
            if (buf[index] != '\0')
                index++;
        }
        *ast = ast_bis;
        (*p)++;
        return size;
    }
    (*p)++;
    return 0;
}

static int aux_aux_6(struct node **ast, char *tmp, struct node **ast_bis,
                     size_t *ind)
{
    int index = 0;
    int loop = 0;
    int size = 0;
    struct node *next = (*ast)->first_child;
    while (tmp[index] != '\0')
    {
        struct token tok;
        memset(tok.data, 0, 16384);
        tok.type = TOKEN_WORD;
        size_t j = 0;
        while (isspace(tmp[index]) == 0 && tmp[index] != '\0')
        {
            tok.data[j] = tmp[index];
            j++;
            index++;
        }
        if (loop == 0)
            add_expand(*ast, tok.data, ind);
        else if (loop == 1)
        {
            struct node *new_node = create_node(tok, NODE_ELEMENT);
            memset(new_node->expanded, 0, 16384);
            for (size_t m = 0; tok.data[m] != '\0'; m++)
                new_node->expanded[m] = tok.data[m];
            new_node->sibling = next;
            (*ast_bis)->first_child = new_node;
            *ast_bis = (*ast_bis)->first_child;
            size++;
            *ind = strlen(tok.data);
        }
        else
        {
            struct node *new_node = create_node(tok, NODE_ELEMENT);
            memset(new_node->expanded, 0, 16384);
            for (size_t m = 0; tok.data[m] != '\0'; m++)
                new_node->expanded[m] = tok.data[m];
            new_node->sibling = next;
            (*ast_bis)->sibling = new_node;
            *ast_bis = (*ast_bis)->sibling;
            size++;
            *ind = strlen(tok.data);
        }
        loop++;
        if (tmp[index] != '\0')
            index++;
    }
    return size;
}

static int aux_6(char **p, struct node **ast, size_t *ind, struct stack *s)
{
    struct dict *tmp2 = stack_lookup(s, *p);
    char *tmp = dict_lookup(tmp2, *p);
    struct node *ast_bis = *ast;
    if (tmp != NULL)
    {
        int size = aux_aux_6(ast, tmp, &ast_bis, ind);
        *ast = ast_bis;
        (*p)++;
        return size;
    }
    (*p)++;
    return 0;
}

static int aux_7(char **p, struct node **ast, size_t *ind, struct stack *s)
{
    struct dict *tmp2 = stack_lookup(s, *p);
    char *tmp = dict_lookup(tmp2, *p);
    int size = 0;
    int index = 0;
    int loop = 0;
    struct node *ast_bis = *ast;
    struct node *next = (*ast)->sibling;
    if (tmp != NULL)
    {
        while (tmp[index] != '\0')
        {
            struct token tok;
            memset(tok.data, 0, 16384);
            tok.type = TOKEN_WORD;
            size_t j = 0;
            while (isspace(tmp[index]) == 0 && tmp[index] != '\0')
            {
                tok.data[j] = tmp[index];
                j++;
                index++;
            }
            if (loop == 0)
                add_expand(*ast, tok.data, ind);
            else
            {
                struct node *new_node = create_node(tok, NODE_ELEMENT);
                memset(new_node->expanded, 0, 16384);
                for (size_t m = 0; tok.data[m] != '\0'; m++)
                    new_node->expanded[m] = tok.data[m];
                new_node->sibling = next;
                ast_bis->sibling = new_node;
                ast_bis = ast_bis->sibling;
                size++;
                *ind = strlen(tok.data);
            }
            loop++;
            if (tmp[index] != '\0')
                index++;
        }
        *ast = ast_bis;
        (*p)++;
        return size;
    }
    (*p)++;
    return 0;
}

static int expand_variable(char **p, struct node **ast, size_t *ind,
                           struct stack *s)
{
    int i = 0;
    char *n = *p + 1;
    *p = *p + 1;
    char *tmp = NULL;

    //-----------------------------------------------------------------------------

    if (**p == '@' && is_dbq)
    {
        if ((*ast)->type == NODE_SIMPLE_COMMAND)
        {
            return aux_1(p, ast, ind, s);
        }

        //-----------------------------------------------------------------------

        else
        {
            return aux_2(p, ast, ind, s);
        }
    }

    //-----------------------------------------------------------------------------

    else if (**p == '*' && is_dbq)
    {
        return aux_3(p, ast, ind, s);
    }

    //-----------------------------------------------------------------------------

    else if (**p == '@' || **p == '*')
    {
        if ((*ast)->type == NODE_SIMPLE_COMMAND)
        {
            return aux_4(p, ast, ind, s);
        }

        //-----------------------------------------------------------------------

        else
        {
            return aux_5(p, ast, ind, s);
        }
    }

    //-----------------------------------------------------------------------------

    else if (**p >= '0' && **p <= '9')
    {
        if ((*ast)->type == NODE_SIMPLE_COMMAND)
        {
            return aux_6(p, ast, ind, s);
        }

        //-----------------------------------------------------------------------

        else
        {
            return aux_7(p, ast, ind, s);
        }
    }

    //-----------------------------------------------------------------------------

    char *b = calloc(8126, sizeof(char));

    while (!is_delimiter(n))
    {
        b[i] = *n;
        n++;
        i++;
        (*p)++;
    }

    if (strcmp(b, "RANDOM") == 0)
    {
        free(b);
        srand(time(NULL));
        int r = rand() % 32768;
        itoa(ast, ind, r);
        return 0;
    }

    struct dict *tmp2 = stack_lookup(s, b);
    tmp = dict_lookup(tmp2, b);
    if (tmp != NULL)
    {
        strcpy((*ast)->expanded + *ind, tmp);
        *ind += strlen(tmp);
    }
    free(b);
    return 0;
}

static int exp_doll(struct node *ast, size_t *ind, struct stack *s)
{
    struct dict *d = stack_lookup(s, "$");
    char *val = dict_lookup(d, "$");
    for (size_t i = 0; val[i] != '\0'; i++)
    {
        char c = val[i];
        ast->expanded[*ind] = c;
        (*ind)++;
    }
    return 0;
}

int expand(struct node *node, struct stack *s)
{
    //    char buff[16825] = { 0 };
    memset(node->expanded, 0, 16384);
    size_t ind = 0;
    char *p = node->tok.data;
    int res = 0;
    while (*p != '\0')
    {
        if (*p == '\'')
            expand_quote(&p, node->expanded, &ind);
        else if (*p == '\"')
        {
            if (is_dbq)
                is_dbq = 0;
            else
                is_dbq = 1;
            p++;
        }
        else if (*p == '\\')
            expand_backslash(&p, node->expanded, &ind);
        else if (*p == '$')
        {
            if (*(p + 1) == '$')
            {
                res += exp_doll(node, &ind, s);
                p += 2;
            }
            else if (*(p + 1) == '\0')
            {
                node->expanded[ind++] = '$';
                p++;
            }
            else
                res += expand_variable(&p, &node, &ind, s);
        }
        else
        {
            node->expanded[ind] = *p;
            p++;
            ind++;
        }
    }
    return res;
    //    strcpy(node->tok.data, node->expanded);
}
