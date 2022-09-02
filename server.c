#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>


#define MAXLINE 100

#define EXIT_PROCESS exit(0)

#define CLIENT_TERMINATED "Client Terminated.\n"
#define ERR_READ_BUF "1 Error while reading buffer.\n"
#define ERR_PROTOCOL "1 Protocol Error.\n"
#define ERR_UPC "1 UPC is not found in database.\n"
#define ERR_SERVER "1 Server Error.\n"



void signal_handler(int sig); // programmer-defined signal handler for Ctrl+C command

typedef struct database
{
	char item_name[MAXLINE];
	int item_upc;
	float item_price;
} db;

db *data_pointer;
int num_of_records; // keeps track of number of num_of_records in the database

void create_database(char *); // creates database
int check_code(int);		  // searches and returns item_upc supplied in the database and returns accordingly
void child_process(int);

int listen_fd;
int connection_fd; // sd newsd are socket descriptors

int main(int argc, char *argv[])
{
	// creating database create_database(argv[1]);
	create_database("database.txt");

	int client_length;							 // client_length is the length of the client socket, used as a value-result argument
	pid_t child_process_id;						 // holds process id of child
	struct sockaddr_in server_addr, client_addr; // sockaddr structure for sockets; one for server and the other for client

	if (argc < 2)
	{
		printf("\nToo few arguments to server!..exiting");
		EXIT_PROCESS;
	}
	int PORT = atoi(argv[1]);
	// creating the socket
	listen_fd = socket(AF_INET, SOCK_STREAM, 0); // socket(internet_family,socket_type,protocol_value) retruns socket descriptor
	if (listen_fd < 0)
	{
		perror("Cannot create socket!");
		return 0;
	}

	// initializing the server socket
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY; // using the loopback IP
	server_addr.sin_port = htons(PORT);		  // self defined server port

	// checking server ip and port number
	// printf("Server IP address is %s\n", inet_ntoa(server_addr.sin_addr));
	// printf("Socket ID=%d, Sever Port=%d\n",sd,PORT);

	// binding socket
	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		perror("Cannot bind port!");
		return 0;
	}

	// defining number of clients that can connect through PORT , listen() indicates that server is ready for connections
	listen(listen_fd, SOMAXCONN);

	signal(SIGINT, signal_handler);

	// server runs an infinite loop

	while(1)
	{
		client_length = sizeof(client_addr);
		
		connection_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_length);
		if(connection_fd < 0)
		{
			perror("Cannot establish connection!");
			return 0;
		}

		child_process_id = fork();
		if (child_process_id == 0)
		{
			close(listen_fd); // child process closes its copy of the listening socket since it is going to service clients through connection_fd
			pid_t child_pid = getpid();
			printf("\nClient request accepted with a new child process created with id %d\n", child_pid);
			child_process(connection_fd); // child process services request

			close(connection_fd); // child closes its version of connection_fd after computation is done (return from child_process())
			EXIT_PROCESS;			  // child terminates
		}

		close(connection_fd); // parent closes the connected socket and begins listening for more connections
	}
}

void child_process(int connection_fd)
{
	int buf_length, token_idx, req_type, item_upc, item_qty, db_item_idx;
	float total = 0.0;
	char buf[MAXLINE], *token, msg[MAXLINE];

	while(1)
	{
		buf_length = 0;
		token_idx = 0;
		db_item_idx = 0;
		item_qty = 0;

		memset(msg, 0, MAXLINE); // clears contents of msg
		buf_length = recv(connection_fd, buf, MAXLINE, 0);

		if (buf_length < 0)
		{
			sprintf(msg, ERR_READ_BUF);
			send(connection_fd, msg, MAXLINE, 0);
			close(connection_fd);
			EXIT_PROCESS;
		}

		if (strcmp(buf, CLIENT_TERMINATED) == 0)
		{
			printf(CLIENT_TERMINATED);
			close(connection_fd);
			EXIT_PROCESS;
		}
		token = strtok(buf, " ");
        
        if(!token) continue;
		req_type = atoi(token); // tokenising request type from command

		while (token != NULL)
		{
            // fputs("here-3\n", stderr);
			token = strtok(NULL, " ");
            
			if (token_idx == 0) {
				item_upc = atoi(token);
            }
			if (token_idx == 1) {
				item_qty = atoi(token);
            }

			token_idx++;
		}
        // printf("%d\n", req_type);
        // printf("%d\n", token_idx);

        // if(req_type != 0 && req_type != 1) {
        //     sprintf(msg, ERR_PROTOCOL);
        //     send(connection_fd, msg, MAXLINE, 0);
        // }

        // fputs("here-4\n", stderr);
		if (token_idx >= 2)
		{
			if (req_type == 0)
			{
				db_item_idx = check_code(item_upc);
				if (db_item_idx == -1)
				{
					sprintf(msg, ERR_UPC);
					send(connection_fd, msg, MAXLINE, 0);
				}
				else
				{
					total += (data_pointer[db_item_idx].item_price * item_qty);
					sprintf(msg, "0 %f\t%s\n", data_pointer[db_item_idx].item_price, data_pointer[db_item_idx].item_name);
					send(connection_fd, msg, MAXLINE, 0);
				}
			}
			else if (req_type == 1)
			{
				sprintf(msg, "0 %f", total);
				send(connection_fd, msg, MAXLINE, 0);
				close(connection_fd);
				EXIT_PROCESS;
			}
		}
		else
		{
			sprintf(msg, ERR_PROTOCOL);
			send(connection_fd, msg, MAXLINE, 0);
		}
	}
}

void create_database(char *file_name)
{
	FILE *fp;
	const char delim[2] = " "; // delim[0] - for delimiter, delim[1]='\0'
	char *token, line_buff[MAXLINE];
	int token_idx = 0, ctr = 0;

	fp = fopen(file_name, "r");

	if (fp == NULL)
	{
		printf("\nCannot open %s", file_name);
		return;
	}

	fgets(line_buff, MAXLINE, fp); // reads the first line that has the number of entries in the text file

	num_of_records = atoi(line_buff); // gets number of num_of_records
	// printf("\nNUmber of num_of_records = %d",num_of_records);

	data_pointer = (db *)malloc(num_of_records * sizeof(db)); // allocates memory for database

	while (fgets(line_buff, MAXLINE, fp) != NULL)
	{
		token = strtok(line_buff, delim);
		data_pointer[ctr].item_upc = atoi(token); // getting the item code
		token_idx = 0;

		while (token != NULL)
		{
			token = strtok(NULL, delim);

			if (token_idx == 0) // getting the name of item
				strcpy(data_pointer[ctr].item_name, token);
			else if (token_idx == 1)
				data_pointer[ctr].item_price = atof(token); // get item price
			/*else
			{
				printf("\ntokenisation error..exiting\n");
				return 0;
			}*/

			token_idx++;
		}

		ctr++;
	}

	return;
}

int check_code(int item_upc)
{
	int ctr;

	for (ctr = 0; ctr < num_of_records; ctr++)
	{
		if (item_upc == data_pointer[ctr].item_upc)
		{
			return (ctr);
		}
	}

	return (-1);
}

void signal_handler(int sig)
{
	char msg[MAXLINE] = ERR_SERVER;

	close(listen_fd);
	fputs("Server terminating.\n", stdout);

	send(connection_fd, msg, MAXLINE, 0);

	EXIT_PROCESS;
}
