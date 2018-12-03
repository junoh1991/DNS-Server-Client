#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include "unistd.h"
#include "server_routine.h"


// Forever-loop to handle local dns server.
// Upon receiving request from the client, this will spawn a worker thread to further process the request.
void local_dns_routine(FILE* logfile, FILE* mappingfile){
    int i = 0;
    pthread_t worker_threads[1000];
	addrlen = sizeof(server_addr);

    while (1){
        // The main program waits until a connection is to be made
        int sock_to_client;
        sock_to_client = accept(sockfd, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen);

        if (sock_to_client < 0){
            perror("accept");   
        }
        else{
            pthread_create(&worker_threads[i++], NULL, local_dns_worker, (void*)&sock_to_client);
            if (i >= 1000) i = 0;
        }
    }

	close(sockfd);
}



// Upon the thread spawn, it will parse the request to pass into other helper function.
// It checks the parsed request to check the format of the request.
// Then, it sends request to the root dns server.
// If iterative, it expects to get dns server ip and port. And, connect to the dns server to retreive the ip.
// If recursive, it expects to get the resolved ip directly from the root.
// For each query to the respective server, it will make a connction and close the socket right after each request.
void *local_dns_worker(void *arg){
    int sock_to_client = *(int*)arg;
    char buff[1000];
    char msg[100];
    char* client_ID;
    char* client_hostname;
    char* client_request_type;

    while(1){
        // Check if the client connection is received.
        memset(buff, 0, sizeof(buff));
        if (recv(sock_to_client, buff, 1000, 0) <= 0){
            close(sock_to_client);
            continue;
        }
        // Else, write to the log file
        else{
            fputs(buff, files.IDlog);
        }
    
        // Parse the request and check the format of the string
        client_ID = strtok(buff, " ,");
        client_hostname = strtok(NULL, " ,");
        client_request_type = strtok(NULL, " ,");



        // Make sure all three arguments are there
        if (client_ID == NULL || client_hostname == NULL || client_request_type == NULL){
            sprintf(msg, "0xEE, %s, \"Invalid format\"", ID);
            if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                perror("send msg error");
            continue;
        }


        
        // lowercase the hostname.
        int i;
        char* hostname_lowercase;
        // lowercase the request
        for (i = 0; client_hostname[i]; i ++)
            client_hostname[i] = tolower((unsigned char) client_hostname[i]);
        
        // analyze hostname to see if it ends with com, org, or gov 
        if(strlen(client_hostname) > 4 && !strcmp(client_hostname + strlen(client_hostname) - 4, ".com")){
        }
        else if(strlen(client_hostname) > 4 && !strcmp(client_hostname + strlen(client_hostname) - 4, ".org")){
        }
        else if(strlen(client_hostname) > 4 && !strcmp(client_hostname + strlen(client_hostname) - 4, ".gov")){
        }
        else{ // There was an issue with the format. 
            sprintf(msg, "0xEE, %s, \"Invalid format\"", ID); 
            fputs(msg, files.IDlog);
            fputs("\n", files.IDlog);
            fputs("\n", files.IDlog);
            if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                perror("send msg error");
            continue;
        }


        // Before connecting to servers, check the mapping file if the requested ip address exists in the mapping.log
        memset(msg, 0, sizeof(msg));
        char line[100];
        rewind(files.mapping);
        int found = 0;
        while(fgets(line, 100, files.mapping) != 0){
            char *newline;
            char* hostname_revised;
            char* request_hostname_revised;


            // get rid of www of mappling file hostname for checking purpose
            if (strncmp(line, "www.", 4) == 0)
                hostname_revised = &line[0] + 4;
            else
                hostname_revised = &line[0];
            
            // get rid of www of clinet request hostname for checking purpose
            if (strncmp(client_hostname, "www.", 4) == 0)
                request_hostname_revised = &client_hostname[0] + 4;
            else
                request_hostname_revised = &client_hostname[0];
            

            // match found in mapping.log file
            if (strcmp(request_hostname_revised, newline = strtok(hostname_revised, " ")) == 0){
                sprintf(msg, "0x00, %s, %s", ID, strtok(NULL, " ,^M\n\0")); 
                fputs(msg, files.IDlog);
                fputs("\n\n", files.IDlog);
                found = 1;
                if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
                break;
            }

        }
        
        fseek(files.mapping, 0, SEEK_END);
        if (found)
            continue;



        // If iterative case, request IP from the root
        // Then, request IP from the server
        if (client_request_type[0] == 'I') {
            char request[1000];
            sprintf(request,"%s, %s, I", ID, client_hostname);
            int server_port;
            char* server_IP;
            fputs(request, files.IDlog);
            fputs("\n", files.IDlog);

            // First, connect to the root server. Retrieve IP and port number.
            // Check response.
            char *response = fetch_iterative(request, ROOT_PORT);
            if (response == NULL){
                char  msg[1000];
                sprintf(msg, "0xFF, %s, \"Host not found\"", ID); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
                continue;
            }
            // successfully got answer from root.
            // Check if answered with 0XEE "invalid format"
            else{
                fputs(response, files.IDlog);
                fputs("\n", files.IDlog);
                char* identifier = strtok(response, " ,");
                char msg[100];
                if (strcmp(identifier, "0xEE") == 0){
                    sprintf(msg, "0xEE, %s, \"Invalid format\"", ID); 
                    fputs(msg, files.IDlog);
                    fputs("\n", files.IDlog);
                    fputs("\n", files.IDlog);
                    if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                        perror("send msg error");
                    continue;
                }
                strtok(NULL, " ,");
                server_IP = strtok(NULL, " ,");
                server_port = atoi(strtok(NULL, " \0\n")); 
            }

            // If first connection with root server is successful, connect to the dns server.
            char *response2 = fetch_iterative(request, server_port);
            char  msg[1000];
            fputs(request, files.IDlog);
            fputs("\n", files.IDlog);
            if (response2 == NULL){
                sprintf(msg, "0xFF, %s, \"Host not found\"", ID); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
                continue;
            }
            else{
                fputs(response2, files.IDlog);
                fputs("\n", files.IDlog);


                // TO DO: parse resovled IP address
                char* resolvedIP = response2;
                strtok(response2, " ,");
                strtok(NULL, " ,");
                resolvedIP = strtok(NULL, " ,");
        
                memset(msg, 0, sizeof(buff));
                // Check if recieved 0xFF
                if (strncmp(response2, "0xFF", 4) == 0){
                    sprintf(msg, "0xFF, %s, \"Host not found\"", ID); 
                    fputs(msg, files.IDlog);
                    fputs("\n", files.IDlog);
                    if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                        perror("send msg error");
                }
                else{
                    // Send the resolved IP address back to the client
                    sprintf(msg, "0x00, %s, %s", ID, resolvedIP); 
                    fputs(msg, files.IDlog);
                    fputs("\n", files.IDlog);
                   
                    if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                        perror("send msg error");

                    // Store in mapping.log as well
                    char mapping_line[100];
                    sprintf(mapping_line, "%s %s\n", client_hostname ,resolvedIP);
                    fputs(mapping_line, files.mapping);
                    fputs("\n", files.IDlog);
                }
                
            }
        }

        // if recursive case, ask root to directly parse the IP from dns server.
        else if (client_request_type[0]  == 'R'){
            char request[100]; 
            sprintf(request, "%s, %s, R", ID, client_hostname); 
            fputs(request, files.IDlog);
            fputs("\n", files.IDlog);

            char *response = fetch_iterative(request, ROOT_PORT); 
            
            // Got response from the root server. Send back to client
            fputs(response, files.IDlog);
            fputs("\n", files.IDlog);

            
            // TO DO: parse resovled IP address
            char* resolvedIP = response;
            strtok(response, " ,");
            strtok(NULL, " ,");
            resolvedIP = strtok(NULL, " ,");
    
            // Check if recieved 0xFF
            if (strncmp(response, "0xFF", 4) == 0){
                sprintf(msg, "0xFF, %s, \"Host not found\"", ID); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
            }
            else{
                // Send the resolved IP address back to the local dns server
                sprintf(msg, "0x00, %s, %s", ID, resolvedIP); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
                // Store in mapping.log as well
                char mapping_line[100];
                sprintf(mapping_line, "%s %s\n", client_hostname ,resolvedIP);
                fputs(mapping_line, files.mapping);
                fputs("\n", files.IDlog);
            }
        }

        // Request type was wrong
        // Send error back
        else{
            char msg[100];
            sprintf(msg, "0xEE, %s, \"Invalid format\"", ID);
            fputs(msg, files.IDlog);
            fputs("\n", files.IDlog);
            if (send(sock_to_client, msg, strlen(msg), 0) < 0) 
                perror("send msg error");
        }

        
        fputs("\n", files.IDlog);
    }
}





