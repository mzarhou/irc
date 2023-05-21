#include "server.hpp"

std::string Server::hostname = "";
std::string Server::password = "";

Server::Server(Context *context, const std::string passw, const std::string &port)
    : context(context)
{
    Server::password = passw;

    char hostbuffer[256];
    if (gethostname(hostbuffer, sizeof(hostbuffer)) == -1)
    {
        perror("gethostname");
        exit(1);
    }

    Server::hostname = hostbuffer;

    this->listener_sock = get_listener_socket(port);
    if (listener_sock < 0)
    {
        std::cerr << "failed getting listener socket" << std::endl;
        exit(1);
    }
    add_to_pfds(listener_sock);
}

/**
 * Get sockaddr, IPv4 or IPv6
 */
void *Server::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int Server::get_listener_socket(const std::string &port)
{
    int status, listener_sock, yes;
    struct addrinfo hints;
    struct addrinfo *servinfo, *p;

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    // hints.ai_family = AF_UNSPEC;     // don't care IPv4 or IPv6
    hints.ai_family = AF_INET;       // handle IPV4 only
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
void Server::add_to_pfds(int newfd)
{
    struct pollfd pfd;
    pfd.fd = newfd;
    pfd.events = POLLIN;

    pfds.push_back(pfd);
}

// Remove an index from the set
void Server::del_from_pfds(int fdToDelete)
{
    std::vector<struct pollfd>::iterator it = pfds.begin();
    for (; it != pfds.end(); it++)
    {
        if ((*it).fd == fdToDelete)
            break;
    }
    if (it == pfds.end())
        return;
    pfds.erase(it);
}

std::string Server::getHostname()
{
    return Server::hostname;
}

std::string Server::getPassword()
{
    return Server::password;
}

void Server::onNewConnection()
{
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof addr;

    int newClientFd = accept(listener_sock, 0, 0);
    if (newClientFd < 0)
    {
        perror("accept");
    }
    else
    {
        add_to_pfds(newClientFd);

        if (getsockname(newClientFd, (struct sockaddr *)&addr, &addr_size) == -1)
        {
            perror("getsockname");
            return;
        }

        char *clientIP = inet_ntoa(addr.sin_addr);
        std::cout << "new connection from " << clientIP << " on socket " << newClientFd << std::endl;
        context->addNewUser(newClientFd, clientIP);

        // send a welcome message to the client
        std::string welcomeMsg = "Welcome to the server!\n";
        if (send(newClientFd, welcomeMsg.c_str(), welcomeMsg.length(), 0) == -1)
        {
            perror("send");
        }
    }
}

void Server::onNewMessage(int clientFd)
{
    const int buffer_len = 1024;
    char buffer[buffer_len];

    // regular client
    int nb_bytes = recv(clientFd, buffer, buffer_len, 0);
    User *user = context->getSocketHandler(clientFd);
    if (!user)
    {
        std::cerr << "failed to find user" << std::endl;
    }
    else if (nb_bytes <= 0)
    {
        // error or connection closed by the client
        if (nb_bytes == 0)
        {
            std::cout << "connection closed, socket " << clientFd << std::endl;
        }
        else
        {
            perror("recv");
        }
        std::string message("QUIT");
        user->handleSocket(Command::fromMessage(message));
        del_from_pfds(clientFd);
    }
    else
    {
        buffer[nb_bytes] = 0;
        std::string message(buffer);

        size_t pos = message.find_last_of("\n");
        if (pos != std::string::npos)
        {
            user->buffer += message.substr(0, pos);
            std::queue<std::string> q = splitChunks(user->buffer, '\n');
            user->buffer = message.substr(pos + 1);

            while (!q.empty())
            {
                message = q.front();
                q.pop();
                Command cmd = Command::fromMessage(message);
                user->handleSocket(cmd);
            }
        }
        else
        {
            user->buffer += message;
        }
    }
}

void Server::run()
{
    for (;;)
    {
        int p_count = poll(pfds.data(), pfds.size(), -1); // -1 timeout -> wait forever
        if (p_count < 0)
        {
            perror("poll");
            exit(1);
        }

        for (size_t i = 0; i < pfds.size(); i++)
        {
            if (!(pfds[i].revents & POLLIN))
            {
                continue;
            }
            if (pfds[i].fd == listener_sock)
                this->onNewConnection();
            else
                this->onNewMessage(pfds[i].fd);
        }
    }
}
