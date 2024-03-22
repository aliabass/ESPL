#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include "LineParser.h"

#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0
#define HISTLEN 20

typedef struct process{
    cmdLine* cmd;
    pid_t pid;
    int status;
    struct process *next;
} process;


char *history[HISTLEN];
int curr_spot = 0;
int oldest_spot = 0;
process** process_list = NULL;

void addCmdToHistory(char *cmd){
    char *cmdToAdd = strdup(cmd);
    if(history[curr_spot] != NULL){
        free(history[curr_spot]);
    }
    history[curr_spot] = cmdToAdd;
    curr_spot = (curr_spot + 1)%HISTLEN;
    oldest_spot = (curr_spot == oldest_spot) ? (oldest_spot + 1) % HISTLEN : oldest_spot;
}
//!!
char* last_cmd(){
    if(curr_spot == oldest_spot){
        printf("Cmd is Fresh!");
        return NULL;
    }
    int last = (curr_spot - 1 +HISTLEN) % HISTLEN;
    return history[last];
}
//!n
char* nth_command(int n){
    if (n < 1 || n > HISTLEN || oldest_spot == curr_spot) {
        printf("Invalid command number or a fresh session.\n");
        return NULL;
    }
    int idx = (oldest_spot + n - 2) % HISTLEN;
    return history[idx];
}
void printHistory(){
    int old_spot;
    int hist_number = 1;
    for (old_spot = oldest_spot; old_spot != curr_spot; old_spot = (old_spot + 1) % HISTLEN){
        printf("%d) %s\n", hist_number, history[old_spot]);
        hist_number++;
    }
}
void get_dct(){
    char directory[PATH_MAX];
    if(getcwd(directory,PATH_MAX) != NULL){
    printf("drc: %s\n", directory);
    }else{
        perror("Error retrieving current directory");
        exit(1);
    }
}
cmdLine* copyCmdLine(const cmdLine* pCmdLine)
{
    if (pCmdLine == NULL)
        return NULL;
    cmdLine* copy = (cmdLine*) malloc(sizeof(cmdLine));
    memcpy(copy, pCmdLine, sizeof(cmdLine));
    copy->inputRedirect = pCmdLine->inputRedirect ? strdup(pCmdLine->inputRedirect) : NULL;
    copy->outputRedirect = pCmdLine->outputRedirect ? strdup(pCmdLine->outputRedirect) : NULL;
    copy->next = NULL;
    for (int i = 0; i < pCmdLine->argCount; i++){
        copy->arguments[i] = strdup(pCmdLine->arguments[i]);
    }
    return copy;
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    process* new_process = (process*) malloc(sizeof(process));
    new_process->cmd = copyCmdLine(cmd);
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->next = NULL;
    if (*process_list == NULL) {
        *process_list = new_process;
    } else {
        new_process->next = *process_list;
        *process_list = new_process;
    }
}

void freeProcessList(process** process_list){
    process* curr_process = *process_list;
    while(curr_process != NULL){
        process* next_process = curr_process->next;
        freeCmdLines(curr_process->cmd);
        free(curr_process);
        curr_process = next_process;
    }
    *process_list = NULL;
}
void printProcessList(process** process_list){
    if(process_list == NULL){
        printf("Error: process_list is NULL\n");
        return;
    }
    updateProcessList(process_list);
    process* temp_process_list = *process_list;
    printf("%-12s %-12s %-12s\n", "PID", "Command", "STATUS");
    while(temp_process_list != NULL){
         printf("%-12d %-12s ", temp_process_list->pid, temp_process_list->cmd->arguments[0]);
        switch (temp_process_list->status) {
            case RUNNING:
                printf("%-12s\n", "Running");
                break;
            case SUSPENDED:
                printf("%-12s\n", "Suspend");
                break;
            case TERMINATED:
                printf("%-12s\n", "Terminated");
                deleteProcess(process_list, temp_process_list);
                break;
        }
        temp_process_list = temp_process_list->next;
    }
}

