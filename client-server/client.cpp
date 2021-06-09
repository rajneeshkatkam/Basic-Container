// Client side C/C++ program to demonstrate Socket programming 
#include <stdio.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string>
#include <string.h>
#include <time.h>
#include<stdlib.h>
#include<pthread.h>
#include <signal.h>
#define PORT 9999

bool thread_flag=true;


std::string server_ip;
// Signal Handler for SIGINT
void sigint_handler(int sig)
{
    thread_flag=false;
    //printf("SIGINT executed \n");
}


void *client_thread_procedure(void *id)
{
    usleep(2000);
    int sock = 0, valread; 
    struct sockaddr_in serv_addr; 
    std::string hello = "Message from client"; 
    char buffer[1024] = {0}; 
    sock = socket(AF_INET, SOCK_STREAM, 0);
   
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 
       
    // Convert IPv4 and IPv6 addresses from text to binary form 
    inet_pton(AF_INET, server_ip.c_str(), &serv_addr.sin_addr);
    connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    long long count=0;

    while(thread_flag)
    {
        send(sock , hello.c_str() , hello.length() , 0 ); 
        //printf("Hello message sent\n"); 
        valread = read( sock , buffer, 1024); 
        if(valread == 0)
                break;
        printf("%s id: %lld %lld \n",buffer, (long long int) id,count++ ); 
        usleep(100);
    }

    pthread_exit((void *)0);
}
   
int main(int argc, char const *argv[]) 
{ 
    signal(SIGINT, sigint_handler);

    long long int NUMBER_OF_THREADS;
	int thread_create_status;
	void* status;

	if(argc >2)
	{
		NUMBER_OF_THREADS=atoi(argv[1]);
		if(NUMBER_OF_THREADS==0)
			return 0;
        server_ip=argv[2];
	}
	else
	{
		printf("./client <number of threads> <server IP>\n");
			return 0;
	}

	pthread_t threads[NUMBER_OF_THREADS];

	for(long long int i=0;i<NUMBER_OF_THREADS;i++)
	{
		thread_create_status=pthread_create(&threads[i],NULL, client_thread_procedure,(void *) i);
		if(thread_create_status){
			printf("Error in creating thread: %lld \n",i);
			exit(-1);
		}
	}

    printf("No. of threads spwaned: %lld\n", NUMBER_OF_THREADS);

	for(long long int i=0;i<NUMBER_OF_THREADS;i++)
	{
		pthread_join(threads[i], &status);
        printf("Thread reapead: %lld\n", i);
	}

    printf("All threads reaped\n");
    
    return 0; 
} 