#include "eval_ast.h"

#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "redir.h"

static int eval_while(struct node *ast);
static int eval_if(struct node *ast);
static int eval_until(struct node *ast);
static int eval_and_or(struct node *ast);
static int eval_list(struct node *ast);
static int eval_command(struct node *ast);
static int eval_pipe(struct node *ast);

struct stack *s = NULL;

struct dict *init_func_args(struct node *ast)
{
    char **argv = calloc(sizeof(char *), 100);
    int i = 1;
    struct dict *st = stack_lookup(s, "0");
    char *v = dict_lookup(st, "0");
    argv[0] = v;
    while (ast != NULL)
    {
        argv[i++] = ast->expanded;
        ast = ast->sibling;
    }
    struct dict *ret = init_dict_with_argv("argv", argv);
    return ret;
}

static int exec_defined_functions(struct node *ast)
{
    struct dict *d = stack_lookup_func(s, ast->expanded);
    struct node *cmd = dict_lookup_func(d, ast->expanded);
    struct dict *func_args = init_func_args(ast->first_child);
    s = stack_push(s, func_args);
    int exit_code = eval_command(cmd);
    free(func_args->argv);
    s = stack_pop(s);
    return exit_code;
}

static int exec_cmd(struct node *ast)
{
    if (strcmp(ast->expanded, "") == 0)
        errx(127, "empty string is not a valid command");

    if (stack_lookup_func(s, ast->expanded) != NULL)
    {
        return exec_defined_functions(ast);
    }

    int cmd_nb = 1;
    char **cmd_list = calloc(cmd_nb, sizeof(char *));

    cmd_list[0] = ast->expanded;

    cmd_nb++;
    cmd_list = realloc(cmd_list, cmd_nb * sizeof(char *));
    cmd_list[cmd_nb - 1] = 0;

    ast = ast->first_child;
    // special case for first child
    if (ast != NULL)
    {
        cmd_list[cmd_nb - 1] = ast->expanded;
        cmd_nb++;
        cmd_list = realloc(cmd_list, cmd_nb * sizeof(char *));
        cmd_list[cmd_nb - 1] = 0;
    }

    while (ast != NULL && ast->sibling != NULL)
    {
        ast = ast->sibling;

        cmd_list[cmd_nb - 1] = ast->expanded;

        cmd_nb++;
        cmd_list = realloc(cmd_list, cmd_nb * sizeof(char *));
        cmd_list[cmd_nb - 1] = 0;
    }

    pid_t pid = fork();

    int status;
    if (pid == 127)
    {
        errx(127, "fork failed when executing command");
    }
    else if (pid == 0)
    {
        execvp(cmd_list[0], cmd_list);
        exit(127);
    }
    waitpid(pid, &status, 0);
    int exit_code = WEXITSTATUS(status);
    if (exit_code == 127)
        errx(127, "unknown command \'%s\'", cmd_list[0]);
    free(cmd_list);
    return exit_code;
}

// ast->tok.data = '>'
// ast->sibling->tok.data = "file.txt"
static int apply_redirection(struct node *ast, int **fd_arr, size_t *len)
{
    int exit_code = 0;
    int dup2_exit = 0;

    int io_nb = -1;
    char *output = NULL;
    if (ast->first_child->type == NODE_IONB)
    {
        io_nb = atoi(ast->first_child->tok.data);
        output = ast->first_child->first_child->tok.data;
    }
    else
    {
        output = ast->first_child->tok.data;
    }

    if (strcmp(ast->tok.data, ">") == 0 || strcmp(ast->tok.data, ">|") == 0)
    {
        dup2_exit = redir_1(fd_arr, len, &io_nb, output);
    }
    else if (strcmp(ast->tok.data, "<") == 0)
    {
        dup2_exit = redir_2(fd_arr, len, &io_nb, output);
    }
    else if (strcmp(ast->tok.data, ">>") == 0)
    {
        dup2_exit = redir_3(fd_arr, len, &io_nb, output);
    }
    else if (strcmp(ast->tok.data, "<>") == 0)
    {
        dup2_exit = redir_4(fd_arr, len, &io_nb, output);
    }
    else if (strcmp(ast->tok.data, ">&") == 0)
    {
        dup2_exit = redir_5(fd_arr, len, &io_nb, output);
    }

    else if (strcmp(ast->tok.data, "<&") == 0)
    {
        dup2_exit = redir_6(&io_nb, output);
    }

    else
        errx(2, "Unknown token type \'%s\' in apply_redirection()\n",
             ast->tok.data);
    if (dup2_exit == -1)
        errx(2, "Failed to dup2");
    return exit_code;
}

