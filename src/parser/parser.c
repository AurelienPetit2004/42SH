#include "parser.h"

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static struct node *parse_list(struct lexer *lex);
static struct node *parse_and_or(struct lexer *lex);
static struct node *parse_pipe(struct lexer *lex);
static struct node *parse_command(struct lexer *lex);
static struct node *parse_simple_command(struct lexer *lex);
static struct node *parse_element(struct lexer *lex);
static struct node *parse_shell_command(struct lexer *lex);
static struct node *parse_rule_if(struct lexer *lex);
static struct node *parse_else_clause(struct lexer *lex);
static struct node *parse_compound_list(struct lexer *lex);
static struct node *parse_redirect(struct lexer *lex);
static struct node *parse_prefix(struct lexer *lex);
static struct node *parse_rule_while(struct lexer *lex);
static struct node *parse_rule_until(struct lexer *lex);
static struct node *parse_rule_for(struct lexer *lex);
static struct node *parse_funcdec(struct lexer *lex);
static struct node *parse_rule_command_block(struct lexer *lex);
static struct node *parse_rule_subshell(struct lexer *lex);

static void get_next_token(struct token *tok, struct lexer *lex)
{
    lexer_pop(lex);
    *tok = lexer_peek(lex);
}

static void remove_newline(struct token *tok, struct lexer *lex)
{
    while (tok->type == TOKEN_NEWLINE)
    {
        lexer_pop(lex);
        *tok = lexer_peek(lex);
    }
}

static int is_separator(enum token_type type)
{
    switch (type)
    {
    case TOKEN_NEWLINE:
        // goto sep;
    case TOKEN_SEMICOLON:
        // goto sep;
    case TOKEN_EOF:
        // goto sep;
    case TOKEN_PIPE:
        // goto sep;
    case TOKEN_AND:
        // goto sep;
    case TOKEN_OR:
    case TOKEN_RIGHT_PAR:
        goto sep;
    default:
        return 0;
    }
sep:
    return 1;
}

static int is_word(enum token_type type)
{
    switch (type)
    {
    case TOKEN_WORD:
    case TOKEN_EXPANSION:
        return 1;
    default:
        return 0;
    }
}

static int is_start_word(enum token_type type)
{
    switch (type)
    {
    case TOKEN_WORD:
        //        goto sep;
    case TOKEN_IF:
        //        goto sep;
    case TOKEN_NOT:
        //        goto sep;
        // case TOKEN_PIPE:
        //    goto sep;
    case TOKEN_REDIRECTION:
        //        goto sep;
    case TOKEN_IONUMBER:
        //        goto sep;
    case TOKEN_UNTIL:
        //        goto sep;
    case TOKEN_WHILE:
    case TOKEN_FOR:
        //        goto sep;
    case TOKEN_ASSIGNMENT:
    case TOKEN_EXPANSION:
    case TOKEN_LEFT_PAR:
    case TOKEN_LEFT_BRACKET:
        goto sep;
    default:
        return 0;
    }
sep:
    return 1;
}

static int shell_command_keyword(enum token_type type)
{
    if (type == TOKEN_IF || type == TOKEN_WHILE || type == TOKEN_UNTIL
        || type == TOKEN_FOR || type == TOKEN_LEFT_BRACKET
        || type == TOKEN_LEFT_PAR)
        return 1;
    return 0;
}

static int is_cpl_delim(enum token_type type)
{
    if (type == TOKEN_THEN || type == TOKEN_FI || type == TOKEN_ELSE
        || type == TOKEN_ELIF || type == TOKEN_DO || type == TOKEN_DONE
        || type == TOKEN_RIGHT_BRACKET || type == TOKEN_RIGHT_PAR)
    {
        return 1;
    }
    return 0;
}

