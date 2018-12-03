#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include "unistd.h"

int main(int argc, char **argv){
	int sockfd;
    struct timeval tv;
    tv.tv_sec = 30; // 30 sec timeout
    FILE *file_ptr;
    char *ID;
    char *IP;
    int port;
	struct sockaddr_in server_addr;
    char buff[1000]; // recv buffer
    char msg[1000]; // send buffer

    

    // Arguments parsing
    if (argc != 4){
        printf("Wrong number of arguments. Start your program again\n");
        return -1;
    }
    else{
        ID = argv[1];
        IP = argv[2];
        port = atoi(argv[3]);
    }
    

	// create socket fd
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("socket error");
        return -1;
    }	

	// socket option
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_RCVTIMEO, (struct timeval*) &tv, sizeof(tv)); 

	// conenct to the local DNS
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        perror("connection error");
        return -1;
    }


    // Connection successful. Create a log file
    char *file_name = strcat(ID, ".log");
    file_ptr = fopen(file_name, "w+");

    while(1){
        // Wait in terminal for the client input and send to the server
        fgets(msg, 100, stdin);
        // if q is received from the client, exit the program gracefully.
        if (strcmp(msg, "q\n") == 0){
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd);
            fclose(file_ptr);
            return 0;
        }
        // send the inquery to the server
        else{
            fputs(msg, file_ptr);
            if (send(sockfd, msg, strlen(msg), 0) < 0){ 
                perror("send msg error");
            }
            else{
                // wait for the server reply until a timeout
                memset(buff, 0, sizeof(buff));
                if (recv(sockfd, buff, 1000, 0) > 0){
                    printf("%s\n", buff);
                    fprintf(file_ptr, "%s\n\n", buff); 
                }
                // Timeout or local_dns_server was closed.
                else{
                    close(sockfd);
                    printf("closing a socket with the local dns server");
                    return 0;
                }
            }
        }
    }
	
    return 0;
}
