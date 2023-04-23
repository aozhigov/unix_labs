#include <fcntl.h>
#include <unistd.h>
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>

#define MISSING_ARG_ERROR "Missing arg: file name"
#define NOT_FOUND_LOCK_FILE_FOR_PID "Not found lock file for pid: %d"
#define ERROR_IN_COMPARE_PID "Error in compare pid in lock file: Expected pid: %d, actual pid: %d"
#define ERROR_UNLOCK_FILE "Error unlock lock file"
#define SUCCESSFUL_LOCK "Successful locks = %d\n"

static int LOCKS = 0;

void sigint_handler(int signum)
{ 
    int lock_fd = open("statistics", O_WRONLY | O_APPEND | O_CREAT, S_IRWXU);
    char* buffer = malloc(200);
    snprintf(buffer, 200, SUCCESSFUL_LOCK, LOCKS);
    write(lock_fd, buffer, 200);
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sigint_handler);

    char *in_file_path = NULL;
    if (optind < argc)
    {
        in_file_path = argv[optind++];
    }
    else
    {
        printf(MISSING_ARG_ERROR);
        return -1;
    }

    char *lock_file_path = malloc(strlen(in_file_path) + strlen(".lck") + 1);
    if (lock_file_path == NULL)
    {
        printf("Error while allocating buffer with 'malloc'\n");
        exit(-1);
    }
    strcpy(lock_file_path, in_file_path);
    strcat(lock_file_path, ".lck");

    int pid = getpid();

    while (1)
    {
        int lock_fd = -1;
        while (lock_fd == -1)
        {
            lock_fd = open(lock_file_path, O_CREAT | O_EXCL | O_RDWR, S_IRWXU);
        }

        LOCKS++;

        int pid_len = snprintf(NULL, 0, "%d", pid);
        char* pid_buff = malloc(pid_len + 1);
        snprintf(pid_buff, pid_len + 1, "%d", pid);
        write(lock_fd, pid_buff, pid_len);

        int file_fd = open(in_file_path, O_RDWR, 0);		
        sleep(1);
        close(file_fd);

        int access_result = access(lock_file_path, F_OK);
        if (access_result == -1)
        {
            printf(NOT_FOUND_LOCK_FILE_FOR_PID, pid);
            free(pid_buff);
            exit(-1);
        }

        lseek(lock_fd, 0, SEEK_SET);

        int read_count = read(lock_fd, pid_buff, pid_len + 1);
        int new_pid = atoi(pid_buff);
        free(pid_buff);
        if (new_pid != pid)
        {
            printf(ERROR_IN_COMPARE_PID, pid, new_pid);
            return -1;
        }

        close(lock_fd);
        access_result = unlink(lock_file_path);
        if (access_result == -1)
        {
            perror(ERROR_UNLOCK_FILE);
            return -1;
        }
    }
    return 0;
}