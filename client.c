//cash register client 
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<ctype.h>

const int SIZE_MAX=250;
int socket_id; //client socket descriptor
#define ERR_SERVER "1 Server Error.\n"

void ctrl_c_handle(int interrupt)
{
    printf("\nClient process ending...");
    send(socket_id,"-256",5,0);
    close(socket_id);
    exit(0);
}

int main(int argc, char *argv[])
{
    //creating the socket
    socket_id=socket(AF_INET,SOCK_STREAM,0); //socket(internet_family,socket_type,protocol_value) returns socket descriptor
    if(socket_id<0) 
    {
        perror("Socket wasn't created!"); //prints on stderr
        return 0;
    }
    
    //initializing the server socket
    int port = atoi(argv[2]);
    char* ip_address=argv[1];
    struct sockaddr_in server; //structure object helps in binding the socket to a particular address
    server.sin_family=AF_INET;
    server.sin_addr.s_addr = inet_addr(ip_address); //using the input IP 
    server.sin_port = htons(port); //self defined server port

    if((connect(socket_id,(struct sockaddr *) &server,sizeof(server)))<0)
    {
        perror("Server not responding...");
        return 0;
    }
    //Server connected now

    signal(SIGINT,ctrl_c_handle);

    printf("\nClient connected to server\n ");

    while(1)
    {
        char request[SIZE_MAX],response[SIZE_MAX];
        printf("\nRequest message format:\n<Request_Type> <UPC Code> <Number>\n");
        fgets(request, SIZE_MAX, stdin);
        //fgets(request,SIZE_MAX,stdin);
        send(socket_id,request,SIZE_MAX,0);
        recv(socket_id,response,SIZE_MAX,0);

        printf("aasadaa: %s", request);
        printf("aaaa: %s", response);

        if(response[0]=='0')
        {
            printf("\n%s\n",response);
            if(request[0]=='1') // client close 
            {
                close(socket_id);
                exit(0);
            }
        }
        else // error
        {
            printf("\n%s\n",response);
            if(strcmp(response, ERR_SERVER) == 0){
                close(socket_id);
                exit(0);
            }
        }
    }
    return 0;
}