static int eval_simple_command(struct node *ast)
{
    int exit_code = 0;
    if (strcmp("echo", ast->expanded) == 0)
        exit_code = my_echo(ast);
    else if (strcmp("true", ast->expanded) == 0)
        exit_code = my_true(ast);
    else if (strcmp("false", ast->expanded) == 0)
        exit_code = my_false(ast);
    else if (strcmp("exit", ast->expanded) == 0)
        my_exit(ast);
    else if (strcmp("unset", ast->expanded) == 0)
        exit_code = my_unset(ast->first_child, &s);
    else if (strcmp("cd", ast->expanded) == 0)
        exit_code = my_cd(ast, s);
    else if (strcmp(".", ast->expanded) == 0)
        exit_code = my_dot(ast, s);
    else if (strcmp("export", ast->expanded) == 0)
        exit_code = my_export(ast, s);
    else if (strcmp("continue", ast->expanded) == 0)
        exit_code = my_break_and_continue(ast, 0);
    else if (strcmp("break", ast->expanded) == 0)
        exit_code = my_break_and_continue(ast, 1);
    else
        exit_code = exec_cmd(ast);
    return exit_code;
}

static int is_breaking(int *ex_code)
{
    if (*ex_code < 0)
    {
        *ex_code += 2;
        if (*ex_code <= 0)
            return 1;
        else
        {
            *ex_code = 0;
            return 0;
        }
    }
    return 0;
}

static int eval_while(struct node *ast)
{
    ast = ast->first_child;
    int exit_code = eval_ast(ast, s);
    if (is_breaking(&exit_code))
        return exit_code;
    int exit_sib = 0;
    while (exit_code == 0)
    {
        switch (ast->sibling->type)
        {
        case NODE_UNTIL:
            exit_sib = eval_until(ast->sibling);
            break;
        case NODE_IF:
            exit_sib = eval_if(ast->sibling);
            break;
        case NODE_WHILE:
            exit_sib = eval_while(ast->sibling);
            break;
        default:
            exit_sib = eval_ast(ast->sibling, s);
            break;
        }
        if (is_breaking(&exit_sib))
            return exit_sib;
        exit_code = eval_ast(ast, s);
        if (is_breaking(&exit_code))
            return exit_sib;
    }
    return exit_sib;
}

static int eval_until(struct node *ast)
{
    ast = ast->first_child;
    int exit_code = eval_ast(ast, s);
    if (is_breaking(&exit_code))
        return exit_code;
    int exit_sib = 0;
    while (exit_code == 1)
    {
        switch (ast->sibling->type)
        {
        case NODE_UNTIL:
            exit_sib = eval_until(ast->sibling);
            break;
        case NODE_IF:
            exit_sib = eval_if(ast->sibling);
            break;
        case NODE_WHILE:
            exit_sib = eval_while(ast->sibling);
            break;
        default:
            exit_sib = eval_ast(ast->sibling, s);
            break;
        }
        if (is_breaking(&exit_sib))
            return exit_sib;
        exit_code = eval_ast(ast, s);
        if (is_breaking(&exit_code))
            return exit_sib;
    }
    return exit_sib;
}

static int eval_if(struct node *ast)
{
    ast = ast->first_child;
    int exit_code = 0;
    int cond = eval_ast(ast, s);
    if (cond < 0)
        return cond;
    ast = ast->sibling;
    if (cond == 0)
        exit_code = eval_ast(ast, s);
    else
    {
        if (ast->sibling == NULL)
            return exit_code;
        switch (ast->sibling->type)
        {
        case NODE_UNTIL:
            exit_code = eval_until(ast->sibling);
            break;
        case NODE_IF:
            exit_code = eval_if(ast->sibling);
            break;
        case NODE_WHILE:
            exit_code = eval_while(ast->sibling);
            break;
        default:
            exit_code = eval_ast(ast->sibling, s);
            break;
        }
    }
    return exit_code;
}

static int eval_and_or_aux(struct node *ast, int old_stdout)
{
    int exit_code = 0;
    if (ast->first_child->type == NODE_PIPE)
    {
        exit_code = eval_pipe(ast->first_child);
        if (exit_code < 0)
            return exit_code;
    }
    else if (ast->first_child->type == NODE_NEG)
    {
        exit_code = eval_pipe(ast->first_child->first_child);
        if (exit_code < 0)
            return exit_code;
        exit_code = !exit_code;
    }
    else
    {
        exit_code = eval_and_or(ast->first_child);
        if (exit_code < 0)
            return exit_code;
    }
    dup2(old_stdout, STDOUT_FILENO);

    if (exit_code == 1 && ast->tok.type == TOKEN_OR)
    {
        if (ast->first_child->sibling->type == NODE_NEG)
        {
            exit_code = !eval_pipe(ast->first_child->sibling->first_child);
            if (exit_code < 0)
                return exit_code;
            exit_code = !exit_code;
        }
        else
        {
            exit_code = eval_pipe(ast->first_child->sibling);
            if (exit_code < 0)
                return exit_code;
        }
    }

    if (ast->tok.type == TOKEN_AND && exit_code == 0)
    {
        if (ast->first_child->sibling->type == NODE_NEG)
        {
            exit_code = !eval_pipe(ast->first_child->sibling->first_child);
            if (exit_code < 0)
                return exit_code;
            exit_code = !exit_code;
        }
        else
        {
            exit_code = eval_pipe(ast->first_child->sibling);
            if (exit_code < 0)
                return exit_code;
        }
    }
    dup2(old_stdout, STDOUT_FILENO);
    return exit_code;
}