void root_dns_routine(FILE* logfile){
    int i = 0;
    pthread_t worker_threads[1000];
	addrlen = sizeof(server_addr);
    int sock_to_localdns;

    // parse server.dat file
    char line[100];
    char *tld;
    char *port;
    char *ip;


    // Parse server.dat file to struct to handle iterative request to the local dns server.
    while(fgets(line, 100, files.serverdat) != 0){
        tld  = strtok(line, " \t");
        ip = strtok(NULL, " \t");
        port = strtok(NULL, " \n\0");


        if (strcmp(tld, "com") == 0){
            comDNS.IP = ip;
            comDNS.port = atoi(port); 
        }
        else if (strcmp(tld, "org") == 0){
            orgDNS.IP = ip;
            orgDNS.port = atoi(port);
        }
        else if (strcmp(tld, "gov") == 0){
            govDNS.IP = ip;
            govDNS.port = atoi(port);
        }
    }

    while (1){
        // The main program waits until a connection is to be made
        sock_to_localdns = accept(sockfd, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen);

        if (sock_to_localdns < 0){
            perror("accept");   
        }
        else{
            pthread_create(&worker_threads[i++], NULL, root_dns_worker, (void*)&sock_to_localdns);
            if (i >= 1000) i = 0;
        }
    }
	close(sockfd);
}