struct node *parse_input(struct lexer *lex)
{
    if (lex == NULL)
    {
        return NULL;
    }
    struct token tok = lexer_peek(lex);
    struct node *root = NULL;
    if (is_start_word(tok.type))
    {
        root = parse_list(lex);
        tok = lexer_peek(lex);
    }
    if (tok.type == TOKEN_NEWLINE)
    {
        lexer_pop(lex);
        return root;
    }
    if (tok.type == TOKEN_EOF)
        return root;
    errx(2, "ERROR GRAMMAR INPUT unknown token : %i %s", tok.type, tok.data);
}

static struct node *parse_list_aux(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    if (tok.type == TOKEN_SEMICOLON)
    {
        lexer_pop(lex);
        tok = lexer_peek(lex);
        if (tok.type == TOKEN_EOF || tok.type == TOKEN_NEWLINE)
        {
            return NULL;
        }
        if (!is_start_word(tok.type))
        {
            errx(2, "ERROR PARSE_LIST_REP");
        }
        else
        {
            return parse_and_or(lex);
        }
    }
    return NULL;
}

static struct node *parse_list(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *list = create_node(tok, NODE_LIST);
    if (is_start_word(tok.type))
    {
        list->first_child = parse_and_or(lex);
        struct node *root = list->first_child;
        root->sibling = parse_list_aux(lex);
        while (root->sibling != NULL)
        {
            root = root->sibling;
            root->sibling = parse_list_aux(lex);
        }
        return list;
    }
    errx(2, "ERROR PARSE_LIST");
}

static struct node *parse_and_or(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *root = create_node(tok, NODE_AND_OR);
    if (is_start_word(tok.type))
    {
        root->first_child = parse_pipe(lex);
        tok = lexer_peek(lex);
    }
    if (tok.type == TOKEN_AND
        || tok.type
            == TOKEN_OR) // If there is an and or or else there ain't (poetry)
    {
        root->tok = tok;
        lexer_pop(lex);
        root->first_child->sibling = parse_pipe(lex);
    }
    tok = lexer_peek(lex);
    while (tok.type == TOKEN_AND || tok.type == TOKEN_OR)
    {
        struct node *new_root = create_node(tok, NODE_AND_OR);
        lexer_pop(lex);
        new_root->first_child = root;
        tok = lexer_peek(lex);
        while (tok.type == TOKEN_NEWLINE)
        {
            lexer_pop(lex);
            tok = lexer_peek(lex);
        }
        new_root->first_child->sibling = parse_pipe(lex);
        tok = lexer_peek(lex);
        root = new_root;
    }
    return root;
}

static struct node *parse_negation(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    if (tok.type == TOKEN_NOT)
    {
        lexer_pop(lex);
        return create_node(tok, NODE_NEG);
    }
    return NULL;
}

static struct node *parse_pipe(struct lexer *lex)
{
    struct node *n_node = parse_negation(lex);
    struct node *p_node = create_node(lexer_peek(lex), NODE_PIPE);
    if (n_node != NULL)
        n_node->first_child = p_node;
    else
        n_node = p_node;
    p_node->first_child = parse_command(lex);
    struct node *last_c = p_node->first_child;
    struct token tok = lexer_peek(lex);
    while (tok.type == TOKEN_PIPE)
    {
        lexer_pop(lex);
        tok = lexer_peek(lex);
        while (tok.type == TOKEN_NEWLINE)
        {
            lexer_pop(lex);
            tok = lexer_peek(lex);
        }
        last_c->sibling = parse_command(lex);
        last_c = last_c->sibling;
        tok = lexer_peek(lex);
    }
    return n_node;
}

static struct node *parse_variable(struct lexer *lex)
{
    struct token tok = lexer_pop(lex);
    struct node *root = create_node(tok, NODE_ASSIGN);
    root->first_child = create_node(lexer_pop(lex), NODE_ELEMENT);
    return root;
}

static struct node *parse_prefix(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    if (tok.type == TOKEN_IONUMBER || tok.type == TOKEN_REDIRECTION)
        return parse_redirect(lex);
    if (tok.type == TOKEN_ASSIGNMENT)
        return parse_variable(lex);
    errx(2, "ERROR PARSE_PREFIX UNEXPECTED TOKEN : %s\n", tok.data);
}