static int eval_and_or(struct node *ast)
{
    int old_stdout = dup(STDOUT_FILENO);

    if (ast->first_child->sibling == NULL)
    {
        if (!(ast->first_child->type == NODE_NEG))
            return eval_pipe(ast->first_child);
        else
        {
            int ex_c = eval_pipe(ast->first_child->first_child);
            return ex_c < 0 ? ex_c : !ex_c;
        }
    }

    if (ast->tok.type == TOKEN_OR || ast->tok.type == TOKEN_AND)
    {
        return eval_and_or_aux(ast, old_stdout);
    }
    errx(2, "From and_or: Wrong type of ast");
}

static int expand_command(struct node *node)
{
    int num = 0;
    struct node *ast = node;
    if (ast->type == NODE_COMMAND)
        ast = node->first_child;
    switch (ast->type)
    {
    case NODE_ASSIGN:
        expand(ast->first_child, s);
        break;
    case NODE_REDIRECTION:
        if (ast->first_child->first_child != NULL) // for io num
            expand(ast->first_child->first_child, s);
        else
            expand(ast->first_child, s);
        break;
    case NODE_SIMPLE_COMMAND:
        num = expand(ast, s);
        struct node *tmp = ast->first_child;
        while (num > 0 && tmp != NULL)
        {
            tmp = tmp->sibling;
            num = num - 1;
        }
        while (tmp != NULL)
        {
            num = expand(tmp, s);
            while (num > 0 && tmp != NULL)
            {
                tmp = tmp->sibling;
                num = num - 1;
            }
            if (tmp != NULL)
                tmp = tmp->sibling;
        }
        if (strcmp(node->tok.data, "unset") == 0)
            return 0;
        return 1;
    default:
        return 0;
    }
    if (ast->sibling == NULL && ast->type == NODE_ASSIGN)
        return 0;
    return expand_command(ast->sibling);
}

static void assign_var(struct node *ast, int is_prefix)
{
    if (ast == NULL)
        return;
    switch (ast->type)
    {
    case NODE_ASSIGN:
        if (is_prefix)
        {
            struct dict *d = stack_peek(s);
            d = dict_add(d, ast->tok.data, ast->first_child->expanded);
            s->dict = d;
        }
        else
        {
            if (stack_lookup(s, ast->tok.data) == NULL)
            {
                struct stack *stack = stack_get_root(s);
                struct dict *d = stack->dict;
                d = dict_add(d, ast->tok.data, ast->first_child->expanded);
                stack->dict = d;
            }
            else
            {
                struct dict *d = stack_lookup(s, ast->tok.data);
                d = dict_add(d, ast->tok.data, ast->first_child->expanded);
            }
        }
        break;
    case NODE_REDIRECTION:
        break;
    default:
        return;
    }
    assign_var(ast->sibling, is_prefix);
}

static int eval_for(struct node *ast)
{
    int exit_code = 0;
    ast = ast->first_child;
    struct node *tmp = ast->first_child;
    while (tmp != NULL)
    {
        int num = expand(tmp, s);
        while (num >= 0 && tmp != NULL)
        {
            if (tmp->expanded[0] != '\0')
            {
                struct dict *d = stack_peek(s);
                d = dict_add(d, ast->tok.data, tmp->expanded);
                s->dict = d;
                exit_code = eval_ast(ast->sibling, s);
                if (is_breaking(&exit_code))
                    return exit_code;
            }
            num = num - 1;
            tmp = tmp->sibling;
        }
        // if (tmp != NULL)
        // tmp = tmp->sibling;
    }
    return exit_code;
}

static int eval_com_loop(struct node **ast, int *exit_code, int **fd_arr,
                         size_t *len)
{
    while (*ast != NULL && (*ast)->type == NODE_ASSIGN)
        *ast = (*ast)->sibling;

    while (*ast != NULL && (*ast)->type == NODE_REDIRECTION)
    {
        *exit_code = apply_redirection(*ast, fd_arr, len);
        if (*exit_code == -1)
        {
            return -1;
        }
        *ast = (*ast)->sibling;
        while (*ast != NULL && (*ast)->type == NODE_ASSIGN)
            *ast = (*ast)->sibling;
    }
    return 0;
}

