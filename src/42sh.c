#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ast/node.h"
#include "eval_ast/eval_ast.h"
#include "io/io.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "utils/dict.h"
#include "utils/expansion.h"
#include "utils/stack.h"

int aux_main(struct stack *s, char *argv[], int argc)
{
    int exit_value = 0;
    struct lexer *lex = NULL;
    char buff[1048576] = { 0 };

    switch (argc)
    {
    case 1:
        read_input(buff);
        lex = lexer_new(buff);
        break;
    default:
        if (strcmp(argv[1], "-c") == 0)
        {
            if (argv[2] == NULL)
                errx(2, "Invalid input");
            lex = lexer_new(argv[2]);
            break;
        }
        else
        {
            for (size_t i = 0; argv[i] != NULL; i++)
                argv[i] = argv[i + 1];
            read_file(argv[0], buff);
            lex = lexer_new(buff);
            break;
        }
        break;
    }

    if (lex == NULL)
    {
        exit_value = 0;
        goto end;
    }

    while (lexer_peek(lex).type != TOKEN_EOF)
    {
        struct node *ast = parse_input(lex);

        exit_value = eval_ast(ast, s);
        free_node(ast);
    }

end:
    free_stack(s);
    lexer_free(lex);

    return (exit_value < 0 ? 0 : exit_value);
}

int main(int argc, char *argv[])
{
    // init env variables
    struct dict *d = init_dict_with_argv("argv", argv);

    d = dict_add(d, "?", "0");
    d = dict_add(d, "IFS", " \t\n");

    char pwd[10000] = { 0 };
    // d = dict_add(d, "PWD", get_current_dir_name());
    d = dict_add(d, "PWD", getcwd(pwd, 10000));

    char oldpwd[10000] = { 0 };
    getcwd(oldpwd, 10000);
    d = dict_add(d, "OLDPWD", oldpwd);

    char dollar[10000] = { 0 };
    sprintf(dollar, "%i", (int)getpid());
    d = dict_add(d, "$", dollar);

    char hash[10000] = { 0 };
    if (argc > 1)
        sprintf(hash, "%i", argc - 1);
    else
        sprintf(hash, "%i", 0);
    d = dict_add(d, "#", hash);

    unsigned long int uid = (unsigned long int)getuid();
    char arr_uid[10000] = { 0 };
    sprintf(arr_uid, "%lu", uid);
    d = dict_add(d, "UID", arr_uid);

    struct stack *s = NULL;
    s = stack_push(s, d);

    return aux_main(s, argv, argc);
}
