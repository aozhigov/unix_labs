#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define ERROR_ARGS "Error count args (must be 4 or 5)\n" 
#define ERROR_CONNECT_TO_SERVER "Error connect to server\n"
#define ERROR_CREATE_SOCKET "Error create socket\n"
#define ERROR_SET_DELAY "Error set delay\n"
#define ERROR_OPEN_CONFIG "Error open config file\n"
#define INFO_ID_AND_DELAY "Client (id = %d) with delay = %f\n"
#define CLIENT_TIMES "Client (id = %d) execute for %f seconds\n"
#define INFO_SERVER_NAME "Server name = %s\n"
#define SOCK_BUFFER_SIZE 64
#define MAX_LINE_LENGTH 1024

char* get_socket_name(char config[]) {
    char *socket_name = malloc(MAX_LINE_LENGTH);
    FILE *config_file = fopen(config, "r");
    if (config_file == NULL) {
        perror(ERROR_OPEN_CONFIG);
        exit(1);
    }

    char buffer[MAX_LINE_LENGTH];
    fgets(buffer, MAX_LINE_LENGTH, config_file);
    buffer[strcspn(buffer, "\n\r")] = '\0';
    strcpy(socket_name, buffer);
    return socket_name;
}

int get_random_num(int lower, int upper) {
    return (rand() % (upper - lower + 1)) + lower;
}

int main(int argc, char** argv) {
    if (argc < 4 | argc > 5) {
        perror(ERROR_ARGS);
        exit(1);
    }

    int client_id = atoi(argv[2]);
    int nums_count = atoi(argv[3]);
    char* socket_name = get_socket_name(argv[1]);
    printf(INFO_SERVER_NAME, socket_name);
    srand(time(0));
    float delay = 0;
    float sleep_time = get_random_num(1, 255)* 10;
    if (argc == 5) {
        delay = atof(argv[4]);
        sleep_time = delay * 1000000;
    }
    printf(INFO_ID_AND_DELAY, client_id, delay);
    struct sockaddr_un sockaddr;
    memset(&sockaddr, 0, sizeof(struct sockaddr_un));
    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror(ERROR_CREATE_SOCKET);
        exit(-1);
    }

    sockaddr.sun_family = AF_UNIX;
    strcpy(sockaddr.sun_path, socket_name);
    if (connect(sock_fd, (struct sockaddr*) &sockaddr, sizeof(sockaddr)) == -1){
        perror(ERROR_CONNECT_TO_SERVER);
        exit(-1);
    }

    time_t start = time(0);

    for (int i = 0; i < nums_count; i++) {
        char send_buffer[SOCK_BUFFER_SIZE];
        char recv_buffer[SOCK_BUFFER_SIZE];
        for (int i = 0; i < SOCK_BUFFER_SIZE; i++) {
                send_buffer[i] = 0;
        }
        for (int i = 0; i < SOCK_BUFFER_SIZE; i++) {
                recv_buffer[i] = 0;
        }
        scanf("%s", send_buffer);
        usleep(sleep_time);
        write(sock_fd, send_buffer, strlen(send_buffer));
        read(sock_fd, recv_buffer, SOCK_BUFFER_SIZE);
    }

    time_t end = time(0);

    double diff_time = difftime(end, start);
    printf(CLIENT_TIMES, client_id, diff_time);
    close(sock_fd);
    exit(0);
}