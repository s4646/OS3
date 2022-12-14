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
#include <sys/mman.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define UDS_SOCKET "socket"
#define CLISOCKET "clisocket"
#define SERVSOCKET "servsocket"
#define BUF_SIZE 128



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
    char recvbuf[BUF_SIZE] = {'\0'}, sendbuf[BUF_SIZE] = {'\0'};
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
            bytes_recv = recv(sockfd2, recvbuf, BUF_SIZE, 0); // receive data  
            if (bytes_recv== -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_recv == 0)
            {
                break;
            }
            char c = checksum(recvbuf, BUF_SIZE);
            // printf("%d\n", c);
            if (c != 0) // validate data received
            {
                printf("Error: checksum is not 0\n");
                printf("%s\n", recvbuf);
                exit(1);
            }
            bzero(recvbuf, BUF_SIZE);
        }
        close(sockfd2);
        
        gettimeofday(&tv2,NULL);
        printf("TCP / IPv4 - end:\t%ld.%ld\n", tv2.tv_sec, tv2.tv_usec); // end time measure
        
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
        printf("TCP / IPv4 - start:\t%ld.%ld\n", tv.tv_sec, tv.tv_usec); // start time measure

        while (1) // parent process sends data
        {
            bytes_send = read(fd, sendbuf, BUF_SIZE-1);   
            if (bytes_send == -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_send == 0)
            {
                break;
            }

            sendbuf[BUF_SIZE-1] = checksum(sendbuf, BUF_SIZE-1);
            if (send(connfd, sendbuf, BUF_SIZE, 0) == -1) // send data
            {
                perror("Error");
                exit(1);
            }
            bzero(sendbuf, BUF_SIZE);
        }
        close(connfd);
        close(sockfd);
        close(fd);
        wait(NULL);
    }
    return 0;
}

int UDS_TCP(const char *path)
{
    int fd = open(path, O_RDONLY);
    int sockfd, sockfd2, connfd, bytes_send, bytes_recv;
    char recvbuf[BUF_SIZE] = {'\0'}, sendbuf[BUF_SIZE] = {'\0'};
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
            bytes_recv = recv(sockfd2, recvbuf, BUF_SIZE, 0); // receive data  
            if (bytes_recv== -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_recv == 0)
            {
                break;
            }
            char c = checksum(recvbuf, BUF_SIZE);
            // printf("%d\n", c);
            if (c != 0) // validate data received
            {
                printf("Error: checksum is not 0\n");
                printf("%s\n", recvbuf);
                exit(1);
            }
            bzero(recvbuf, BUF_SIZE);
        }
        close(sockfd2);
        
        gettimeofday(&tv2,NULL);
        printf("UDS / STREAM - end:\t%ld.%ld\n", tv2.tv_sec, tv2.tv_usec); // end time measure
        
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
        printf("UDS / STREAM - start:\t%ld.%ld\n", tv.tv_sec, tv.tv_usec); // start time measure

        while (1) // parent process sends data
        {
            bytes_send = read(fd, sendbuf, BUF_SIZE-1);   
            if (bytes_send == -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_send == 0)
            {
                break;
            }

            sendbuf[BUF_SIZE-1] = checksum(sendbuf, BUF_SIZE-1);
            if (send(connfd, sendbuf, BUF_SIZE, 0) == -1) // send data
            {
                perror("Error");
                exit(1);
            }
            bzero(sendbuf, BUF_SIZE);
        }
        close(connfd);
        close(sockfd);
        close(fd);
        wait(NULL);
    }
    return 0;
}

