#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>

#define BACKLOG 10 // how many pending connections queue will hold

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "invalid args\n";
        return 1;
    }
    std::string port = argv[1];
    std::string password = argv[2];

    // std::cout << "port: " << port << std::endl;
    // std::cout << "password: " << password << std::endl;

    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;

    // char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    int sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
    {
        std::cerr << "failed to associate socket with port: " << port << std::endl;
        std::cerr << strerror(errno) << std::endl;
        exit(1);
    }

    listen(sockfd, BACKLOG);

    addr_size = sizeof client_addr;
    int clientfd = accept(sockfd, (struct sockaddr *)&client_addr, &addr_size);
    if (clientfd < 0)
    {
        std::cerr << "failed to accept the connection" << std::endl;
        std::cerr << strerror(errno) << std::endl;
        exit(1);
    }

    const int buffer_len = 1024;
    char buffer[buffer_len];

    int recv_res;
    while ((recv_res = recv(clientfd, buffer, buffer_len, 0)) > 0)
    {
        std::cout << buffer << std::endl;

        // if (recv_res < 0)
        // {
        //     std::cout << "failed to receive date from client" << std::endl;
        //     std::cerr << strerror(errno) << std::endl;
        //     exit(1);
        // }
        // else if (recv_res == 0)
        // {
        //     std::cout << "connection closed by the client" << std::endl;
        //     exit(0);
        // }
        // else
        // {
        //     std::cout << "message received" << std::endl;
        // }
    }
    freeaddrinfo(servinfo); // free the linked-list

    // close sockets
    close(sockfd);
    close(clientfd);
}
