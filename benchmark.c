#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define UDS_SOCKET "socket"



char checksum(char *ptr, size_t sz) // https://stackoverflow.com/questions/3463976/c-file-checksum
{
    unsigned char chk = 0;
    while (sz-- != 0)
        chk -= *ptr++;
    return chk;
}

int generate_data(char *path,int kb) // generate a file with random data
{
    FILE *data = fopen(path, "w");
    char randomchar;
    for (size_t i = 1; i <= (kb*1000); i++)
    {
        if ((i!=0) && (i%80 == 0))
        {
            fputc('\n', data);
        }
        else
        {
            randomchar = 'A' + (random() % 26);
            fputc(randomchar, data);
        }
    }
    if (fclose(data) == -1)
    {
        perror("Error");
        exit(1);
    }
    return 0;
}

int TCP_IPv4(const char *path)
{
    int fd = open(path, O_RDONLY);
    int sockfd, sockfd2, connfd, bytes_send, bytes_recv;
    char recvbuf[BUFSIZ] = {'\0'}, sendbuf[BUFSIZ] = {'\0'};
    struct sockaddr_in servaddr, cliaddr, cli;
    struct timeval tv, tv2; // for time measurment
    
    if (!fork()) // child process
    {
        if ((sockfd2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            perror("Error");
            exit(1);
        }
        cliaddr.sin_family = AF_INET;
        cliaddr.sin_port = htons(8080);
        cliaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

        usleep(10000); // wait 0.01 second for sender to start up
        if (connect(sockfd2, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) == -1)
        {
            perror("Error");
            exit(1);
        }
        
        while(1) // child process receives data
        {
            bytes_recv = recv(sockfd2, recvbuf, BUFSIZ, 0); // receive data  
            if (bytes_recv== -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_recv == 0)
            {
                break;
            }
            char c = checksum(recvbuf, BUFSIZ);
            // printf("%d\n", c);
            if (c != 0) // validate data received
            {
                printf("Error: checksum is not 0\n");
                printf("%s\n", recvbuf);
                exit(1);
            }
            bzero(recvbuf, BUFSIZ);
        }
        close(sockfd2);
        
        gettimeofday(&tv2,NULL);
        printf("TCP / IPv4 Socket - end:\t%ld\n", tv2.tv_usec); // end time measure
        
        exit(0);
    }
    else
    {
        if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        {
            perror("Error");
            exit(1);
        }
        
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(8080);
        servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
        if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) == -1)
        {
            perror("Error");
            exit(1);
        }
        if ((listen(sockfd, 1)) == -1)
        {
            perror("Error");
            exit(1);
        }

        int len = sizeof(cli);
        connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        if (connfd == -1)
        {
            perror("Error");
            exit(1);
        }
        
        gettimeofday(&tv,NULL);
        printf("TCP / IPv4 Socket - start:\t%ld\n", tv.tv_usec); // start time measure

        while (1) // parent process sends data
        {
            bytes_send = read(fd, sendbuf, BUFSIZ-1);   
            if (bytes_send == -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_send == 0)
            {
                break;
            }

            sendbuf[BUFSIZ-1] = checksum(sendbuf, BUFSIZ-1);
            if (send(connfd, sendbuf, BUFSIZ, 0) == -1) // send data
            {
                perror("Error");
                exit(1);
            }
            bzero(sendbuf, BUFSIZ);
        }
        close(connfd);
        close(sockfd);
        close(fd);
        wait(NULL);
    }
    return 0;
}

int UDS(const char *path)
{
    int fd = open(path, O_RDONLY);
    int sockfd, sockfd2, connfd, bytes_send, bytes_recv;
    char recvbuf[BUFSIZ] = {'\0'}, sendbuf[BUFSIZ] = {'\0'};
    struct sockaddr_un servaddr, cliaddr, cli;
    struct timeval tv, tv2; // for time measurment
    
    if (!fork()) // child process
    {
        if ((sockfd2 = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        {
            perror("Error");
            exit(1);
        }
        cliaddr.sun_family = AF_UNIX;
        strcpy(cliaddr.sun_path, UDS_SOCKET);

        usleep(10000); // wait 0.01 second for sender to start up
        if (connect(sockfd2, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) == -1)
        {
            perror("Error");
            exit(1);
        }
        
        while(1) // child process receives data
        {
            bytes_recv = recv(sockfd2, recvbuf, BUFSIZ, 0); // receive data  
            if (bytes_recv== -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_recv == 0)
            {
                break;
            }
            char c = checksum(recvbuf, BUFSIZ);
            // printf("%d\n", c);
            if (c != 0) // validate data received
            {
                printf("Error: checksum is not 0\n");
                printf("%s\n", recvbuf);
                exit(1);
            }
            bzero(recvbuf, BUFSIZ);
        }
        close(sockfd2);
        
        gettimeofday(&tv2,NULL);
        printf("UDS - end:\t%ld\n", tv2.tv_usec); // end time measure
        
        exit(0);
    }
    else
    {
        if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        {
            perror("Error");
            exit(1);
        }
        
        servaddr.sun_family = AF_UNIX;
        strcpy(servaddr.sun_path, UDS_SOCKET);
        if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) == -1)
        {
            perror("Error");
            exit(1);
        }
        if ((listen(sockfd, 1)) == -1)
        {
            perror("Error");
            exit(1);
        }

        int len = sizeof(cli);
        connfd = accept(sockfd, (struct sockaddr*)&cli, (socklen_t*)&len);
        if (connfd == -1)
        {
            perror("Error");
            exit(1);
        }
        
        gettimeofday(&tv,NULL);
        printf("UDS - start:\t%ld\n", tv.tv_usec); // start time measure

        while (1) // parent process sends data
        {
            bytes_send = read(fd, sendbuf, BUFSIZ-1);   
            if (bytes_send == -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_send == 0)
            {
                break;
            }

            sendbuf[BUFSIZ-1] = checksum(sendbuf, BUFSIZ-1);
            if (send(connfd, sendbuf, BUFSIZ, 0) == -1) // send data
            {
                perror("Error");
                exit(1);
            }
            bzero(sendbuf, BUFSIZ);
        }
        close(connfd);
        close(sockfd);
        close(fd);
        wait(NULL);
    }
    return 0;
}

int main()
{
    generate_data("data.txt", 100000);
    TCP_IPv4("data.txt");
    UDS("data.txt");
    printf("SUCCESS!\n");

    return 0;
}
