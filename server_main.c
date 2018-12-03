#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "unistd.h"
#include "server_routine.h"


int main(int argc, char *argv[]){

    char logfile_name[100];
    char *default_mappingdat_name;
    char  *serverdat_name;
    const int opt = 1;
    char *IP;

    signal(SIGINT, INThandler);

    // checking arguments
    if (argc < 3){
        printf("Wrong Number of arguments. Exiting\n");
        return 0;
    }

    // fetch arguments
    ID = argv[1];
    port = atoi(argv[2]);
    if (argc >= 4){
        default_mappingdat_name = argv[3]; 
        serverdat_name = argv[4];
    }
    

	// create socket fd
	sockfd = socket(AF_INET, SOCK_STREAM , 0);
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)); 
	
	// bind
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) < 0){
        perror("bind error");
        return 0;
    }

    // listen
    if (listen(sockfd, 3) == -1){
        perror("listen error");
        return 0;
    }


    // Create log file
    sprintf(logfile_name, "%s.log", ID);
    files.IDlog = fopen(logfile_name, "w+");



    // The following statements open up the routine for each individual server.
    // If this program is ran by local_dns, create mapping file as well
    if (argc == 3){
        files.mapping = fopen("mapping.log", "w+"); 
        local_dns_routine(NULL, NULL);
    }
    else{
        // If this program is ran by root_dns, open server.dat
        if (strcmp(default_mappingdat_name, "null") == 0){
            files.serverdat = fopen(serverdat_name, "r");
            if (files.serverdat == NULL){
                printf("File not found. Exiting\n");
                close(sockfd);
                return -1;
            }
            root_dns_routine(NULL); 
        }
        // If this program is ran by server_dns, read appropriate dat files    
        else if (strcmp(serverdat_name, "null") == 0){
            files.default_mappingdat = fopen(default_mappingdat_name, "r");
            if (files.default_mappingdat == NULL){
                printf("File not found. Exiting\n");
                close(sockfd);
                return -1;
            }
            dns_servers_routine(NULL); 
        }
        // Arguments were wrong. Exit program
        else{   
            printf("Wrong arguments. Exiting\n");            
        }
    }


    close(sockfd);

	return 0;
}




