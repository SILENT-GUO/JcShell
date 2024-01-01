// student ID: 3035770915
// Name: Guo Zebin
// Course: COMP3230
// Description: This is a shell program that can handle multiple commands with pipes.

// The following is the code for JCshell_3035770915.c
#include <stdlib.h>
#include <stdio.h>
#include <signal.h> /* To handle various signals */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

#define MAX_NUM_STRING 30
#define MAX_STRING_LENGTH 1024

#define MAX_SIZE 100
#define MAX_PARTS 5
#define MAX_SUBPARTS 205
#define MAX_SUBPART_LENGTH 205

char buffer[5][1024];
int buffer_index = 0;

void sortArrayWithIndices(double arr[], int n, int indices[])
{
    // Initialize indices array
    for (int i = 0; i < n; ++i)
    {
        indices[i] = i;
    }

    // Bubble sort algorithm for sorting with indices
    for (int i = 0; i < n - 1; ++i)
    {
        for (int j = 0; j < n - i - 1; ++j)
        {
            if (arr[j] > arr[j + 1])
            {
                // Swap corresponding indices only
                int temp = indices[j];
                indices[j] = indices[j + 1];
                indices[j + 1] = temp;
            }
        }
    }
}
// split the string by delimiter
int splitString(char *line, char *delimiter, char *parts[])
{
    int count = 0;
    char *token = strtok(line, delimiter);
    // first scan through whether two | symbols are adjacent

    while (token != NULL && count < MAX_PARTS)
    {
        parts[count++] = token;
        token = strtok(NULL, delimiter);
    }

    return count;
}

int siguser1SignalReceived = 0;
int sigintSignalReceived = 0;

// handle SIGINT signal in child process
void child_sigint_handler(int sig)
{
}

// handle SIGINT signal in parent process
void parent_sigint_handler(int sig)
{
    printf("\n## JCshell [%d] ## ", getpid());
    fflush(stdout);
    sigintSignalReceived = 1;
}

// handle SIGUSR1 signal in parent process
void siguser1_handler(int sig)
{
    siguser1SignalReceived = 1;
}

void bind_main_process_handler()
{
    signal(SIGINT, SIG_DFL);
    signal(SIGUSR1, SIG_DFL);

    struct sigaction act = {0};
    act.sa_flags = SA_RESTART;
    act.sa_handler = &parent_sigint_handler;
    sigaction(SIGINT, &act, NULL);

    struct sigaction act3 = {0};
    act3.sa_flags = SA_RESTART;
    act3.sa_handler = &siguser1_handler;
    sigaction(SIGUSR1, &act3, NULL);
}

void bind_child_process_handler()
{
    signal(SIGINT, SIG_DFL);

    struct sigaction act = {0};
    act.sa_flags = SA_RESTART;
    act.sa_handler = &child_sigint_handler;
    sigaction(SIGINT, &act, NULL);
}

