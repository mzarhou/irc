#ifndef SERVER_HPP
#define SERVER_HPP

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

struct Command;

#include "command.hpp"

#define BACKLOG 10 // how many pending connections queue will hold

class Server
{
private:
    Context *context;
    static std::vector<pollfd> pfds;
    int listener_sock;

    static std::string hostname;
    static std::string password;

private:
    void *get_in_addr(struct sockaddr *sa);
    int get_listener_socket(const std::string &port);
    void add_to_pfds(int newFd);
    void del_from_pfds(int fdToDelete);

    void onNewConnection();
    void onNewMessage(int clientFd);

public:
    Server(Context *context, const std::string passw, const std::string &port);
    void run();

    static std::string getHostname();
    static std::string getPassword();
    static std::vector<pollfd> &getPfds();
};

#endif
