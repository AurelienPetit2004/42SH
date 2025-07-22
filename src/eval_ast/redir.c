#include "redir.h"

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int redir_1(int **fd_arr, size_t *len, int *io_nb, char *output)
{
    int fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    *fd_arr = realloc(*fd_arr, sizeof(int) * (*len + 1));
    *(*fd_arr + *len) = fd;
    *len = *len + 1;
    if (*io_nb == -1)
        return dup2(fd, 1);
    else
        return dup2(fd, *io_nb);
}

int redir_2(int **fd_arr, size_t *len, int *io_nb, char *output)
{
    int fd = open(output, O_RDONLY, 0644);
    *fd_arr = realloc(*fd_arr, sizeof(int) * (*len + 1));
    *(*fd_arr + *len) = fd;
    *len = *len + 1;
    if (*io_nb == -1)
        return dup2(fd, 0);
    else
        return dup2(fd, *io_nb);
}

int redir_3(int **fd_arr, size_t *len, int *io_nb, char *output)
{
    int fd = open(output, O_WRONLY | O_CREAT | O_APPEND, 0644);
    *fd_arr = realloc(*fd_arr, sizeof(int) * (*len + 1));
    *(*fd_arr + *len) = fd;
    *len = *len + 1;
    if (*io_nb == -1)
        return dup2(fd, 1);
    else
        return dup2(fd, *io_nb);
}

int redir_4(int **fd_arr, size_t *len, int *io_nb, char *output)
{
    int fd = open(output, O_RDWR | O_CREAT, 0644);
    *fd_arr = realloc(*fd_arr, sizeof(int) * (*len + 1));
    *(*fd_arr + *len) = fd;
    *len = *len + 1;
    if (*io_nb == -1)
        return dup2(fd, 0);
    else
        return dup2(fd, *io_nb);
}

int redir_5(int **fd_arr, size_t *len, int *io_nb, char *output)
{
    if (output[0] != '\0' && atoi(output) == 0)
    {
        int fd = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        *fd_arr = realloc(*fd_arr, sizeof(int) * (*len + 1));
        *(*fd_arr + *len) = fd;
        *len = *len + 1;
        if (*io_nb == -1)
        {
            return dup2(fd, 1);
            return dup2(fd, 2);
        }
        else
            return dup2(fd, *io_nb);
    }
    else
    {
        if (*io_nb == -1)
            return dup2(atoi(output), 1);
        else
            return dup2(atoi(output), *io_nb);
    }
}

int redir_6(int *io_nb, char *output)
{
    if (output[0] == '\0' || atoi(output) != 0)
    {
        if (*io_nb == -1)
            return dup2(atoi(output), 0);
        else
            return dup2(atoi(output), *io_nb);
    }
    else
        errx(2, "Invalid file descriptor for <& redirection\n");
}