int UDP_IPv6(const char *path)
{
    int fd = open(path, O_RDONLY);
    int sockfd, sockfd2, bytes_send, bytes_recv;
    char recvbuf[BUF_SIZE] = {'\0'}, sendbuf[BUF_SIZE] = {'\0'};
    struct sockaddr_in6 servaddr, cliaddr, serv;
    struct timeval tv, tv2; // for time measurment
    
    if (!fork()) // child process
    {
        if ((sockfd2 = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            perror("Error");
            exit(1);
        }
        serv.sin6_family = AF_INET6;
        serv.sin6_port = htons(8081);
        inet_pton(AF_INET6, "::1", &(serv.sin6_addr));

        if (sendto(sockfd2, "init", 5, 0, (struct sockaddr*)&serv, sizeof(serv)) == -1)
        {
            perror("Error");
            exit(1);
        }
        
        socklen_t len = sizeof(serv);
        while(1) // child process receives data
        {
            bytes_recv = recvfrom(sockfd2, recvbuf, BUF_SIZE, 0, (struct sockaddr*)&serv, &len); // receive data  
            if (bytes_recv== -1)
            {
                perror("Error");
                exit(1);
            }
            else if (strcmp(recvbuf, "end") == 0)
            {
                break;
            }
            char c = checksum(recvbuf, BUF_SIZE);
            // printf("%d\n", c);
            if (c != 0) // validate data received
            {
                printf("Error: checksum is not 0\n");
                printf("%s\n", recvbuf);
                exit(1);
            }
            bzero(recvbuf, BUF_SIZE);
        }
        close(sockfd2);
        
        gettimeofday(&tv2,NULL);
        printf("UDP / IPv6 - end:\t%ld.%ld\n", tv2.tv_sec, tv2.tv_usec); // end time measure
        
        exit(0);
    }
    else
    {
        if ((sockfd = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1)
        {
            perror("Error");
            exit(1);
        }
        memset(&servaddr, 0, sizeof(servaddr));
        memset(&cliaddr, 0, sizeof(cliaddr));
        
        servaddr.sin6_family = AF_INET6;
        servaddr.sin6_port = htons(8081);
        servaddr.sin6_addr = in6addr_any;
        if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) == -1)
        {
            perror("Error");
            exit(1);
        }
        socklen_t len = sizeof(cliaddr);
        if (recvfrom(sockfd, recvbuf, 5, MSG_WAITALL, ( struct sockaddr *) &cliaddr, &len) == -1)
        {
            perror("Error");
            exit(1);
        }

        usleep(100000); // wait 0.1 second for receiver to start up
        bzero(recvbuf, BUF_SIZE);
        gettimeofday(&tv,NULL);
        printf("UDP / IPv6 - start:\t%ld.%ld\n", tv.tv_sec, tv.tv_usec); // start time measure

        while (1) // parent process sends data
        {
            bytes_send = read(fd, sendbuf, BUF_SIZE-1);
            if (bytes_send == -1)
            {
                perror("Error");
                exit(1);
            }
            else if (bytes_send == 0)
            {
                break;
            }

            sendbuf[BUF_SIZE-1] = checksum(sendbuf, BUF_SIZE-1);

            if (sendto(sockfd, sendbuf, BUF_SIZE, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) == -1) // send data
            {
                perror("Error");
                exit(1);
            }
            bzero(sendbuf, BUF_SIZE);
        }
        bzero(sendbuf, BUF_SIZE);
        if (sendto(sockfd, "end", 4, 0, (struct sockaddr*)&cliaddr, sizeof(cliaddr)) == -1) // send data
        {
            perror("Error");
            exit(1);
        }
        wait(NULL);
        close(sockfd);
        close(fd);
    }
    return 0;
}

int UDS_UDP(const char *path) // https://stackoverflow.com/questions/3324619/unix-domain-socket-using-datagram-communication-between-one-server-process-and
{
    int fd = open(path, O_RDONLY);
    int servsock, clisock, bytes_send, bytes_recv;
    char sendbuf[BUF_SIZE] = {'\0'}, recvbuf[BUF_SIZE] = {'\0'};
    struct sockaddr_un server_addr, client_addr;
    struct timeval tv, tv2; // for time measurment

    if (!fork())
    {
        if ((clisock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
        {
            perror("Error: socket");
            exit(1);
        }

        memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sun_family = AF_UNIX;
        strcpy(client_addr.sun_path, CLISOCKET);
        if (bind(clisock, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1)
        {
            perror("Error: bind");
            exit(1);
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, SERVSOCKET);
        if (connect(clisock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            perror("Error: connect");
            exit(1);
        }

        gettimeofday(&tv2,NULL);
        printf("UDS / DGRAM - start:\t%ld.%ld\n", tv2.tv_sec, tv2.tv_usec); // start time measure
        while(1)
        {
            bytes_send = read(fd, sendbuf, BUF_SIZE);
            if (bytes_send == -1)
            {
                perror("Error: read");
                exit(1);
            }
            if (bytes_send == 0)
            {
                break;
            }

            sendbuf[BUF_SIZE-1] = checksum(sendbuf, BUF_SIZE-1);

            if (send(clisock, sendbuf, BUF_SIZE, 0) == -1)
            {
                perror("Error: send");
                exit(1);
            }
            bzero(sendbuf, BUF_SIZE);
        }
        if (send(clisock, "end", 4, 0) == -1)
        {
            perror("Error: send");
            exit(1);
        }
        close(clisock);
        close(fd);
        exit(0);
    }

    else
    {
        if ((servsock = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1)
        {
		    perror("Error: socket");
            exit(1);
	    }

        memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sun_family = AF_UNIX;
        strcpy(client_addr.sun_path, CLISOCKET);
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strcpy(server_addr.sun_path, SERVSOCKET);
        if (bind(servsock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        {
            perror("Error: bind");
            exit(1);
        }

        socklen_t len = sizeof(client_addr);
        while(1)
        {
            bytes_recv = recvfrom(servsock, recvbuf, BUF_SIZE, 0, (struct sockaddr*)&client_addr, &len);
            if (bytes_recv == -1)
            {
                perror("Error: recvfrom");
                exit(1);
            }
            if (strcmp(recvbuf, "end") == 0)
            {
                break;
            }
            if (checksum(recvbuf, BUF_SIZE) !=0)
            {
                printf("Error: checksum is not 0\n");
                exit(1);
            }
            bzero(recvbuf, BUF_SIZE);
        }

        gettimeofday(&tv, NULL);
        printf("UDS / DGRAM - end:\t%ld.%ld\n", tv.tv_sec, tv.tv_usec); // end time measure
        wait(NULL);
        close(servsock);
    }
    return 0;
}

int main()
{
    generate_data("data.txt", 100000);
    TCP_IPv4("data.txt");
    UDS_TCP("data.txt");
    UDP_IPv6("data.txt");
    UDS_UDP("data.txt");
    printf("SUCCESS!\n");

    return 0;
}