static struct node *parse_redirect(struct lexer *lex)
{
    struct token tok = lexer_pop(lex);
    struct node *root = create_node(tok, NODE_REDIRECTION);
    struct node *last_child = NULL;

    if (tok.type == TOKEN_IONUMBER)
    {
        root->first_child = create_node(tok, NODE_IONB);
        tok = lexer_pop(lex);
        last_child = root->first_child;
    }
    if (tok.type == TOKEN_REDIRECTION)
    {
        root->tok = tok;
        struct token file = lexer_peek(lex);
        lexer_pop(lex);
        if (root->first_child == NULL)
            root->first_child = create_node(file, NODE_ELEMENT);
        else
            last_child->first_child = create_node(file, NODE_ELEMENT);

        return root;
    }
    else
        errx(2, "ERROR PARSE_COMMAND unknown token 69: %i %s", tok.type,
             tok.data);
}

static struct node *parse_command(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *root = create_node(tok, NODE_COMMAND);
    struct node *last_child = NULL;

    // shell command
    if (shell_command_keyword(tok.type))
    {
        struct node *if_node = parse_shell_command(lex);
        tok = lexer_peek(lex);
        if (tok.type == TOKEN_REDIRECTION || tok.type == TOKEN_IONUMBER)
        {
            root->first_child = parse_redirect(lex);
            last_child = root->first_child;
            tok = lexer_peek(lex);
            while (tok.type == TOKEN_REDIRECTION || tok.type == TOKEN_IONUMBER)
            {
                last_child->sibling = parse_redirect(lex);
                last_child = last_child->sibling;
                tok = lexer_peek(lex);
            }
            last_child->sibling = if_node;
        }
        else
            root->first_child = if_node;
    }
    // simple command + //funcdec
    else if (tok.type == TOKEN_IONUMBER || tok.type == TOKEN_REDIRECTION
             || is_word(tok.type) || tok.type == TOKEN_ASSIGNMENT)
    {
        root->first_child = parse_simple_command(lex);
    }
    else
        errx(2, "ERROR PARSE_COMMAND unknown token : %i %s", tok.type,
             tok.data);
    return root;
}

static struct node *parse_funcdec(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    remove_newline(&tok, lex);
    tok = lexer_peek(lex);
    struct node *root = create_node(tok, NODE_COMMAND);
    struct node *last_child = NULL;

    if (shell_command_keyword(tok.type))
    {
        struct node *if_node = parse_shell_command(lex);
        tok = lexer_peek(lex);
        if (tok.type == TOKEN_REDIRECTION || tok.type == TOKEN_IONUMBER)
        {
            root->first_child = parse_redirect(lex);
            last_child = root->first_child;
            tok = lexer_peek(lex);
            while (tok.type == TOKEN_REDIRECTION || tok.type == TOKEN_IONUMBER)
            {
                last_child->sibling = parse_redirect(lex);
                last_child = last_child->sibling;
                tok = lexer_peek(lex);
            }
            last_child->sibling = if_node;
        }
        else
            root->first_child = if_node;
        return root;
    }
    errx(2, "Error in parse_funcdec assigning a function");
}

static void simple_com_aux(struct token *tok, struct node **root,
                           struct node **last_sibling, struct lexer *lex)
{
    while (tok->type == TOKEN_IONUMBER || tok->type == TOKEN_REDIRECTION
           || tok->type == TOKEN_ASSIGNMENT)
    {
        if (*root == NULL)
        {
            *root = parse_prefix(lex);
            *last_sibling = *root;
        }
        else
        {
            (*last_sibling)->sibling = parse_prefix(lex);
            *last_sibling = (*last_sibling)->sibling;
        }
        *tok = lexer_peek(lex);
    }
}

