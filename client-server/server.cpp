// Server side C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h>
#include <string> 
#include <time.h>
#include<pthread.h>
#include<cmath>
#define PORT 9999 


void *server_thread_procedure(void *socket_fd)
{
    long long new_socket=(long long) socket_fd;
    int valread;
    char buffer[1024] = {0}; 
    std::string hello = "Message from server";
    long long count=0;
        while(1)
        {
            valread = read( new_socket , buffer, 1024);
            if(valread == 0)
                break;
            
            long long sum=1;
            for(long long i=1; i<10000; i++)
            {
                sum*=pow(i,3);
            }

            //usleep(1000);
            

            printf("%s %lld \n",buffer, count++); 
            //usleep(100);
            send(new_socket , hello.c_str() , hello.length() , 0 );
        }

        printf("Thread Exited\n");
        pthread_exit((void *)0);

}

int main(int argc, char const *argv[]) 
{ 
    int server_fd;
    long long new_socket; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
     
       
    // Creating socket file descriptor 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
 
    // Forcefully attaching socket to the port 8080 
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT ); 
       
    bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address));
    listen(server_fd, 10);
    printf("Server is listening at port: %d\n", PORT);

    while(1)
    {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        
        pthread_t thread;
		if(pthread_create(&thread,NULL, server_thread_procedure,(void *) new_socket)){
			printf("Error in creating thread \n");
			exit(-1);
		}

    }
    

    return 0; 
} 