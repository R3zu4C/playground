#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#define SIZE_MAX 100

#define EXIT_PROCESS exit(0)

#define CLIENT_TERMINATED "Client Terminated.\n"
#define ERR_READ_BUF "1 Error while reading buffer.\n"
#define ERR_PROTOCOL "1 Protocol Error.\n"
#define ERR_UPC "1 UPC is not found in database.\n"
#define ERR_SERVER "1 Server Error.\n"

int listen_fd;
int connection_fd;

typedef struct database
{
    char item_name[SIZE_MAX];
    int item_upc;
    float item_price;
} db;

db *data_pointer;
int num_of_records;

void child_process(int connection_fd)
{
    int resp_length, token_idx, req_type, item_upc, item_qty, db_item_idx;
    float total = 0.0;
    char resp[SIZE_MAX], *token, log[SIZE_MAX];

    while (1)
    {
        resp_length = 0;
        token_idx = 0;
        db_item_idx = 0;
        item_qty = 0;

        bzero(log, SIZE_MAX);
        resp_length = recv(connection_fd, resp, SIZE_MAX, 0);

        if (resp_length < 0)
        {
            sprintf(log, ERR_READ_BUF);
            send(connection_fd, log, SIZE_MAX, 0);
            close(connection_fd);
            EXIT_PROCESS;
        }

        if (strcmp(resp, CLIENT_TERMINATED) == 0)
        {
            printf(CLIENT_TERMINATED);
            close(connection_fd);
            EXIT_PROCESS;
        }
        token = strtok(resp, " ");

        if (!token)
            continue;

        req_type = atoi(token);

        while (token != NULL)
        {
            token = strtok(NULL, " ");
            if (token == NULL)
                break;
            if (token_idx == 0)
            {
                item_upc = atoi(token);
            }
            if (token_idx == 1)
            {
                item_qty = atoi(token);
            }

            token_idx++;
        }

        if (req_type != 0 && req_type != 1)
        {
            sprintf(log, ERR_PROTOCOL);
            send(connection_fd, log, SIZE_MAX, 0);
        }
        if (token_idx == 2)
        {
            if (req_type == 0)
            {
                db_item_idx = get_item(item_upc);
                if (db_item_idx == -1)
                {
                    sprintf(log, ERR_UPC);
                    send(connection_fd, log, SIZE_MAX, 0);
                }
                else
                {
                    total += (data_pointer[db_item_idx].item_price * item_qty);
                    sprintf(log, "0 %f\t%s\n", data_pointer[db_item_idx].item_price, data_pointer[db_item_idx].item_name);
                    send(connection_fd, log, SIZE_MAX, 0);
                }
            }
            else if (req_type == 1)
            {
                sprintf(log, "0 %f", total);
                send(connection_fd, log, SIZE_MAX, 0);
                close(connection_fd);
                EXIT_PROCESS;
            }
        }
        else
        {
            sprintf(log, ERR_PROTOCOL);
            send(connection_fd, log, SIZE_MAX, 0);
        }
    }
}

void import_db(char *file_name)
{
    FILE *fp;
    char *token, buf[SIZE_MAX];
    int token_idx = 0, cnt = 0;
    char c;

    fp = fopen(file_name, "r");

    for (c = getc(fp); c != EOF; c = getc(fp))
    {
        if (c == '\n')
        {
            num_of_records = num_of_records + 1;
        }
    }

    data_pointer = (db *)malloc(num_of_records * sizeof(db));
    fseek(fp, 0, SEEK_SET);
    while (fgets(buf, SIZE_MAX, fp) != NULL)
    {
        token = strtok(buf, " ");
        data_pointer[cnt].item_upc = atoi(token);
        token_idx = 0;

        while (token != NULL)
        {
            token = strtok(NULL, " ");

            if (token_idx == 0)
                strcpy(data_pointer[cnt].item_name, token);
            else if (token_idx == 1)
                data_pointer[cnt].item_price = atof(token);

            token_idx++;
        }

        cnt++;
    }

    return;
}

int get_item(int item_upc)
{
    int cnt;

    for (cnt = 0; cnt < num_of_records; cnt++)
    {
        if (item_upc == data_pointer[cnt].item_upc)
        {
            return cnt;
        }
    }

    return -1;
}

void interrupt_handler(int interrupt)
{
    close(listen_fd);
    printf("Server process ended.\n");

    send(connection_fd, ERR_SERVER, SIZE_MAX, 0);

    EXIT_PROCESS;
}

int main(int argc, char *argv[])
{
    import_db("database.txt");

    int client_length;
    pid_t child_process_id;
    struct sockaddr_in server_addr, client_addr;

    int port = atoi(argv[1]);

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0)
    {
        perror("Socket wasn't created!");
        return 0;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Port with client could not be bound!");
        return 0;
    }

    listen(listen_fd, SOMAXCONN);

    signal(SIGINT, interrupt_handler);

    while (1)
    {
        client_length = sizeof(client_addr);

        connection_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_length);
        if (connection_fd < 0)
        {
            perror("Connection could not be established with the client!");
            return 0;
        }

        child_process_id = fork();
        if (child_process_id == 0)
        {
            close(listen_fd);
            pid_t child_pid = getpid();
            printf("\nClient request accepted with a new child process created with id %d\n", child_pid);
            child_process(connection_fd);

            close(connection_fd);
            EXIT_PROCESS;
        }

        close(connection_fd);
    }
}
