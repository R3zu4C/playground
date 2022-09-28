#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>

const int SIZE_MAX = 100;
int socket_id;
#define ERR_SERVER "1 Server Error.\n"

void interrupt_handler(int interrupt)
{
    printf("\nClient process ending...");
    send(socket_id, ERR_SERVER, SIZE_MAX, 0);
    close(socket_id);
    exit(0);
}

int main(int argc, char *argv[])
{
    socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id < 0)
    {
        perror("Socket wasn't created!");
        return 0;
    }

    int port = atoi(argv[2]);
    char *ip_address = argv[1];
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_address);
    server.sin_port = htons(port);

    if ((connect(socket_id, (struct sockaddr *)&server, sizeof(server))) < 0)
    {
        perror("Server not responding...");
        return 0;
    }

    signal(SIGINT, interrupt_handler);

    printf("\nClient connected to server\n ");

    while (1)
    {
        char request[SIZE_MAX], response[SIZE_MAX];
        int req_len, resp_len;
        printf("\nRequest message format:\n<Request_Type> <UPC Code> <Number>\n");
        fgets(request, SIZE_MAX, stdin);
        req_len = send(socket_id, request, SIZE_MAX, 0);
        resp_len = recv(socket_id, response, SIZE_MAX, 0);

        if (response[0] == '0')
        {
            printf("\n%s\n", response);
            if (request[0] == '1')
            {
                close(socket_id);
                exit(0);
            }
        }
        else
        {
            printf("\n%s\n", response);
            if (strcmp(response, ERR_SERVER) == 0)
            {
                close(socket_id);
                exit(0);
            }
        }
    }
    return 0;
}
