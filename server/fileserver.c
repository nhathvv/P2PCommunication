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
#include <time.h>
#include <unistd.h>

#define ERROR -1
#define MAX_CLIENTS 4
#define MAX_BUFFER 512

int add_IP(char *);
int update_IPlist(char *);

time_t current_time;

int main(int argc, char **argv)
{
    int sock, newClient;
    struct sockaddr_in server, client;
    int sockaddr_len = sizeof(struct sockaddr_in);

    char buffer[MAX_BUFFER];
    char file_name[MAX_BUFFER];
    char *peer_ip;

    char user_key[MAX_BUFFER];
    int len;

    int child;

    if (argc < 2)
    {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(1);
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == ERROR)
    {
        perror("server socket error: ");
        exit(-1);
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[1]));
    server.sin_addr.s_addr = INADDR_ANY;
    bzero(&server.sin_zero, 8);

    if ((bind(sock, (struct sockaddr *)&server, sizeof(server))) == ERROR)
    {
        perror("bind");
        exit(-1);
    }

    if ((listen(sock, MAX_CLIENTS)) == ERROR)
    {
        perror("listen");
        exit(-1);
    }

    while (1)
    {
        if ((newClient = accept(sock, (struct sockaddr *)&client, &sockaddr_len)) == ERROR)
        {
            perror("ACCEPT.Error accepting newClient connection");
            exit(-1);
        }

        child = fork();

        if (child == -1)
        {
            perror("fork");
            exit(-1);
        }

        if (!child)
        {
            close(sock);

            printf("New client connected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
            peer_ip = inet_ntoa(client.sin_addr);
            add_IP(peer_ip);

            while (1)
            {
                len = recv(newClient, buffer, MAX_BUFFER, 0);
                buffer[len] = '\0';
                printf("%s\n", buffer);

                if (len <= 0)
                {
                    if (len == 0)
                    {
                        printf("Peer %s disconnected\n", inet_ntoa(client.sin_addr));
                        update_IPlist(peer_ip);
                    }
                    else
                    {
                        perror("ERROR IN RECEIVE");
                    }
                    close(newClient);
                    exit(0);
                }

                if (buffer[0] == 'p' && buffer[1] == 'u' && buffer[2] == 'b')
                {
                    // PUBLISH OPERATION
                    char *fileinfo = "filelist.txt";
                    FILE *filedet = fopen(fileinfo, "a+");

                    if (filedet == NULL)
                    {
                        printf("ERROR. Unable to open File");
                        return 1;
                    }
                    else
                    {
                        fwrite("\n", sizeof(char), 1, filedet);

                        len = recv(newClient, file_name, MAX_BUFFER, 0);
                        fwrite(&file_name, len, 1, filedet);
                        char Report[] = "File published";
                        send(newClient, Report, sizeof(Report), 0);

                        fwrite("\t", sizeof(char), 1, filedet);

                        peer_ip = inet_ntoa(client.sin_addr);

                        fwrite(peer_ip, 1, strlen(peer_ip), filedet);
                        fclose(filedet);
                        printf("%s\n", "FILE PUBLISHED");
                    }
                }
                else if (buffer[0] == 's' && buffer[1] == 'e' && buffer[2] == 'a')
                {
                    // SEARCH OPERATION
                    bzero(buffer, MAX_BUFFER);

                    len = recv(newClient, user_key, MAX_BUFFER, 0);
                    char Report3[] = "Keyword received";
                    send(newClient, Report3, sizeof(Report3), 0);
                    user_key[len] = '\0';

                    FILE *file_list = fopen("filelist.txt", "r");
                    FILE *search_list = fopen("searchresult.txt", "a+");
                    if (file_list == NULL || search_list == NULL)
                    {
                        fprintf(stderr, "ERROR while opening file on server.");
                        exit(1);
                    }

                    char line[MAX_BUFFER];
                    int flag = 0;
                    while (fgets(line, MAX_BUFFER, file_list) != NULL)
                    {
                        if ((strstr(line, user_key)) != NULL)
                        {
                            fwrite(line, 1, strlen(line), search_list);
                            fwrite("\n", sizeof(char), 1, search_list);
                            flag = 1;
                            break;
                        }
                    }

                    if (flag == 0)
                    {
                        char not_found[] = "Sorry. Desired file not found";
                        fwrite(not_found, 1, strlen(not_found), search_list);
                        fwrite("\n", sizeof(char), 1, search_list);
                    }
                    fclose(search_list);
                    fclose(file_list);

                    char *search_result = "searchresult.txt";
                    FILE *file_search = fopen(search_result, "r");
                    if (file_search == NULL)
                    {
                        fprintf(stderr, "ERROR while opening file on server.");
                        exit(1);
                    }

                    bzero(buffer, MAX_BUFFER);
                    int file_search_send;
                    while ((file_search_send = fread(buffer, sizeof(char), MAX_BUFFER, file_search)) > 0)
                    {
                        len = send(newClient, buffer, file_search_send, 0);

                        if (len < 0)
                        {
                            fprintf(stderr, "ERROR: File not found");
                            exit(1);
                        }
                        bzero(buffer, MAX_BUFFER);
                    }
                    fclose(file_search);
                    char Reportsearch[] = "Search complete. You are disconnected from the server now. Connect again for further actions";
                    send(newClient, Reportsearch, sizeof(Reportsearch), 0);

                    printf("Search complete!!!!\n");
                    printf("Client disconnected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
                    peer_ip = inet_ntoa(client.sin_addr);
                    update_IPlist(peer_ip);

                    close(newClient);
                    exit(0);
                }
                else if (buffer[0] == 't' && buffer[1] == 'e' && buffer[2] == 'r')
                {
                    // TERMINATE OPERATION
                    printf("Client disconnected from port no %d and IP %s\n", ntohs(client.sin_port), inet_ntoa(client.sin_addr));
                    peer_ip = inet_ntoa(client.sin_addr);
                    update_IPlist(peer_ip);

                    close(newClient);
                    exit(0);
                }
            }
        }
    }
    printf("Server shutting down \n");
    close(sock);
}

int update_IPlist(char *peer_ip)
{
    char *peerinfo = "peerIPlist.txt";
    FILE *peerdet = fopen(peerinfo, "a+");

    if (peerdet == NULL)
    {
        printf("Unable to open IPList File.Error");
        return -1;
    }
    else
    {
        fwrite("\n", sizeof(char), 1, peerdet);
        fwrite(peer_ip, 1, strlen(peer_ip), peerdet);
        char update[] = "  disconnected from server at ";
        fwrite(update, 1, strlen(update), peerdet);
        current_time = time(NULL);
        fwrite(ctime(&current_time), 1, strlen(ctime(&current_time)), peerdet);
        fclose(peerdet);
    }
}

int add_IP(char *peer_ip)
{
    char *peerinfo = "peerIPlist.txt";
    FILE *peerdet = fopen(peerinfo, "a+");

    if (peerdet == NULL)
    {
        printf("Unable to open IPList File.Error");
        return -1;
    }
    else
    {
        fwrite("\n", sizeof(char), 1, peerdet);
        fwrite(peer_ip, 1, strlen(peer_ip), peerdet);
        char update[] = "  connected to server at ";
        fwrite(update, 1, strlen(update), peerdet);
        current_time = time(NULL);
        fwrite(ctime(&current_time), 1, strlen(ctime(&current_time)), peerdet);
        fclose(peerdet);
    }
}