void *root_dns_worker(void *arg){
    int sock_to_localdns = *(int*)arg;
    char buff[1000];
    char* localdns_ID;
    char* localdns_hostname;
    char* localdns_request_type;

    while(1){
        // Check if the localdns connection is received.
        memset(buff, 0, sizeof(buff));
        if (recv(sock_to_localdns, buff, 1000, 0) <= 0){
            close(sock_to_localdns);
            break;
        }
        // Else, write to the log file
        else{
            fputs(buff, files.IDlog);
            fputs("\n", files.IDlog);
        }

        // Parse the request and check the format of the string
        localdns_ID = strtok(buff, " ,");
        localdns_hostname = strtok(NULL, " ,");
        localdns_request_type = strtok(NULL, " \0\n");
        

        // iterative request. Read and return port and IP of the dns server
        // To do so, parse server.dat
        char msg[100];
        if (localdns_request_type[0] == 'I') {
            // analyze hostname to see if it ends with com, org, or gov 
            if(strlen(localdns_hostname) > 4 && !strcmp(localdns_hostname + strlen(localdns_hostname) - 4, ".com")){
                sprintf(msg, "0x01, %s, %s, %d", ID, comDNS.IP, comDNS.port); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_localdns, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
            }
            else if(strlen(localdns_hostname) > 4 && !strcmp(localdns_hostname + strlen(localdns_hostname) - 4, ".org")){
                sprintf(msg, "0x01, %s, %s, %d", ID, orgDNS.IP, orgDNS.port); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_localdns, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
            }
            else if(strlen(localdns_hostname) > 4 && !strcmp(localdns_hostname + strlen(localdns_hostname) - 4, ".gov")){
                sprintf(msg, "0x01, %s, %s, %d", ID, govDNS.IP, govDNS.port); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_localdns, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
            }
            else{ // There was an issue with the format. 
                sprintf(msg, "0xEE, %s, \"Invalid format\"", ID); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_localdns, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
            }
        }
        // recursive request. Connect to the dns server and retrieve the IP
        else if(localdns_request_type[0] == 'R'){
            // analyze hostname to see if it ends with com, org, or gov 
            char *response;
            if(strlen(localdns_hostname) > 4 && !strcmp(localdns_hostname + strlen(localdns_hostname) - 4, ".com")){
                sprintf(msg, "%s, %s, R", ID, localdns_hostname); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                response = fetch_iterative(msg, comDNS.port);
            }
            else if(strlen(localdns_hostname) > 4 && !strcmp(localdns_hostname + strlen(localdns_hostname) - 4, ".org")){
                sprintf(msg, "%s, %s, R", ID, localdns_hostname); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                response = fetch_iterative(msg, orgDNS.port);
            }
            else if(strlen(localdns_hostname) > 4 && !strcmp(localdns_hostname + strlen(localdns_hostname) - 4, ".gov")){
                sprintf(msg, "%s, %s, R", ID, localdns_hostname); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                response = fetch_iterative(msg, govDNS.port);
            }
            else{ // There was an issue with the format. 
                sprintf(msg, "0xEE, %s, \"Invalid format\"", ID); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_localdns, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
                fputs("\n", files.IDlog);
                continue;
            }

            // Got response from the dns server. Send back to local_dns_server
            fputs(response, files.IDlog);
            fputs("\n", files.IDlog);

            
            // TO DO: parse resovled IP address
            char* resolvedIP = response;
            strtok(response, " ,");
            strtok(NULL, " ,");
            resolvedIP = strtok(NULL, " ,");
    
            // Check if recieved 0xFF
            if (strncmp(response, "0xFF", 4) == 0){
                sprintf(msg, "0xFF, %s, \"Host not found\"", ID); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_localdns, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
            }
            else{
                // Send the resolved IP address back to the local dns server
                sprintf(msg, "0x00, %s, %s", ID, resolvedIP); 
                fputs(msg, files.IDlog);
                fputs("\n", files.IDlog);
                if (send(sock_to_localdns, msg, strlen(msg), 0) < 0) 
                    perror("send msg error");
            }
            
            
        }

        fputs("\n", files.IDlog);
    }
}