static int is_funcdec(struct node *cmd_node, struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    if (tok.type == TOKEN_LEFT_PAR)
    {
        if (cmd_node == NULL)
        {
            lexer_pop(lex);
            tok = lexer_peek(lex);
            if (tok.type == TOKEN_RIGHT_PAR)
            {
                lexer_pop(lex);
                return 1;
            }
        }
        errx(2, "Error in function definition unexpected token: %s %d\n",
             tok.data, tok.type);
    }
    else
        return 0;
}

static struct node *parse_simple_command(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *root = NULL;
    struct node *last_sibling = NULL;
    simple_com_aux(&tok, &root, &last_sibling, lex);
    if (is_word(tok.type))
    {
        lexer_pop(lex);
        if (is_funcdec(root, lex))
        {
            struct node *fun_node = create_node(tok, NODE_FUNC);
            fun_node->first_child = parse_funcdec(lex);
            return fun_node;
        }
        struct node *smp_cmd = create_node(tok, NODE_SIMPLE_COMMAND);
        struct node *last_child_cmd = NULL;
        while (!is_separator(tok.type))
        {
            tok = lexer_peek(lex);
            struct node *elm = parse_element(lex);
            if (elm == NULL)
                goto ret;
            if (elm->type == NODE_ELEMENT)
            {
                if (smp_cmd->first_child == NULL)
                {
                    smp_cmd->first_child = elm;
                    last_child_cmd = elm;
                }
                else
                {
                    last_child_cmd->sibling = elm;
                    last_child_cmd = last_child_cmd->sibling;
                }
            }
            else
            {
                if (root == NULL)
                {
                    root = elm;
                    last_sibling = root;
                }
                else
                {
                    last_sibling->sibling = elm;
                    last_sibling = last_sibling->sibling;
                }
            }
        }
    ret:
        if (root == NULL)
            return smp_cmd;
        else
            last_sibling->sibling = smp_cmd;
        return root;
    }
    return root;
}

static struct node *parse_shell_command(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    switch (tok.type)
    {
    case TOKEN_LEFT_PAR:
        return parse_rule_subshell(lex);
    case TOKEN_LEFT_BRACKET:
        return parse_rule_command_block(lex);
    case TOKEN_IF:
        return parse_rule_if(lex);
    case TOKEN_WHILE:
        return parse_rule_while(lex);
    case TOKEN_UNTIL:
        return parse_rule_until(lex);
    case TOKEN_FOR:
        return parse_rule_for(lex);
    default:
        errx(2, "ERROR PARSE_SHELL_COMMAND");
    }
}

static struct node *parse_rule_subshell(struct lexer *lex)
{
    struct token tok = lexer_pop(lex);
    struct node *root = create_node(tok, NODE_SUBSHELL);
    tok = lexer_peek(lex);
    if (tok.type != TOKEN_RIGHT_PAR)
    {
        // lexer_pop(lex);
        root->first_child = parse_compound_list(lex);
        tok = lexer_peek(lex);
        if (tok.type == TOKEN_RIGHT_PAR)
        {
            lexer_pop(lex);
            return root;
        }
    }
    errx(2, "Error parse_subshell unexpected token: %s %d\n", tok.data,
         tok.type);
}

static struct node *parse_rule_command_block(struct lexer *lex)
{
    struct token tok = lexer_pop(lex);
    struct node *root = parse_compound_list(lex);
    tok = lexer_pop(lex);
    if (tok.type == TOKEN_RIGHT_BRACKET)
        return root;
    errx(2, "Error missing closing curly bracket");
}

static void get_all_val_for(struct token *tok, struct lexer *lex,
                            struct node **last_child)
{
    if (is_word(tok->type))
    {
        lexer_pop(lex);
        (*last_child)->first_child = create_node(*tok, NODE_ELEMENT);
        *last_child = (*last_child)->first_child;
        *tok = lexer_peek(lex);
    }
    while (is_word(tok->type))
    {
        lexer_pop(lex);
        (*last_child)->sibling = create_node(*tok, NODE_ELEMENT);
        *last_child = (*last_child)->sibling;
        *tok = lexer_peek(lex);
    }
}

