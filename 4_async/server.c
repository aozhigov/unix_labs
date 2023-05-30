#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#define ERROR_CREATE_SOCK "Error create socket\n"
#define ERROR_BIND "Error bind address\n"
#define ERROR_ACCEPT_CLIENT "Error accept client\n"
#define ERROR_CREATE_SERVER "Error create server\n"
#define ERROR_SELECT "Error in select\n"
#define ERROR_ARGS "Error in args\n"
#define ERROR_RECV "Error receive from client\n"
#define ERROR_SEND "Error send to client\n"
#define ERROR_OPEN_CONFIG "Error open config file\n"
#define START_SERVER "Start server\n"
#define START_LISTENING "Server start listening at %s\n"
#define ACCEPR_CLIENT "Accept client %d, client sbrk %ld\n"
#define RECEIVE_FROM_CLIENT "Received from client: %d. Current server state = %d\n"
#define INFO_SERVER_NAME "Server name = %s\n"
#define SERVER_LOG_FILENAME "/tmp/server.log"
#define MAX_CLIENTS 126
#define SOCK_BUFFER_SIZE 64
#define MAX_LINE_LENGTH 1024

char buffer_global[MAX_LINE_LENGTH * 2];
FILE *log_file = NULL;
int STATE = 0;

char* get_socket_name(char config[]) {
    char *sock_name = malloc(MAX_LINE_LENGTH);
    FILE *config_file = fopen(config, "r");
    if (config_file == NULL) {
        perror(ERROR_OPEN_CONFIG);
        exit(1);
    }

    char buffer[MAX_LINE_LENGTH];
    fgets(buffer, MAX_LINE_LENGTH, config_file);
    buffer[strcspn(buffer, "\n\r")] = '\0';
    strcpy(sock_name, buffer);
    return sock_name;
}

void write_log(char *msg) {
    fwrite(msg, 1, strlen(msg), log_file);
    fflush(log_file);
}

int setup(char *server_name) {
    unlink(server_name);

    int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror(ERROR_CREATE_SOCK);
        exit(1);
    }

    int opt_value = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opt_value, sizeof(opt_value));

    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));
    address.sun_family = AF_UNIX;
    sprintf(address.sun_path, "%s", server_name);

    if (bind(sock_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) == -1) {
        perror(ERROR_BIND);
        close(sock_fd);
        exit(1);
    }
    return sock_fd;
}

void accept_client(int server_fd, int clients[]) {
    long sbrk_value = 0;

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror(ERROR_ACCEPT_CLIENT);
        exit(1);
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] == 0) {
            clients[i] = client_fd;
            sbrk_value = (long) sbrk(0);
            sprintf(buffer_global, ACCEPR_CLIENT, client_fd, sbrk_value);
            write_log(buffer_global);
            break;
        }
    }
}

int recv_client(int client_fd) {
        
    char sock_buffer[SOCK_BUFFER_SIZE];
    for (int i = 0; i < SOCK_BUFFER_SIZE; i++) {
        sock_buffer[i] = 0;
    }
    int number = 0;

    if (recv(client_fd, sock_buffer, sizeof(sock_buffer), 0) <= 0) {
        return -1;
    }

    number = atoi(sock_buffer);
    STATE += number;

    sprintf(sock_buffer, "%d", STATE);
        
    if (send(client_fd, sock_buffer, strlen(sock_buffer) + 1, 0) <= 0) {
        perror(ERROR_RECV);
        return -1;
    }

    sprintf(buffer_global, RECEIVE_FROM_CLIENT, number, STATE);
    write_log(buffer_global);
    return 0;
}

void poll(int server_fd, char *server_address) {
    fd_set main_fd_set;
    int max_sock_fd = server_fd;
    int client_fd = -1;
    int select_ret = 0;
    int clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = 0;
    }

    listen(server_fd, MAX_CLIENTS);
    sprintf(buffer_global, START_LISTENING, server_address);
    write_log(buffer_global);

    while (1) {
        FD_ZERO(&main_fd_set);
        FD_SET(server_fd, &main_fd_set);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            client_fd = clients[i];
            if (client_fd > 0) {
                FD_SET(client_fd, &main_fd_set);
            }

            if (client_fd > max_sock_fd) {
                max_sock_fd = client_fd;
            }
        }

        select_ret = select(max_sock_fd + 1, &main_fd_set, NULL, NULL, NULL);
        if (select_ret < 0) {
            perror(ERROR_SELECT);
            exit(1);
        }

        if (FD_ISSET(server_fd, &main_fd_set)) {
            accept_client(server_fd, clients);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (FD_ISSET(clients[i], &main_fd_set)) {
                if (recv_client(clients[i]) < 0) {
                    close(clients[i]);
                    clients[i] = 0;
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        perror(ERROR_ARGS);
        return 1;
    }

    char *socket_name = get_socket_name(argv[1]);
    log_file = fopen(SERVER_LOG_FILENAME, "w");

    sprintf(buffer_global, INFO_SERVER_NAME, socket_name);
    write_log(buffer_global);

    int sock_fd = setup(socket_name);
    if (sock_fd == -1) {
        write_log(ERROR_CREATE_SERVER);
        exit(1);
    }

    write_log(START_SERVER);
    poll(sock_fd, socket_name);

    close(sock_fd);
    fclose(log_file);
    free(socket_name);
    exit(0);
}