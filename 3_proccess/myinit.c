#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <stdarg.h>

#define ERROR_ARGS "Error args count: not config file path"
#define ERROR_ROOT "Error to change root directory \n"
#define ERROR_READ_CONFIG "Error read config file: %s\n"
#define ERROR_START_PROC "Error in start proc with name %s\n"
#define USE_ABSOLUTE_PATH "Use absolute path in %s\n"
#define START "Start myinit\n"
#define RESTART "Restar myinit\n"
#define STARTED_TASK "Task with number %d started %s with pid: %d\n"
#define FINISHED_TASK "Task with number %d finished with status: %d\n"
#define KILLED_TASK "Task with number %d killed\n"
#define PATH_LOG "/tmp/myinit.log"
#define ROOT_DIR "/"
#define SPLIT " "

#define MAX_PROC 100
#define MAX_LINE_LENGTH 1024

struct task_info {
    char **args;
    char* in;
    char* out;
};

int pids_count;
pid_t pids[MAX_PROC];
struct task_info tasks[MAX_PROC];
char buffer[MAX_LINE_LENGTH * 2];
char config_file_name[MAX_LINE_LENGTH];
FILE* log_file;


void write_log(char *msg){
    fwrite(msg, 1, strlen(msg), log_file);
    fflush(log_file);
}

void check_absolute_path(char* path) {
    if (path[0] == '.') {
        sprintf(buffer, USE_ABSOLUTE_PATH, path);
        write_log(buffer);
        exit(1);
    }
}

struct task_info get_task(char* line) {
    char* args = strtok(line, SPLIT);
    check_absolute_path(args);

    struct task_info task;

    task.args = malloc(strlen(args) + 1);
    int i = 0;
    while (args) {
        task.args[i] = malloc(strlen(args));
        strcpy(task.args[i++], args);
        args = strtok(NULL, SPLIT);
    }

    task.in = malloc(strlen(task.args[i - 2]));
    task.out = malloc(strlen(task.args[i - 1]));
    check_absolute_path(task.in);
    check_absolute_path(task.out);

    strcpy(task.in, task.args[i - 2]);
    strcpy(task.out, task.args[i - 1]);

    task.args[i - 1] = NULL;
    task.args[i - 2] = NULL;
    return task;
}

void create_task(int idx) {
    struct task_info task = tasks[idx];
    pid_t cpid = fork();
    switch (cpid) {
    case -1:
        sprintf(buffer, ERROR_START_PROC, task.args[0]);
        write_log(buffer);
        exit(1);
        break;
    case 0:
        freopen(task.in, "r", stdin);
        freopen(task.out, "w", stdout);
        execv(task.args[0], task.args);
    default:
        pids[idx] = cpid;
        pids_count++;
        sprintf(buffer, STARTED_TASK, idx, task.args[0], cpid);
        write_log(buffer);
        break;
    }
}

void exec_init() {
    FILE* config_file = fopen(config_file_name, "r");
    if (!config_file) {
        sprintf(buffer, ERROR_READ_CONFIG, config_file_name);
        write_log(buffer);
        exit(1);
    }
    char* line = NULL;
    size_t len = 0;
    int count_tasks_in_config = 0;
    while ((getline(&line, &len, config_file) != -1)) {
        tasks[count_tasks_in_config] = get_task(line);
        count_tasks_in_config++;
    }

    pid_t cpid;

    for (int i = 0; i < count_tasks_in_config; i++) {
        create_task(i);
    }

    while (pids_count) {
        int status = 0;
        cpid = waitpid(-1, &status, 0);
        for (int i = 0; i < count_tasks_in_config; i++) {
            if (pids[i] == cpid) {
                sprintf(buffer, FINISHED_TASK, i, status);
                write_log(buffer);
                pids[pids_count--] = 0;
                create_task(i);
            }
        }
    }
}

void sighup_handler(int sig) {
    for (int p = 0; p < MAX_PROC; p++) {
        if (pids[p]) {
            kill(pids[p], SIGKILL);
            sprintf(buffer, KILLED_TASK, p);
            write_log(buffer);
        }
    }
    write_log(RESTART);
    exec_init();
    exit(0);
}

void close_all_fds() {
    struct rlimit fd_limit;
    getrlimit(RLIMIT_NOFILE, &fd_limit);
    for (int fd = 0; fd < fd_limit.rlim_max; fd++)
        close(fd);
}

void prepare_environment(char *argv[]){
    char *new_dir = ROOT_DIR;
    if (chdir(new_dir) != 0) {
        printf(ERROR_ROOT);
        exit(1);
    }

    close_all_fds();
    if (getppid() != 1) {
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        if (fork() != 0) {
            exit(0);
        }
        setsid();
    }
    log_file = fopen(PATH_LOG, "w");
    write_log(START);
    strcpy(config_file_name, argv[1]);
    check_absolute_path(config_file_name);
    signal(SIGHUP, (void (*)(int)) sighup_handler);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf(ERROR_ARGS); 
        exit(1);
    }
    prepare_environment(argv);
    exec_init();
    exit(0);
}