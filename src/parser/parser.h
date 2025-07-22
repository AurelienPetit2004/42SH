#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdio.h>

#include "../ast/node.h"
#include "../lexer/lexer.h"
#include "../lexer/token.h"

struct node *parse_input(struct lexer *lex);

#endif /* !PARSER_H */
