#include "lexer.h"

#include <ctype.h>
#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// fonction utilitaire
//----------------------------------------------------------------------------
struct lexer *lexer_new(const char *input)
{
    if (input[0] == '\0')
        return NULL;
    struct lexer *new = calloc(1, sizeof(struct lexer));
    new->input = input;
    new->pos = 0;
    return new;
}

void lexer_free(struct lexer *lexer)
{
    free(lexer);
}

static enum token_type m_type_bis(char *c)
{
    if (strcmp(c, "if") == 0)
        return TOKEN_IF;
    if (strcmp(c, "then") == 0)
        return TOKEN_THEN;
    if (strcmp(c, "elif") == 0)
        return TOKEN_ELIF;
    if (strcmp(c, "else") == 0)
        return TOKEN_ELSE;
    if (strcmp(c, "fi") == 0)
        return TOKEN_FI;
    if (strcmp(c, ";") == 0)
        return TOKEN_SEMICOLON;
    if (strcmp(c, "\n") == 0)
        return TOKEN_NEWLINE;
    if (strcmp(c, "\0") == 0)
        return TOKEN_EOF;
    if (strcmp(c, "|") == 0)
        return TOKEN_PIPE;
    if (strcmp(c, "||") == 0)
        return TOKEN_OR;
    if (strcmp(c, "!") == 0)
        return TOKEN_NOT;
    return TOKEN_WORD;
}

enum token_type m_type(char *c)
{
    enum token_type res = m_type_bis(c);
    if (res != TOKEN_WORD)
        return res;
    if (strcmp(c, "while") == 0)
        return TOKEN_WHILE;
    if (strcmp(c, "do") == 0)
        return TOKEN_DO;
    if (strcmp(c, "done") == 0)
        return TOKEN_DONE;
    if (strcmp(c, "until") == 0)
        return TOKEN_UNTIL;
    if (strcmp(c, "for") == 0)
        return TOKEN_FOR;
    if (strcmp(c, "in") == 0)
        return TOKEN_IN;
    if (strcmp(c, "(") == 0)
        return TOKEN_LEFT_PAR;
    if (strcmp(c, ")") == 0)
        return TOKEN_RIGHT_PAR;
    if (strcmp(c, "{") == 0)
        return TOKEN_LEFT_BRACKET;
    if (strcmp(c, "}") == 0)
        return TOKEN_RIGHT_BRACKET;
    return TOKEN_WORD;
}

static void reset(char *buff)
{
    for (int i = 0; i < 16384; i++)
        buff[i] = '\0';
}

static void reset_bis(char *buff, char *buf)
{
    for (int i = 0; i < 16384; i++)
        buff[i] = buf[i];
}

static int case_backslash(char c, struct token *res, struct lexer *lex)
{
    size_t i = 0;
    if (lex->input[lex->pos + 1] == '\n')
    {
        lex->pos = lex->pos + 2;
        c = lex->input[lex->pos];
    }
    while (isblank(c) == 0 && c != '\n' && c != '\0')
    {
        res->data[i] = c;
        i++;
        lex->pos++;
        c = lex->input[lex->pos];
    }
    return 1;
}

static int cont(char c, struct lexer *lex)
{
    if (isblank(c) != 0 || c == '\n' || c == ';' || c == '\0' || c == ' '
        || c == '|' || c == '>' || c == '<' || c == '(' || c == ')' || c == '{'
        || c == '}')
        return 1;
    if (c == '&' && lex->input[lex->pos + 1] == '&')
        return 1;
    return 0;
}

//----------------------------------------------------------------------------

static int squot_fill(struct token *res, struct lexer *lex, int *i)
{
    res->data[*i] = lex->input[lex->pos];
    (*i)++;
    lex->pos++;
    char act = lex->input[lex->pos];
    while (act != '\'' && act != '\n' && act != '\0')
    {
        res->data[*i] = act;
        (*i)++;
        lex->pos++;
        act = lex->input[lex->pos];
    }
    if (act == '\0' || act == '\n')
    {
        errx(2, "Error single quotes not closed");
    }
    return 0;
}

static int dquot_fill(struct token *res, struct lexer *lex, int *i)
{
    res->data[*i] = lex->input[lex->pos];
    (*i)++;
    lex->pos++;
    char act = lex->input[lex->pos];
    while (act != '"' && act != '\n' && act != '\0')
    {
        if (act == '\\' && lex->input[lex->pos + 1] == '\n')
        {
            lex->pos = lex->pos + 2;
            act = lex->input[lex->pos];
        }
        else
        {
            res->data[*i] = act;
            (*i)++;
            lex->pos++;
            act = lex->input[lex->pos];
        }
    }
    if (act == '\0' || act == '\n')
    {
        errx(2, "Error double quotes not closed");
    }
    return 0;
}

