//
// Created by eylon on 4/19/22.
//
/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#define PORT "3495"  // the port users will be connecting to

#define BACKLOG 10   // how many pending connections queue will hold

#define SIZE 10000


int new_fd = 0;
struct flock lock;

int mem_fd = shm_open("stack_memory", O_CREAT | O_RDWR, 0);

char* stack_beginning = (char*)(mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, mem_fd, 0));
//char* stack_head = stack_beginning;
int* place = (int*)(mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
char* stack_end = stack_beginning + SIZE;

bool push(char* data){
   lock.l_type = F_WRLCK;
   fcntl(mem_fd, F_SETLKW, &lock);
    int size_data = strlen(data);
    if(stack_beginning + (*place) + size_data + 1 > stack_end){
        return 0;
    }
    int i = 0;
    for(i = 1; i < size_data +1; ++i){
        if(stack_beginning + (*place) + i+1 == stack_end){
            return 0;
        }
        stack_beginning[i+(*place)] = data[size_data-i];
    }
    if(stack_beginning + (*place) + i == stack_end){
        return 0;
    }
    stack_beginning[i+(*place)] = '\0';
    (*place) += i;
   lock.l_type = F_UNLCK;
   fcntl (mem_fd, F_SETLKW, &lock);
    return 1;
}

bool pop(){
   lock.l_type = F_WRLCK;
   fcntl (mem_fd, F_SETLKW, &lock);
    if(stack_beginning == stack_beginning + (*place)){
        return 0;
    }
    (*place)--;
    while(stack_beginning[*place] != '\0'){
        (*place)--;
    }
   lock.l_type = F_UNLCK;
   fcntl (mem_fd, F_SETLKW, &lock);
    return 1;
}

void top(int* new_fd){
   lock.l_type = F_WRLCK;
   fcntl (mem_fd, F_SETLKW, &lock);
    if(stack_beginning == stack_beginning + (*place)){
        char answer [1024] = "ERROR: stack is empty";
        send(*new_fd, answer, 1024, 0);
    }else{
        char answer [1024] = "OUTPUT: ";
        char* tmp = stack_beginning + (*place);
        tmp--;
        int i = strlen(answer);
        while(*tmp != '\0'){
            answer[i] = *tmp;
            i++;
            tmp--;
        }
        answer[i] = '\0';
        send(*new_fd, answer, 1024, 0);
        lock.l_type = F_UNLCK;
        fcntl (mem_fd, F_SETLKW, &lock);
    }
}

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void sigint_handler(int num){
    close(new_fd);
    if(munmap(stack_beginning, SIZE) < 0){
        printf("error\n");
    }
    if(munmap(place, SIZE) < 0){
        printf("error\n");
    }
    close(mem_fd);
    shm_unlink("stack_memory");
    exit(0);
}

int main(void)
{
    int sockfd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

   memset(&lock, 0, sizeof(lock));

    if(mem_fd < 0){
        printf("memory alloc failed\n");
        exit(1);
    }

    *place = 0;
    stack_beginning[*place] = '\0';

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");


    signal(SIGINT, sigint_handler);

    int check = 0;
    while(1) {  // main accept() loop
        printf("%d\n", check);
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
                  get_in_addr((struct sockaddr *)&their_addr),
                  s, sizeof s);
        printf("server: got connection from %s\n", s);

        if(fork() == 0){
            check++;
            char buf[2048];

            while(recv(new_fd, buf, 2048, 0) != -1){
                size_t ln = strlen(buf)-1;
                if (buf[ln] == '\n') {
                    buf[ln] = '\0';
                }
                char command[5];
                int j;
                for(j = 0; buf[j] != ' ' && buf[j] != '\0'; ++j){
                    command[j] = buf[j];
                }
                command[j] = '\0';
                if(!(strcmp(command, "PUSH"))) {
                    char copy[2048];
                    int k;
                    j = j+1;
                    for(k = 0; buf[j] != '\0'; ++k, ++j){
                        copy[k] = buf[j];
                    }
                    copy[k] = '\0';
                    push(copy);
                }else if(!(strcmp(command, "TOP"))) {
                    top(&new_fd);
                }else if(!(strcmp(command, "POP"))){
                    if(pop())
                    {
                        send(new_fd, "OUTPUT: popping", 2048, 0);
                    }
                    else{
                        send(new_fd, "ERROR: stack is empty", 2048, 0);
                    }
                }else if(!(strcmp(command, "EXIT"))){
                    break;
                }else{
                    printf("ERROR: illegal command\n");
                    break;
                }
            }
            close(new_fd);
        }
    }
    return 0;
}