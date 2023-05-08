#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h> // non blocking sockets
#include <poll.h>
#include "context.hpp"
#include "str-utils.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int get_listener_socket(const std::string &port)
{
    int status, listener_sock, yes;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me

    if ((status = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        exit(1);
    }

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        listener_sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
        if (listener_sock < 0)
        {
            continue;
        }

        // Lose the pesky "address already in use" error message
        setsockopt(listener_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener_sock, servinfo->ai_addr, servinfo->ai_addrlen) < 0)
        {
            close(listener_sock);
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // free the linked-list

    if (p == NULL)
    {
        return -1;
    }

    if (listen(listener_sock, BACKLOG) < 0)
    {
        return -1;
    }

    return listener_sock;
}

// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size)
{
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size)
    {
        *fd_size *= 2; // Double it

        *pfds = (struct pollfd *)realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; // Check ready-to-read

    (*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count - 1];

    (*fd_count)--;
}

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "invalid args\n";
        return 1;
    }
    std::string port = argv[1];
    std::string password = argv[2];

    Context context(password);

    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    char clientIP[INET6_ADDRSTRLEN];
    const int buffer_len = 1024;
    char buffer[buffer_len];

    int listener_sock = get_listener_socket(port);
    if (listener_sock < 0)
    {
        std::cerr << "failed getting listener socket" << std::endl;
        exit(1);
    }

    int fd_count = 0;
    int fd_size = 5;
    struct pollfd *pfds = (struct pollfd *)malloc(sizeof(pollfd *) * fd_size);

    // add listener socket
    pfds[0].fd = listener_sock;
    pfds[0].events = POLLIN;
    fd_count = 1;

    while (true)
    {
        int p_count = poll(pfds, fd_count, -1); // -1 timeout -> wait forever
        if (p_count < 0)
        {
            perror("poll");
            exit(1);
        }

        for (int i = 0; i < fd_count; i++)
        {
            if (!(pfds[i].revents & POLLIN))
            {
                continue;
            }
            if (pfds[i].fd == listener_sock)
            {
                // new connection
                addr_size = sizeof clientIP;
                int newClientFd = accept(listener_sock, (struct sockaddr *)&client_addr, &addr_size);
                if (newClientFd < 0)
                {
                    perror("accept");
                }
                else
                {
                    add_to_pfds(&pfds, newClientFd, &fd_count, &fd_size);
                    inet_ntop(
                        client_addr.ss_family,
                        get_in_addr((struct sockaddr *)&client_addr),
                        clientIP,
                        INET6_ADDRSTRLEN);
                    std::cout << "new connection from " << clientIP << " on socket " << newClientFd << std::endl;
                    context.addNewUser(newClientFd);
                }
            }
            else
            {
                // regular client
                int client_fd = pfds[i].fd;
                int nb_bytes = recv(pfds[i].fd, buffer, buffer_len, 0);
                if (nb_bytes <= 0)
                {
                    // error or connection closed by the client
                    if (nb_bytes == 0)
                    {
                        std::cout << "connection closed, socket " << client_fd << std::endl;
                    }
                    else
                    {
                        perror("recv");
                    }
                    context.onUserDeconnected(client_fd);
                    close(client_fd);
                    del_from_pfds(pfds, i, &fd_count);
                }
                else
                {
                    buffer[nb_bytes] = 0;

                    std::istringstream ss(buffer);
                    std::string message;
                    while (std::getline(ss, message, '\n'))
                    {
                        Command cmd = User::parseIntoCmd(message);
                        User *user = context.getSocketHandler(client_fd);
                        user->handleSocket(cmd);
                    }

                    // PASS 123
                    // NICK mzarhou_nickname
                    // USER mzarhou_login 0 * mzarhou_realname
                }
            }
        }
    }
}