void dns_servers_routine(FILE* logfile){
    int i = 0;
    pthread_t worker_threads[1000];
    addrlen = sizeof(server_addr);
    char line[100];

    // Fetch tld dat files and story in array. store in lower case and without www. prefix.
    int j = 0;
    while(fgets(line, 100, files.default_mappingdat) != 0){
        char *newline;

        for (i = 0; line[i]; i ++)
            line[i] = tolower((unsigned char) line[i]);

        if (strncmp(line, "www.", 4) == 0){
            newline = &line[0] + 4;
            datArr[j] = strdup(newline);
        }
        else{
            datArr[j] = strdup(line);
        }
        j++;
    }
    datArr_column_size = j;
    i = 0;
    while (1){
        int new_sock;
        new_sock = accept(sockfd, (struct sockaddr*)&server_addr, (socklen_t*)&addrlen);

        if (new_sock < 0){
            perror("accept");   
        }
        else{
            pthread_create(&worker_threads[i++], NULL, server_dns_worker, (void*)&new_sock);
            if (i >= 1000) i = 0;
        }
    }
}


void *server_dns_worker(void *arg){
    int new_sock = *(int*)arg;
    char buff[1000];

    while(1){
        memset(buff, 0, sizeof(buff));
      
        if (recv(new_sock, buff, 1000, 0) <= 0){
            close(new_sock);
            continue;
        }
        // Else, write to the log file
        else{
            fputs(buff, files.IDlog);
            fputs("\n", files.IDlog);

            strtok(buff, " ,");
            char* sock_hostname = strtok(NULL, " ,");
            

            int i;
            // lowercase the request
            for (i = 0; sock_hostname[i]; i ++)
                sock_hostname[i] = tolower((unsigned char) sock_hostname[i]);

            char* hostname_revised;
            if (strncmp(sock_hostname, "www.", 4) == 0)
                hostname_revised = &sock_hostname[0] + 4;
            else
                hostname_revised = &sock_hostname[0];
            

            // Check the request honestname with the stored array of hostnames
            int found = 0;
            for (i = 0; i < datArr_column_size; i++){
                char temp[100];
                strcpy(temp, datArr[i]);
                strtok(temp, " ");
                // match found
                if (strcmp(hostname_revised, temp) == 0){
                    char* ip = strtok(NULL, " \n\0");  
                    char msg[100];
                    sprintf(msg, "0x00, %s, %s", ID, ip);
                    fputs(msg, files.IDlog);
                    fputs("\n", files.IDlog);
                    send(new_sock, msg, strlen(msg), 0); 
                    found = 1;
                    break;
                }
            }
            
            // not found
            if (!found){
                char not_found[100];
                sprintf(not_found, "0xFF, %s, \"Host not found\"", ID);
                fputs(not_found, files.IDlog);
                fputs("\n", files.IDlog);
                send(new_sock, not_found, strlen(not_found), 0);
            }
        }

        fputs("\n", files.IDlog);
        break;
    }
}



static char* fetch_recursive(char* hostname, int port){


}


static char* fetch_iterative(char* request, int port){
    int new_sock;
    struct timeval tv;
    tv.tv_sec = 30; // 30 sec timeout
    static char buff[1000];
    memset(buff, 0, sizeof(buff));
    struct sockaddr_in addr_to_root;
    struct sockaddr_in addr_to_server;
    int opt = 1;

	// create socket fd
	new_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1){
        perror("socket error");
        return NULL;
    }	

	// socket option
	setsockopt(new_sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT | SO_RCVTIMEO, &opt, sizeof(opt)); 

	// conenct to the root  DNS
	addr_to_root.sin_family = AF_INET;
	addr_to_root.sin_addr.s_addr = INADDR_ANY;
	addr_to_root.sin_port = htons(port);

	if (connect(new_sock, (struct sockaddr*)&addr_to_root, sizeof(addr_to_root)) == -1){
        perror("connection error");
        shutdown(new_sock, SHUT_RDWR);
        close (new_sock);
        return NULL;
    }
    
    // send the request to the server 
    if (send(new_sock, request, strlen(request), 0) >= 0){ 
        if (recv(new_sock, buff, 1000, 0) > 0){
            close (new_sock);
            return buff;
        }
        // Timeout or error
        else{
            perror("recv error"); 
            shutdown(new_sock, SHUT_RDWR);
            close (new_sock);
            return NULL;
        }
    }
    else{
        perror("send msg error");
        shutdown(new_sock, SHUT_RDWR);
        close (new_sock);
        return NULL;
    }
    
}



// Close a server program by ctrl-c interrupt handler.
// Closing a server program requires a smooth exit.
// Close all sockets and files properly.
void INThandler(int sig){
    fclose(files.IDlog);
    if (files.mapping != NULL)
        fclose(files.mapping);
    exit(sig);
}