int main()
{
    // use mask to block SIGINT signal to distinguish sigint handler in parent process and child process
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    bind_main_process_handler();

    while (1)
    {

        char input_string[MAX_STRING_LENGTH];
        char *input_string_array[MAX_NUM_STRING];
        int child_status;

        printf("## JCshell [%d] ## ", getpid());

        if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
        {
            perror("Error unblocking signal in child process");
            exit(1);
        }
        fgets(input_string, MAX_STRING_LENGTH, stdin);
        if (input_string[strlen(input_string) - 1] == '\n')
        {
            input_string[strlen(input_string) - 1] = '\0';
        }
        // check if input_string has adjacent | symbols
        int length = strlen(input_string);
        int legal = 1;
        for (int i = 0; i < length - 1; ++i)
        {
            if (input_string[i] == '|' && input_string[i + 1] == '|')
            {
                printf("JCshell: should not have two | symbols without in-between command\n");
                legal = 0;
                break;
            }
        }

        char *parts[MAX_PARTS];
        char subParts[MAX_PARTS][MAX_SUBPARTS][MAX_SUBPART_LENGTH];

        int numParts;
        int numSubParts[MAX_PARTS];
        numParts = splitString(input_string, "|", parts);
        double runtime_list[numParts];
        int wstat;

        for (int i = 0; i < numParts; i++)
        {
            int count = 0;
            char *token = strtok(parts[i], " ");

            while (token != NULL && count < MAX_SUBPARTS)
            {
                strcpy(subParts[i][count++], token);
                token = strtok(NULL, " ");
            }
            if (count == 0)
            {
                printf("JCshell: should not have two | symbols without in-between command\n");
                legal = 0;
                break;
            }
            numSubParts[i] = count;
        }
        if (legal == 0)
        {
            continue;
        }

        // split input_string by \n and space
        char *token = strtok(input_string, " \n");

        // if only one string, check if it is exit
        if (strcmp(subParts[0][0], "exit") == 0)
        {
            if (numParts > 1 | numSubParts[0] > 1)
            {
                printf("JCshell: 'exit' with other arguments!!!\n");
                continue;
            }
            printf("JCshell: Terminated\n");
            exit(0);
        }

        if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
        {
            perror("Error blocking signal");
            return 1;
        }
        // fork a child process to handle the input string
        int pipeNum = numParts - 1;
        int pipes[pipeNum][2];
        int pid[numParts];
        // create pipes
        for (int _ = 0; _ < pipeNum; _++)
        {
            if (pipe(pipes[_]) == -1)
            {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }
        // create child processes
        for (int i = 0; i < numParts; i++)
        {
            pid[i] = fork();
            // fail case
            if (pid[i] < 0)
            {
                perror("fork");
                exit(EXIT_FAILURE);
            }
            if (pid[i] == 0)
            { // child process
                if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
                {
                    perror("Error unblocking signal in child process");
                    exit(1);
                }
                bind_child_process_handler();

                if (i != 0)
                {
                    // Redirect input from previous pipe
                    if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1)
                    {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }

                if (i != numParts - 1)
                {
                    // Redirect output to next pipe
                    if (dup2(pipes[i][1], STDOUT_FILENO) == -1)
                    {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }

                // Close all pipes in child process
                for (int j = 0; j < numParts - 1; j++)
                {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                // solve one command first, later solve pipes
                int length_for_ith_command = numSubParts[i];
                char *args[length_for_ith_command + 1];
                for (int j = 0; j < length_for_ith_command; ++j)
                {
                    args[j] = subParts[i][j];
                }
                args[length_for_ith_command] = (char *)0;

                // while not receive siguser1 signal, not execute the command
                while (siguser1SignalReceived == 0)
                {
                }
                // execute the command
                execvp(args[0], args);

                // fail case
                printf("JCshell: '%s': No such file or directory\n", args[0]);
                exit(0);
            }
            else
            { // parent process
                bind_main_process_handler();

                if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1)
                { // block SIGINT signal
                    perror("Error blocking signal");
                    return 1;
                }
                // send SIGUSR1 signal to child process to arrange the order of execution
                kill(pid[i], SIGUSR1);
            }
        }
        for (int i = 0; i < pipeNum; i++)
        {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
        for (int i = 0; i < numParts; i++)
        {
            siginfo_t info;
            int ret;
            ret = waitid(P_PID, pid[i], &info, WNOWAIT | WEXITED);
            // read statistics from /proc
            char stat_file_name[100], status_file_name[100];
            sprintf(stat_file_name, "/proc/%d/stat", pid[i]);
            sprintf(status_file_name, "/proc/%d/status", pid[i]);
            FILE *stat_file = fopen(stat_file_name, "r");
            FILE *status_file = fopen(status_file_name, "r");
            FILE *status_file_dummy = fopen(status_file_name, "r");

            if (stat_file == NULL)
            {
                printf("Error: cannot open stat_file %s\n", stat_file_name);
                exit(0);
            }
            if (status_file == NULL)
            {
                printf("Error: cannot open status_file %s\n", status_file_name);
                exit(0);
            }
            if (status_file_dummy == NULL)
            {
                printf("Error: cannot open status_file_dummy %s\n", status_file_name);
                exit(0);
            }

            int param, ppid, foo_int;
            unsigned int foo_uint;
            unsigned long foo_ulong, utime, stime;
            long int foo_long_int;
            long long int start_time;
            long clock_tick_rate = sysconf(_SC_CLK_TCK);
            char comm[50], state;
            fscanf(stat_file, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %lld ",
                   &param, comm, &state, &ppid, &foo_int, &foo_int, &foo_int, &foo_int, &foo_uint,
                   &foo_ulong, &foo_ulong, &foo_ulong, &foo_ulong, &utime, &stime, &foo_long_int, &foo_long_int,
                   &foo_long_int, &foo_long_int, &foo_long_int, &foo_long_int, &start_time);
            fclose(stat_file);
            double time_in_user_mode = (double)utime / clock_tick_rate;
            double time_in_kernel_mode = (double)stime / clock_tick_rate;
            // remove () of comm from the beginning and end
            int comm_length = strlen(comm);
            for (int i = 0; i < comm_length - 2; ++i)
            {
                comm[i] = comm[i + 1];
            }
            comm[comm_length - 2] = '\0';

            int vctx = 0, nvctx = 0;

            int num_of_lines = 0;
            int j = 0;
            char input[300];
            rewind(status_file_dummy);
            while (fscanf(status_file_dummy, "%s", input) != EOF)
            {
                num_of_lines++;
            }
            fclose(status_file_dummy);
            rewind(status_file);
            while (fscanf(status_file, "%s", input) != EOF)
            {
                j++;
                if (j == num_of_lines - 2)
                {
                    // if input can't be converted to int, raise error
                    if (sscanf(input, "%d", &vctx) != 1)
                    {
                        printf("Error: cannot convert vctx to int\n");
                    }
                }
                else if (j == num_of_lines)
                {
                    if (sscanf(input, "%d", &nvctx) != 1)
                    {
                        printf("Error: cannot convert nvctx to int\n");
                    }
                }
            }
            fclose(status_file);
            runtime_list[i] = (double)start_time / clock_tick_rate + utime + stime;
            // print the status of the child process
            if (info.si_code == CLD_EXITED)
            {
                snprintf(buffer[buffer_index++], MAX_SUBPARTS, "(PID)%d (CMD)%s (STATE)%c (EXCODE)%d (PPID)%d (USER)%.2f (SYS)%.2f (VCTX)%d (NVCTX)%d\n", param, comm, state, info.si_status, ppid, time_in_user_mode, time_in_kernel_mode, vctx, nvctx);
            }
            else if (info.si_code == CLD_KILLED | info.si_code == CLD_DUMPED | info.si_code == CLD_TRAPPED | info.si_code == CLD_STOPPED)
            {
                snprintf(buffer[buffer_index++], MAX_SUBPARTS, "(PID)%d (CMD)%s (STATE)%c (EXSIG)%s (PPID)%d (USER)%.2f (SYS)%.2f (VCTX)%d (NVCTX)%d\n", param, comm, state, strsignal(info.si_status), ppid, time_in_user_mode, time_in_kernel_mode, vctx, nvctx);
            }
            else
            {
                printf("Child process with PID %d terminated abnormally.\n", pid[i]);
            }
            waitpid(pid[i], &wstat, 0);
        }

        int indice_ordered_list[numParts];
        sortArrayWithIndices(runtime_list, numParts, indice_ordered_list);
        for (int i = 0; i < buffer_index; ++i)
        {
            printf("%s", buffer[indice_ordered_list[i]]);
        }
        buffer_index = 0;
        for (int i = 0; i < 5; ++i)
        {
            memset(buffer[i], 0, sizeof(buffer[i]));
        }
    }
    return 0;
}