static struct node *parse_rule_for(struct lexer *lex)
{
    struct token tok = lexer_pop(lex);
    struct node *root = create_node(tok, NODE_FOR);
    struct node *last_child = NULL;
    tok = lexer_pop(lex);
    if (is_word(tok.type))
    {
        root->first_child = create_node(tok, NODE_ELEMENT);
        last_child = root->first_child;
        tok = lexer_peek(lex);
        if (tok.type == TOKEN_SEMICOLON)
            get_next_token(&tok, lex);
        else
        {
            remove_newline(&tok, lex);
            if (tok.type == TOKEN_IN)
            {
                get_next_token(&tok, lex);
                get_all_val_for(&tok, lex, &last_child);
                if (tok.type == TOKEN_NEWLINE || tok.type == TOKEN_SEMICOLON)
                {
                    get_next_token(&tok, lex);
                }
                else
                    errx(2, "Error parse_rule_for: unexpected token %s, %i\n",
                         tok.data, tok.type);
            }
        }
        remove_newline(&tok, lex);
        if (tok.type == TOKEN_DO)
        {
            get_next_token(&tok, lex);
            remove_newline(&tok, lex);
            if (tok.type != TOKEN_DONE)
            {
                if (root->first_child->first_child == NULL)
                {
                    struct token aux_tok;
                    aux_tok.type = TOKEN_WORD;
                    strcpy(aux_tok.data, "$@");
                    root->first_child->first_child =
                        create_node(aux_tok, NODE_ELEMENT);
                }
                root->first_child->sibling = parse_compound_list(lex);
                tok = lexer_pop(lex);
                if (tok.type == TOKEN_DONE)
                    return root;
            }
        }
    }
    errx(2, "Error general rule_for: unexpected token %s, %i\n", tok.data,
         tok.type);
}

static struct node *parse_rule_while(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *while_node = create_node(tok, NODE_WHILE);
    if (tok.type == TOKEN_WHILE)
    {
        lexer_pop(lex);
        tok = lexer_peek(lex);
        remove_newline(&tok, lex);
        if (tok.type != TOKEN_DO)
        {
            while_node->first_child = parse_compound_list(lex);
            tok = lexer_peek(lex);
            if (tok.type == TOKEN_DO)
            {
                lexer_pop(lex);
                tok = lexer_peek(lex);
                remove_newline(&tok, lex);
                if (tok.type != TOKEN_DONE)
                {
                    while_node->first_child->sibling = parse_compound_list(lex);
                    tok = lexer_peek(lex);
                    if (tok.type == TOKEN_DONE)
                    {
                        lexer_pop(lex);
                        return while_node;
                    }
                }
            }
        }
    }
    errx(2, "ERROR PARSE_RULE_WHILE");
}

static struct node *parse_rule_until(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *until_node = create_node(tok, NODE_UNTIL);
    if (tok.type == TOKEN_UNTIL)
    {
        lexer_pop(lex);
        tok = lexer_peek(lex);
        remove_newline(&tok, lex);
        if (lexer_peek(lex).type != TOKEN_DO)
        {
            until_node->first_child = parse_compound_list(lex);
            tok = lexer_peek(lex);
            if (tok.type == TOKEN_DO)
            {
                lexer_pop(lex);
                tok = lexer_peek(lex);
                remove_newline(&tok, lex);
                if (lexer_peek(lex).type != TOKEN_DONE)
                {
                    until_node->first_child->sibling = parse_compound_list(lex);
                    tok = lexer_peek(lex);
                    if (tok.type == TOKEN_DONE)
                    {
                        lexer_pop(lex);
                        return until_node;
                    }
                }
            }
        }
    }
    errx(2, "ERROR PARSE_RULE_UNTIL");
}

