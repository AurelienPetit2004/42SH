#ifndef TOKEN_H
#define TOKEN_H

#include <unistd.h>

enum token_type
{
    TOKEN_IF,
    TOKEN_THEN,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_FI,
    TOKEN_SEMICOLON,
    TOKEN_NEWLINE,
    TOKEN_WORD,
    TOKEN_APO_ERROR,
    TOKEN_IONUMBER,
    TOKEN_REDIRECTION,
    TOKEN_PIPE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_DONE,
    TOKEN_UNTIL,
    TOKEN_FOR,
    TOKEN_IN,
    TOKEN_EXPANSION,
    TOKEN_ASSIGNMENT,
    TOKEN_LEFT_PAR,
    TOKEN_RIGHT_PAR,
    TOKEN_LEFT_BRACKET,
    TOKEN_RIGHT_BRACKET,
    TOKEN_EOF
};

struct token
{
    enum token_type type; // The kind of token
    char data[16384];
};
#endif /* !TOKEN_H */