void deleteProcess(process **process_list, process *p) {
    if (*process_list == NULL) return;
    if (*process_list == p) {
        *process_list = (*process_list)->next;
        free(p);
        return;
    }
    process *prev = *process_list;
    while (prev->next != NULL && prev->next != p) {
        prev = prev->next;
    }
    if (prev->next != NULL) {
        prev->next = prev->next->next;
        free(p);
    }
}
void updateProcessList(process **process_list) {
    if (process_list == NULL) {
        return;
    }
     process *temp_process_list = *process_list;
    while (temp_process_list != NULL) {
        int status;
        int res = waitpid(temp_process_list->pid, &status, WNOHANG);
        if (res == -1) {
            temp_process_list = temp_process_list->next;
            continue;
        }
         if (res == temp_process_list->pid) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                updateProcessStatus(*process_list, temp_process_list->pid, TERMINATED);
            } else if (WIFSTOPPED(status)) {
                updateProcessStatus(*process_list, temp_process_list->pid, SUSPENDED);
            } else if (WIFCONTINUED(status)) {
                updateProcessStatus(*process_list, temp_process_list->pid, RUNNING);
            }
        }
         temp_process_list = temp_process_list->next;
    }
}

void updateProcessStatus(process* process_list, int pid, int status) {
    if (process_list == NULL) {
        return;
    }
    process *temp_process_list = process_list;
    while (temp_process_list != NULL) {
        if (temp_process_list->pid == pid) {
            temp_process_list->status = status;
            return;
        }
        temp_process_list = temp_process_list->next;
    }
}

bool isForm_N(const char *str){
    if(str[0] != '!') return false;
    for(int i = 1; str[i] != '\0'; i++){
        if(!isdigit(str[i])) return false;
    }
    return true;
}





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

void wakeProcess(pid_t pid){
    if(kill(pid, SIGCONT) == -1){
        perror("Error waking up the process");
    } else{
        printf("Process of ID: %d woken up\n", pid);
    }
}

void nukeProcess(pid_t pid){
    if(kill(pid, SIGTERM) == -1){
        perror("Error killing the process");
    }else{
        printf("Process of ID: %d has been terminated\n");
    }
}

void suspendProcess(pid_t pid) {
    if (kill(pid, SIGTSTP) == -1) {
         perror("Error suspending process");
    } else {
        printf("Process with PID %d suspended.\n", pid);
    }
}

void execute(cmdLine *pCmdLine)
{
    pid_t pid = fork();
    if(pid == -1){
        perror("Error while forking!");
        exit(EXIT_FAILURE);
    }else if(pid == 0){
        if(pCmdLine->inputRedirect != NULL){
            int inp = open(pCmdLine->inputRedirect, O_RDONLY);
            if(inp == -1){
                perror("Error openning file");
                exit(EXIT_FAILURE);
            }
            if(dup2(inp,STDIN_FILENO) == -1){
                perror("Error redirecting standard INPUT");
                exit(EXIT_FAILURE);
            }
            close(inp);
        }
        if(pCmdLine->outputRedirect != NULL){
            int inp = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if(inp == -1){
                perror("Error openning file");
                exit(EXIT_FAILURE);
            }
            if(dup2(inp,STDOUT_FILENO) == -1){
                perror("Error redirecting standard OUTPUT");
                exit(EXIT_FAILURE);
            }
            close(inp);
        }



    if(execvp(pCmdLine->arguments[0],pCmdLine->arguments)== -1)
    {
        perror("error in execv");
        _exit(EXIT_FAILURE);
    }

    }else{
        int status;
        if(pCmdLine->blocking){
            if(waitpid(pid,&status,0) == -1){
                perror("Error while waiting for child process!");
                exit(EXIT_FAILURE);
            }
        }
        addProcess(&process_list, pCmdLine, pid);
    }
    
}