static struct node *parse_rule_if(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *root = NULL;
    if (tok.type == TOKEN_IF)
    {
        root = create_node(tok, NODE_IF);
        lexer_pop(lex);
        tok = lexer_peek(lex);
        if (tok.type == TOKEN_NEWLINE || is_start_word(tok.type)
            || tok.type == TOKEN_IF)
        {
            root->first_child = parse_compound_list(lex);
            struct node *last_child = root->first_child;
            if (lexer_peek(lex).type == TOKEN_THEN)
            {
                lexer_pop(lex);
                last_child->sibling = parse_compound_list(lex);
                last_child = last_child->sibling;
                last_child->sibling = parse_else_clause(lex);
                if (lexer_peek(lex).type == TOKEN_FI)
                {
                    lexer_pop(lex);
                    return root;
                }
            }
        }
    }
    errx(2, "ERROR PARSE_RULE_IF, token \'%s\' of type %i", tok.data, tok.type);
}

static struct node *parse_else_clause(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *root = NULL;
    if (tok.type == TOKEN_ELSE)
    {
        lexer_pop(lex);
        root = parse_compound_list(lex);
        return root;
    }
    if (tok.type == TOKEN_ELIF)
    {
        root = create_node(tok, NODE_IF);
        lexer_pop(lex);
        tok = lexer_peek(lex);
        if (is_word(tok.type) || tok.type == TOKEN_NEWLINE
            || tok.type == TOKEN_IF)
        {
            root->first_child = parse_compound_list(lex);
            struct node *last_child = root->first_child;
            if (lexer_peek(lex).type == TOKEN_THEN)
            {
                lexer_pop(lex);
                last_child->sibling = parse_compound_list(lex);
                last_child = last_child->sibling;
                last_child->sibling = parse_else_clause(lex);
                return root;
            }
        }
    }
    return root;
}

static struct node *parse_compound_list_aux(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    if (tok.type == TOKEN_SEMICOLON || tok.type == TOKEN_NEWLINE)
    {
        lexer_pop(lex);
        tok = lexer_peek(lex);
        while (tok.type == TOKEN_NEWLINE)
        {
            lexer_pop(lex);
            tok = lexer_peek(lex);
        }
        if (is_cpl_delim(tok.type))
        {
            return NULL;
        }
        if (!is_start_word(tok.type))
        {
            errx(2, "ERROR PARSE_COMPOUND_LIST_REP");
        }
        return parse_and_or(lex);
    }
    return NULL;
}

static struct node *parse_compound_list(struct lexer *lex)
{
    struct node *list = create_node(lexer_peek(lex), NODE_LIST);
    struct token tok = lexer_peek(lex);
    while (tok.type == TOKEN_NEWLINE)
    {
        lexer_pop(lex);
        tok = lexer_peek(lex);
    }
    if (!is_start_word(tok.type) && !shell_command_keyword(tok.type))
        errx(2, "Error in compound list: bad token");
    list->first_child = parse_and_or(lex);
    struct node *root = list->first_child;
    root->sibling = parse_compound_list_aux(lex);
    while (root->sibling != NULL)
    {
        root = root->sibling;
        root->sibling = parse_compound_list_aux(lex);
    }
    if (lexer_peek(lex).type == TOKEN_SEMICOLON)
    {
        lexer_pop(lex);
    }
    while (lexer_peek(lex).type == TOKEN_NEWLINE)
    {
        lexer_pop(lex);
    }
    return list;
}

static struct node *parse_element(struct lexer *lex)
{
    struct token tok = lexer_peek(lex);
    struct node *root = NULL;
    if (!is_separator(tok.type))
    {
        if (tok.type == TOKEN_IONUMBER || tok.type == TOKEN_REDIRECTION)
        {
            root = parse_redirect(lex);
            return root;
        }
        else
        {
            root = create_node(tok, NODE_ELEMENT);
            lexer_pop(lex);
            return root;
        }
        // else
        // errx(2, "ERROR PARSE ELEMENT UNEXPECTED TOKEN: %s\n", tok.data);
    }
    return root;
    // if (tok.type == TOKEN_EOF)
    //{
    //}
    // errx(2, "ERROR PARSING ELEMENT");
}
