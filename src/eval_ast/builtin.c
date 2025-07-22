#include "builtin.h"

#include <stdlib.h>

static int is_flag(char *word)
{
    if (word[0] != '-')
        return 0;
    for (int i = 1; word[i] != 0; i++)
    {
        if (word[i] != 'n' && word[i] != 'e' && word[i] != 'E')
        {
            return 0;
        }
    }
    return 1;
}

struct node *get_flags(struct node *ast, int *flag_n, int *flag_e)
{
    if (ast == NULL)
        return ast;
    if (ast->type != NODE_ELEMENT)
        return ast;
    int is_f = is_flag(ast->expanded);
    if (!is_f)
        return ast;
    for (int i = 1; ast->expanded[i] != 0; i++)
    {
        if (ast->expanded[i] == 'e')
            *flag_e = 1;
        else if (ast->expanded[i] == 'E')
            *flag_e = 0;
        else if (ast->expanded[i] == 'n')
            *flag_n = 1;
    }
    return get_flags(ast->sibling, flag_n, flag_e);
}

int my_unset(struct node *ast, struct stack **s)
{
    if (strcmp(ast->expanded, "unset") == 0)
        return my_unset(ast->sibling, s);
    struct dict *d = stack_peek(*s);
    if (stack_lookup(*s, ast->expanded) != NULL)
        d = dict_add(d, ast->expanded, NULL);
    else if (stack_lookup_func(*s, ast->expanded) != NULL)
        d = dict_add_func(d, ast->expanded, NULL);
    else
        d = dict_add(d, ast->expanded, NULL);
    (*s)->dict = d;
    if (ast->sibling != NULL)
        my_unset(ast->sibling, s);
    return 0;
}

static void escaped_echo(char *word)
{
    int is_escaped = 0;
    for (int i = 0; word[i] != 0; i++)
    {
        if (!is_escaped && word[i] == '\\')
            is_escaped = 1;
        else
        {
            if (is_escaped && word[i] == 'n')
                putchar('\n');
            else if (is_escaped && word[i] == 't')
                putchar('\t');
            else if (is_escaped && word[i] == '\\')
                putchar('\\');
            else
                putchar(word[i]);
            is_escaped = 0;
        }
    }
}

void my_exit(struct node *ast)
{
    int exit_code = 0;
    if (ast->first_child != NULL)
    {
        if (ast->first_child->sibling != NULL)
        {
            fprintf(stderr, "wrong way to use exit\n");
            exit(1);
            return;
        }
        exit_code = atoi(ast->first_child->tok.data);
    }
    if (exit_code < 0 || exit_code > 255)
        errx(1, "exit code not valide");
    exit(exit_code);
}

int my_echo(struct node *ast)
{
    int flag_n = 0;
    int flag_e = 0;
    struct node *word =
        get_flags(ast->first_child, &flag_n, &flag_e); // ast->first_child;
    while (word != NULL && word->type == NODE_ELEMENT)
    {
        //        struct token tok = word->tok;
        if (flag_e == 0)
            printf("%s", word->expanded);
        else
            escaped_echo(word->expanded);
        word = word->sibling;
        if (word != NULL && word->expanded[0] != 0 && word->type == NODE_ELEMENT
            && word->expanded[0] != 0)
            printf(" ");
    }
    if (flag_n == 0)
        printf("\n");
    fflush(stdout);
    return 0;
}

static int aux_main(struct stack *s, char *argv[], int argc)
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
    lexer_free(lex);

    return exit_value;
}

static char *path_finding(char *file)
{
    char *p_env = getenv("PATH");
    if (p_env == NULL)
        errx(1, "Path error");
    char *path = strdup(p_env);
    char *d = strtok(path, ":");
    while (d)
    {
        char full_p[4096] = { 0 };
        snprintf(full_p, sizeof(full_p), "%s/%s", d, file);

        if (access(full_p, R_OK) == 0)
        {
            free(path);
            char *res = strdup(full_p);
            return res;
        }

        d = strtok(NULL, ":");
    }
    free(path);
    return NULL;
}

int my_dot(struct node *ast, struct stack *s)
{
    char **arg = calloc(3, sizeof(char *));
    if (ast->first_child == NULL)
        errx(1, "error with dot");
    int to_free = 0;
    arg[0] = ".";
    arg[1] = ast->first_child->tok.data;
    if (access(arg[1], R_OK) != 0)
    {
        errx(127, "error the file does not exist");
    }
    if (strchr(arg[1], '/') == NULL && access(arg[1], R_OK) == -1)
    {
        arg[1] = path_finding(arg[1]);
        if (arg[1] == NULL)
            errx(1, "file not found for dot");
        to_free = 1;
    }
    int exit_code = aux_main(s, arg, 2);
    if (to_free == 1)
        free(arg[1]);
    free(arg);
    return exit_code;
}

int my_cd(struct node *ast, struct stack *s)
{
    struct dict *d = stack_lookup(s, "PWD");
    char *old_pwd = dict_lookup(d, "PWD");
    char new_pwd[4096] = { 0 };
    char *path = NULL;

    if (ast->first_child != NULL)
    {
        if (ast->first_child->sibling != NULL)
            errx(1, "error with cd ast");
        path = ast->first_child->tok.data;
    }

    if (path == NULL || strcmp(path, "~") == 0)
    {
        path = getenv("HOME");
        if (path == NULL)
        {
            fprintf(stderr, "cd: home is not set\n");
            return 1;
        }
    }
    else if (strcmp(path, "-") == 0)
    {
        d = stack_lookup(s, "OLDPWD");
        path = dict_lookup(d, "OLDPWD");
        printf("%s\n", path);
        if (path == NULL)
        {
            fprintf(stderr, "cd: the variable OLDPWD is not set\n");
            return 1;
        }
    }

    if (chdir(path) != 0)
        fprintf(stderr, "cd: %s No such file or directory\n", path);
    if (getcwd(new_pwd, sizeof(new_pwd)) == NULL)
        fprintf(stderr, "getcwd error\n");
    d = stack_lookup(s, "OLDPWD");
    dict_add(d, "OLDPWD", old_pwd);
    d = stack_lookup(s, "PWD");
    dict_add(d, "PWD", new_pwd);
    return 0;
}

int my_export(struct node *ast, struct stack *s)
{
    char *val = NULL;
    char *name = NULL;
    if (ast == NULL)
        return 0;
    if (ast->first_child == NULL)
        errx(1, "error with the export");
    if (ast->first_child->tok.type == TOKEN_ASSIGNMENT)
    {
        name = ast->first_child->tok.data;
        if (ast->first_child->sibling == NULL)
            errx(1, "error with the value of assign");
        val = ast->first_child->sibling->tok.data;
    }
    else
    {
        name = ast->first_child->tok.data;
        struct dict *d = stack_lookup(s, name);
        val = dict_lookup(d, name);
    }
    if (name == NULL || val == NULL)
        return 0;
    if (setenv(name, val, 0) == -1)
        errx(1, "error with the env");
    return 0;
}

int my_true(struct node *ast)
{
    (void)ast;
    return 0;
}

int my_false(struct node *ast)
{
    (void)ast;
    return 1;
}

int my_break_and_continue(struct node *ast, int cmd)
{
    int n = (cmd == 0 ? -1 : -2);
    if (ast->first_child != NULL)
    {
        if (ast->first_child->sibling != NULL)
            errx(2, "Too many argument in break and continue");
        n = n - (2 * atoi(ast->first_child->expanded));
    }
    return n;
}