// it stands for evaluate function assignment, don't be a baby
static int eval_func_ass(struct node *ast)
{
    struct dict *d = stack_get_root(s)->dict;
    dict_add_func(d, ast->tok.data, ast->first_child);
    ast->first_child =
        NULL; // set fc to NULL so that it is not free in free_ast
    return 0;
}

static int eval_subshell(struct node *ast)
{
    int status = 0;
    int pid = fork();
    if (pid == 127)
        errx(pid, "failed to fork");
    int exit_code = 0;
    if (pid == 0) // child process
    {
        exit_code = eval_list(ast->first_child);
        exit(exit_code);
    }
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status); // extract return value of child
}

static int command_switch(struct node *ast)
{
    if (ast->type == NODE_IF)
        return eval_if(ast);
    else if (ast->type == NODE_SIMPLE_COMMAND)
        return eval_simple_command(ast);
    else if (ast->type == NODE_WHILE)
        return eval_while(ast);
    else if (ast->type == NODE_UNTIL)
        return eval_until(ast);
    else if (ast->type == NODE_FOR)
        return eval_for(ast);
    else if (ast->type == NODE_LIST)
        return eval_list(ast);
    else if (ast->type == NODE_FUNC)
        return eval_func_ass(ast);
    else if (ast->type == NODE_SUBSHELL)
        return eval_subshell(ast);
    else
        errx(2, "Did not implement node %s of type %i in eval_command yet",
             ast->tok.data, ast->type);
}

static int eval_command(struct node *ast)
{
    if (ast == NULL)
        errx(1, "Error when evaluating eval_command NULL: unexpected");
    int exit_code = 0;
    int *fd_arr = NULL;
    size_t len = 0;
    int is_prefix = expand_command(ast);
    ast = ast->first_child;
    struct dict *dict = NULL;
    if (is_prefix)
    {
        s = stack_push(s, dict);
        assign_var(ast, is_prefix);
    }
    else
    {
        assign_var(ast, is_prefix);
    }
    if (eval_com_loop(&ast, &exit_code, &fd_arr, &len) == -1)
        return -1;
    if (ast == NULL)
        return 0;
    exit_code = command_switch(ast);
    if (is_prefix)
        s = stack_pop(s);
    char str[100] = { 0 };
    sprintf(str, "%i", exit_code);
    dict_add(stack_lookup(s, "?"), "?", str);
    if (fd_arr != NULL)
    {
        for (size_t i = 0; i < len; i++)
            close(fd_arr[i]);
        free(fd_arr);
    }
    return exit_code;
}

static int child_fork(struct node *ast, int *p, int pip[2])
{
    if (p != NULL)
    {
        close(p[1]);
        dup2(p[0], STDIN_FILENO);
        close(p[0]);
    }
    close(pip[0]);
    dup2(pip[1], STDOUT_FILENO);
    close(pip[1]);
    int exit = eval_command(ast);
    free(p);
    return exit;
}

static int eval_pipe(struct node *ast)
{
    ast = ast->first_child;
    int *p = NULL;
    int exit_code = 0;
    if (ast->sibling == NULL)
        return eval_command(ast);
    while (ast != NULL)
    {
        if (ast->sibling != NULL)
        {
            int pip[2];
            pipe(pip);
            int i = fork();
            if (i == 0)
                _exit(child_fork(ast, p, pip));
            close(pip[1]);
            waitpid(i, NULL, 0);
            free(p);
            p = calloc(2, sizeof(int));
            p[0] = pip[0];
            p[1] = pip[1];
        }
        else
        {
            close(p[1]);
            dup2(p[0], STDIN_FILENO);
            close(p[0]);
            exit_code = eval_command(ast);
            free(p);
        }
        ast = ast->sibling;
    }
    if (exit_code > 0)
        return 1;
    return exit_code;
}

static int eval_list(struct node *ast)
{
    int exit_code = 0;
    ast = ast->first_child;
    while (ast != NULL && exit_code >= 0)
    {
        int old_stdout = dup(STDOUT_FILENO);
        exit_code = eval_and_or(ast);
        dup2(old_stdout, STDOUT_FILENO);
        ast = ast->sibling;
    }
    return exit_code;
}

int eval_ast(struct node *ast, struct stack *st)
{
    if (ast == NULL || ast->tok.data[0] == 0)
        return 0;
    s = st;
    if (ast->type == NODE_LIST)
        return eval_list(ast);
    else
        errx(2, "Did not implement token \'%s\' of type '%i' yet",
             ast->tok.data, ast->type);
}
