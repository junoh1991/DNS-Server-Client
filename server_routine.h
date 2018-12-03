#define LOCAL_PORT 5352
#define ROOT_PORT 5353

// global variables for each server bind instance.
// Thset variables keep track of bind socket, port, addrlen
struct sockaddr_in server_addr;
int sockfd;
int port;
int addrlen;
char* ID;

// File struct so each thread can access this descriptors to write to the file
struct workerArg {
    FILE *mapping;
    FILE *IDlog;
    FILE *serverdat;
    FILE *default_mappingdat;
}files;


// root server reads the server.dat and stores in the following structs
struct comDNS{
    char* IP;
    int port;
}comDNS;

struct orgDNS{
    char* IP;
    int port;
}orgDNS;
struct govDNS{
    char* IP;
    int port;
}govDNS;


// each dns server reads their dat file and store them in this array for easier recovery
char* datArr[100];
int datArr_column_size;

char* mappingArr[100];
int mappingArr_column_size;

// This function handles accept() connection between the local dns server and clients.
// This function will spawn thread  when connection is made to the client.
void local_dns_routine(FILE* logfile, FILE* mappingfile);
void root_dns_routine(FILE* logfile);
void server_dns_routine(FILE* logfile);

// Helper function for local_dns_routine. 
static char* fetch_iterative(char* hostname, int port);

// If the request from the local dns server is recursive, this function will make a connection to the 
// dns servers, acquire IP addresses, and return back to the local dns server.
// If the request is recursive, this funtion simply returns dns server IP and port back to the local dns server.
void root_dns_routine();


// This function is initialized and waits for eiter root or local dns server connection depend on the type of request.
// Whether the connection was from root or local dns server, it returns the IP address from the dat file.
void dns_servers_routine();



// This is a function of for individual worker thread function adhere to.
// Each server program creates a threads whenever there is a new connection.
// Each request is processed in this thread function.
void *local_dns_worker(void *arg);
void *root_dns_worker(void *arg);
void *server_dns_worker(void *arg);


// ctrl-c interrupt handler so that everything closes correctly.
void INThandler(int sig);
