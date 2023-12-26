#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>

#define ERROR     -1
#define BUFFER    512
#define MAX_CLIENTS    4

void error(const char *msg);

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in remote_server;
    char input[BUFFER];
    char output[BUFFER];
    int len;
    char *keyword_to_be_send;
    int choice;
    int LISTENING_PORT;
    LISTENING_PORT = (atoi(argv[3]));

    char file_fetch[BUFFER];
    char file_save[BUFFER];
    char peer_ip[BUFFER];
    char peer_port[BUFFER];
    struct sockaddr_in peer_connect;
    int peer_sock;

    char message[BUFFER];

    int listen_sock;
    struct sockaddr_in server;
    struct sockaddr_in client;
    int sockaddr_len = sizeof(struct sockaddr_in);
    int child;

    fd_set master;
    fd_set read_fd;

    if (argc < 3) {
        fprintf(stderr, "ERROR, ENTER SERVER IP ADDRESS AND PORT WITH YOUR LISTENING PORT\n");
        exit(-1);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        perror("socket");
        exit(-1);
    }

    remote_server.sin_family = AF_INET;
    remote_server.sin_port = htons(atoi(argv[2]));
    remote_server.sin_addr.s_addr = inet_addr(argv[1]);
    memset(&remote_server.sin_zero, 0, sizeof(remote_server.sin_zero));

    if (connect(sock, (struct sockaddr*)&remote_server, sizeof(struct sockaddr_in)) == ERROR) {
        perror("connect");
        exit(-1);
    }

    if ((listen_sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
        perror("socket");
        exit(-1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(LISTENING_PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(&server.sin_zero, 0, sizeof(server.sin_zero));

    if (bind(listen_sock, (struct sockaddr*)&server, sockaddr_len) == ERROR) {
        perror("bind");
        exit(-1);
    }

    if (listen(listen_sock, MAX_CLIENTS) == ERROR) {
        perror("listen");
        exit(-1);
    }

    FD_ZERO(&master);
    FD_SET(listen_sock, &master);
    int i;

    child = fork();

    if (!child) {
        while (1) {
            read_fd = master;
            if (select(FD_SETSIZE, &read_fd, NULL, NULL, NULL) == -1) {
                perror("select");
                return -1;
            }

            for (i = 0; i < FD_SETSIZE; ++i) {
                if (FD_ISSET(i, &read_fd)) {
                    if (i == listen_sock) {
                        int new_peer_sock;
                        if ((new_peer_sock = accept(listen_sock, (struct sockaddr*)&client, &sockaddr_len)) == ERROR) {
                            perror("ACCEPT.Error accepting new connection");
                            exit(-1);
                        } else {
                            FD_SET(new_peer_sock, &master);
                        }
                    } else {
                        bzero(input, BUFFER);
                        if ((len = recv(i, input, BUFFER, 0)) <= 0) {
                            if (len == 0) {
                                printf("Peer %d with IP address %s disconnected\n", i, inet_ntoa(client.sin_addr));
                            } else {
                                perror("ERROR IN RECEIVE");
                            }
                            close(i);
                            FD_CLR(i, &master);
                        } else {
                            printf("%s\n", input);

                            char* requested_file = input;
                            FILE* file_request = fopen(requested_file, "r");

                            if (file_request == NULL) {
                                close(i);
                                FD_CLR(i, &master);
                            } else {
                                bzero(output, BUFFER);
                                int file_request_send;
                                while ((file_request_send = fread(output, sizeof(char), BUFFER, file_request)) > 0) {
                                    if (send(i, output, file_request_send, 0) < 0) {
                                        fprintf(stderr, "ERROR: Not able to send file");
                                    }

                                    bzero(output, BUFFER);
                                }
                                fclose(file_request);
                                close(i);
                                FD_CLR(i, &master);
                            }
                        }
                    }
                }
            }
        }
        close(listen_sock);
        exit(0);
    }

    while (1) {
        printf("\nWELCOME. ENTER YOUR CHOICE\n");
        printf("1.SEARCH FILE\n");
        printf("2.FETCH FILE\n");
        printf("3.SEND MESSAGE\n");
        printf("4.TERMINATE CONNECTION FROM SERVER\n");
        printf("5.EXIT\n");
        printf("Enter your choice : ");
        if (scanf("%d", &choice) <= 0) {
            printf("Enter only an integer from 1 to 5\n");
            kill(child, SIGKILL);
            exit(0);
        } else {
            switch (choice) { 
               case 1:
                    keyword_to_be_send = "sea";
                    send(sock, keyword_to_be_send, strlen(keyword_to_be_send), 0);
                    printf("Enter the file name to search:\t");
                    scanf(" %[^\t\n]s", input);
                    send(sock, input, strlen(input), 0);
                    len = recv(sock, output, BUFFER, 0);
                    output[len] = '\0';
                    printf("%s\n", output);
                    bzero(output, BUFFER);
                    printf("Server searching...... Waiting for response\n");
                    printf("1.Filename 2.Filepath 3.Port No        4.Peer IP\n");
                    while ((len = recv(sock, output, BUFFER, 0)) > 0) {
                        output[len] = '\0';
                        printf("%s\n", output);
                        bzero(output, BUFFER);
                    }
                    close(sock);
                    printf("SEARCH COMPLETE!!! \n");
                    printf("DISCONNECTED FROM SERVER. GO TO OPTION 3 FOR FETCH");
                    break;

                case 2:
                    printf("Enter file path you want to download:(./upload_files/<filename.extn>):\t");
                    scanf(" %[^\t\n]s", file_fetch);
                    printf("Enter file path you want to save file:(./download_files/<filename.extn>):\t");
                    scanf(" %[^\t\n]s", file_save);
                    printf("Enter peer IP address\t");
                    scanf(" %[^\t\n]s", peer_ip);
                    printf("Enter peer listening port number\t");
                    scanf(" %[^\t\n]s", peer_port);

                    if ((peer_sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
                        perror("socket");
                        kill(child, SIGKILL);
                        exit(-1);
                    }

                    peer_connect.sin_family = AF_INET;
                    peer_connect.sin_port = htons(atoi(peer_port));
                    peer_connect.sin_addr.s_addr = inet_addr(peer_ip);
                    memset(&peer_connect.sin_zero, 0, sizeof(peer_connect.sin_zero));

                    if (connect(peer_sock, (struct sockaddr*)&peer_connect, sizeof(struct sockaddr_in)) == ERROR) {
                        perror("connect");
                        kill(child, SIGKILL);
                        exit(-1);
                    }

                    send(peer_sock, file_fetch, strlen(file_fetch), 0);
                    printf("Recieving file from peer. Please wait \n");

                    FILE* file_saving = fopen(file_save, "w");
                    if (file_saving == NULL) {
                        printf("File %s cannot be created. Error: %s\n", file_save, strerror(errno));
                    } else {
                        bzero(input, BUFFER);
                        int file_fetch_size = 0;
                        int len_recd = 0;
                        while ((file_fetch_size = recv(peer_sock, input, BUFFER, 0)) > 0) {
                            len_recd = fwrite(input, sizeof(char), file_fetch_size, file_saving);

                            if (len_recd < file_fetch_size) {
                                error("Error while writing file.Try again\n");
                                kill(child, SIGKILL);
                                exit(-1);
                            }

                            bzero(input, BUFFER);

                            if (file_fetch_size == 0 || file_fetch_size != 512) {
                                break;
                            }
                        }

                        if (file_fetch_size < 0) {
                            error("Error in recieve\n");
                            exit(1);
                        }

                        fclose(file_saving);
                        printf("FETCH COMPLETE");
                        close(peer_sock);
                    }
                    break;

                case 3:
                    printf("Enter peer IP address\t");
                    scanf(" %[^\t\n]s", peer_ip);
                    printf("Enter peer listening port number\t");
                    scanf(" %[^\t\n]s", peer_port);

                    if ((peer_sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR) {
                        perror("socket");
                        kill(child, SIGKILL);
                        exit(-1);
                    }

                    peer_connect.sin_family = AF_INET;
                    peer_connect.sin_port = htons(atoi(peer_port));
                    peer_connect.sin_addr.s_addr = inet_addr(peer_ip);
                    memset(&peer_connect.sin_zero, 0, sizeof(peer_connect.sin_zero));

                    if (connect(peer_sock, (struct sockaddr*)&peer_connect, sizeof(struct sockaddr_in)) == ERROR) {
                        perror("connect");
                        kill(child, SIGKILL);
                        exit(-1);
                    }

                    printf("Enter message:\t");
                    scanf(" %[^\t\n]s", message);
                    send(peer_sock, message, strlen(message), 0);

                    bzero(message, BUFFER);
                    int message_size = 0;
                    int len_recd = 0;
                    while ((message_size = recv(peer_sock, message, BUFFER, 0)) > 0) {
                        printf("%s", message);
                        bzero(message, BUFFER);
                        if (message_size == 0 || message_size != 512) {
                            break;
                        }
                    }

                    if (message_size < 0) {
                        error("Error in recieve\n");
                        exit(1);
                    }

                    close(peer_sock);
                    break;

                case 4:
                    keyword_to_be_send = "ter";
                    send(sock, keyword_to_be_send, strlen(keyword_to_be_send), 0);
                    close(sock);
                    printf("Connection terminated with server.\n");
                    break;

                case 5:
                    kill(child, SIGKILL);
                    close(sock);
                    return 0;

                default:
                    printf("Invalid option\n");
            }
        }
    }
    close(listen_sock);
    return 0;
}