int main(int argc, char **argv)
{
    printf("Starting the program\n");
	signal(SIGINT, handler);
	signal(SIGTSTP, handler);
	signal(SIGCONT, handler);

    int d = 0;
    for(int i = 1; i<argc; i++)
    {
        if(strcmp(argv[i],"-d") == 0)
        {
            d = 1;
            break;
        }
    }


    while(1)
    {
        get_dct();

        cmdLine *res;
        char input[2048];
        
        if(fgets(input,sizeof(input),stdin) == NULL)
        {
            perror("ERROR recieving input");
            exit(EXIT_FAILURE);
        }

        res = parseCmdLines(input);
        addCmdToHistory(res->arguments[0]);
        if(strcmp(input, "quit\n") == 0)
        {
            printf("quitting..\n");
            freeProcessList(&process_list);
            exit(0);
        }

        if(d == 1)
        {
            fprintf(stderr, "PID: %d\n",getpid());
            fprintf(stderr, "Executing command: %s", input);
        }
        if(strcmp(res->arguments[0],"cd") == 0){
            if(chdir(res->arguments[1]) == -1){
                fprintf(stderr, "Error in cd: %s\n", res->arguments[1]);
            }
        }else if(strcmp(res->arguments[0],"history") == 0){
            printHistory();
        }else if(strcmp(res->arguments[0],"!!") == 0){
            char* cmd = last_cmd();
            res = parseCmdLines(cmd);
            //!! is added to history
        }else if(isForm_N(res->arguments[0])){
            int n = atoi(res->arguments[0] + 1);
            char* cmd = nth_command(n);
            res = parseCmdLines(cmd);
        }else if(strcmp(res->arguments[0],"wakeup")==0){
            pid_t pid = atoi(res->arguments[1]);
            wakeProcess(pid);

        }else if(strcmp(res->arguments[0], "suspend") == 0){
            pid_t pid = atoi(res->arguments[1]);
            suspendProcess(pid);
        }

        else if(strcmp(res->arguments[0],"nuke")==0){
            pid_t pid = atoi(res->arguments[1]);
            nukeProcess(pid);
        }
        else if(strcmp(res->arguments[0], "procs")==0){
            printProcessList(&process_list);
        }
        else if(res->next != NULL){

            //meaning there is pipe
            int pipefd[2];
            pid_t childp1,childp2;
            if(pipe(pipefd) == -1){
                perror("Error in making pipe");
                exit(EXIT_FAILURE);
            }

            childp1 = fork();

            if(childp1 == -1){
                perror("Error in forking child1");
                exit(EXIT_FAILURE);
            }

            if(childp1 == 0){
                close(STDOUT_FILENO);
                if (dup(pipefd[1]) == -1) {
                    perror("dup");
                    exit(EXIT_FAILURE);
                 }
                close(pipefd[1]);
                if(execvp(res->arguments[0],res->arguments)== -1)
                {
                    perror("error in execv");
                    _exit(EXIT_FAILURE);
                }
            }else
            {
                addProcess(&process_list, res, childp1);
                close(pipefd[1]);
                childp2 = fork();

                if(childp2 == -1){
                perror("Error forking child2");
                exit(EXIT_FAILURE);
                }

                if(childp2 == 0){
                close(STDIN_FILENO);

                if (dup(pipefd[0]) == -1) {
                    perror("dup");
                    exit(EXIT_FAILURE);
                }

                close(pipefd[0]);
                if(execvp(res->next->arguments[0],res->next->arguments)== -1)
                {
                    perror("error in execv");
                    _exit(EXIT_FAILURE);
                }
            }else {
                //parent process
                addProcess(&process_list, res->next, childp2);

                close(pipefd[0]);
                waitpid(childp1, NULL, 0);
                waitpid(childp2, NULL, 0);
            }
            }
            
       } else{
            execute(res);
        }
        freeCmdLines(res);
    }
    return 0;
}