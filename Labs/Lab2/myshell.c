#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include "LineParser.h"

void handler(int sig)
{
    printf("\nRecieved Signal : %s\n", strsignal(sig));
    if (sig == SIGTSTP)
    {
        signal(SIGTSTP, SIG_DFL);
    }
    else if (sig == SIGCONT)
    {
        signal(SIGCONT, SIG_DFL);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

void wakeProcess(pid_t pid)
{
    if (kill(pid, SIGCONT) == -1)
    {
        perror("Error waking up the process");
    }
    else
    {
        printf("Process of ID: %d woken up\n", pid);
    }
}

void nukeProcess(pid_t pid)
{
    if (kill(pid, SIGTERM) == -1)
    {
        perror("Error killing the process");
    }
    else
    {
        printf("Process of ID: %d has been terminated\n", pid);
    }
}

void execute(cmdLine *pCmdLine)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("Error while forking!");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {
        if (pCmdLine->inputRedirect != NULL)
        {
            int inp = open(pCmdLine->inputRedirect, O_RDONLY);
            if (inp == -1)
            {
                perror("Error openning file");
                exit(EXIT_FAILURE);
            }
            if (dup2(inp, STDIN_FILENO) == -1)
            {
                perror("Error redirecting standard INPUT");
                exit(EXIT_FAILURE);
            }
            close(inp);
        }
        if (pCmdLine->outputRedirect != NULL)
        {
            int inp = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (inp == -1)
            {
                perror("Error openning file");
                exit(EXIT_FAILURE);
            }
            if (dup2(inp, STDOUT_FILENO) == -1)
            {
                perror("Error redirecting standard OUTPUT");
                exit(EXIT_FAILURE);
            }
            close(inp);
        }

        if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1)
        {
            perror("error in execv");
            exit(1);
        }
    }
    else
    {
        int status;
        if (pCmdLine->blocking)
        {
            if (waitpid(pid, &status, 0) == -1)
            {
                perror("Error while waiting for child process!");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void get_dct()
{
    char directory[PATH_MAX];
    if (getcwd(directory, PATH_MAX) != NULL)
    {
        printf("drc: %s\n", directory);
    }
    else
    {
        perror("Error retrieving current directory");
        exit(1);
    }
}

int main(int argc, char **argv)
{
    printf("Starting the program\n");
    signal(SIGINT, handler);
    signal(SIGTSTP, handler);
    signal(SIGCONT, handler);

    int d = 0;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
        {
            d = 1;
            break;
        }
    }

    while (1)
    {
        get_dct();
        cmdLine *res;
        char input[2048];
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            perror("ERROR recieving input");
            exit(EXIT_FAILURE);
        }

        res = parseCmdLines(input);

        if (strcmp(input, "quit\n") == 0)
        {
            printf("quitting..\n");
            exit(0);
        }

        if (d == 1)
        {
            fprintf(stderr, "PID: %d\n", getpid());
            fprintf(stderr, "Executing command: %s", input);
        }
        if (strcmp(res->arguments[0], "cd") == 0)
        {
            if (chdir(res->arguments[1]) == -1)
            {
                fprintf(stderr, "Error in cd: %s\n", res->arguments[1]);
            }
        }
        else if (strcmp(res->arguments[0], "wakeup") == 0)
        {
            pid_t pid = atoi(res->arguments[1]);
            wakeProcess(pid);
        }
        else if (strcmp(res->arguments[0], "nuke") == 0)
        {
            pid_t pid = atoi(res->arguments[1]);
            nukeProcess(pid);
        }
        else
        {
            execute(res);
        }
        freeCmdLines(res);
    }
    return 0;
}