static int word_assign(char c, struct token *res, struct lexer *lex)
{
    int i = 0;
    int r = 0;
    while (cont(c, lex) == 0)
    {
        if (c == '\\' && lex->input[lex->pos + 1] == '\n')
        {
            lex->pos = lex->pos + 2;
            c = lex->input[lex->pos];
        }
        else
        {
            if (c == '\'')
            {
                r = squot_fill(res, lex, &i);
                if (r == 1)
                    return 1;
            }
            if (c == '"')
            {
                r = dquot_fill(res, lex, &i);
                if (r == 1)
                    return 1;
            }
            res->data[i] = c;
            i++;
            lex->pos++;
            c = lex->input[lex->pos];
        }
    }
    res->type = m_type(res->data);
    return 1;
}

static int check_par(char c, struct token *res)
{
    if (c == '(' || c == ')' || c == '{' || c == '}')
    {
        res->data[0] = c;
        res->type = m_type(res->data);
        return 1;
    }
    return 0;
}

static int check_deli(char c, struct token *res, struct lexer *lex)
{
    if (c == '\\')
        return case_backslash(c, res, lex);
    if (c == ';' || c == '\n' || c == '|' || c == '!')
    {
        res->data[0] = c;
        lex->pos++;
        size_t t = 1;
        if (lex->input[lex->pos] == '!')
        {
            res->data[t++] = '!';
            lex->pos++;
        }
        char c1 = lex->input[lex->pos];
        if (c == '|' && c1 == '|')
        {
            res->data[t] = c1;
            lex->pos++;
        }
        res->type = m_type(res->data);
        return 1;
    }
    if (check_par(c, res) == 1)
    {
        lex->pos++;
        return 1;
    }
    if (c == '#')
    {
        while (c != '\0' && c != '\n')
        {
            lex->pos++;
            c = lex->input[lex->pos];
        }
        struct token r = lexer_next_token(lex);
        res->type = r.type;
        for (size_t i = 0; r.data[i] != '\0'; i++)
            res->data[i] = r.data[i];
        return 1;
    }
    return word_assign(c, res, lex);
}

static int check_redir_io_and(char c, struct token *res, struct lexer *lex)
{
    if (c == '>' || c == '<')
    {
        res->type = TOKEN_REDIRECTION;
        res->data[0] = c;
        lex->pos++;
        char c1 = lex->input[lex->pos];
        if (c1 == '>' || c1 == '&' || (c == '>' && c1 == '|'))
        {
            lex->pos++;
            res->data[1] = c1;
        }
        return 1;
    }
    if (c >= '0' && c <= '9')
    {
        char buf[16384] = { 0 };
        size_t i = 0;
        while (c >= '0' && c <= '9')
        {
            buf[i] = c;
            i++;
            c = lex->input[lex->pos + i];
        }
        if (c == '>' || c == '<')
        {
            reset_bis(res->data, buf);
            res->type = TOKEN_IONUMBER;
            lex->pos = lex->pos + i;
            return 1;
        }
    }
    if (c == '&' && lex->input[lex->pos + 1] == '&')
    {
        res->data[0] = '&';
        res->data[1] = '&';
        lex->pos = lex->pos + 2;
        res->type = TOKEN_AND;
        return 1;
    }
    return 0;
}

static int check_asg(char c, struct token *tok, struct lexer *lex)
{
    if (c != '_' && !(c >= 'A' && c <= 'Z') && !(c >= 'a' && c <= 'z'))
        return 0;
    int save = lex->pos;
    int i = 0;
    char act = c;
    while (act == '_' || (act >= 'A' && act <= 'Z')
           || (act >= 'a' && act <= 'z') || act == '='
           || ('0' <= act && act <= '9'))
    {
        if (act == '=')
        {
            lex->pos++;
            // tok->data[i] = act;
            tok->type = TOKEN_ASSIGNMENT;
            return 1;
        }
        tok->data[i] = act;
        lex->pos++;
        i++;
        act = lex->input[lex->pos];
    }
    lex->pos = save;
    reset(tok->data);
    return 0;
}

struct token lexer_next_token(struct lexer *lexer)
{
    struct token res;
    struct token fail = { .type = TOKEN_EOF, .data = { 0 } };
    reset(res.data);
    if (lexer->input[lexer->pos] == '\0')
    {
        res.type = TOKEN_EOF;
        return res;
    }
    char c = lexer->input[lexer->pos];
    while (isblank(c) != 0)
    {
        lexer->pos++;
        c = lexer->input[lexer->pos];
    }
    int r = check_asg(c, &res, lexer);
    if (r == 1)
        return res;
    r = check_redir_io_and(c, &res, lexer);
    if (r == 1)
        return res;
    r = check_deli(c, &res, lexer);
    if (r == 1)
        return res;
    return fail;
}

struct token lexer_peek(struct lexer *lexer)
{
    size_t i = lexer->pos;
    struct token ret = lexer_next_token(lexer);
    lexer->pos = i;
    return ret;
}

struct token lexer_pop(struct lexer *lexer)
{
    struct token res = lexer_next_token(lexer);
    return res;
}
