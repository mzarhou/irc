#include "server.hpp"
#include "context.hpp"

#include "signal.h"

Context context;

void handler(int sig)
{
    (void)sig;
    std::vector<pollfd> &pf = Server::getPfds();
    std::cout << "heyy: " << pf.size() << std::endl;
    std::vector<pollfd>::iterator it = pf.begin();
    it++;

    std::string message("QUIT");
    for (; it != pf.end(); it++)
    {

        User *user = context.getSocketHandler(it->fd);
        if (!user)
        {
            close(it->fd);
            pf.erase(it);
        }
        else
        {
            user->handleSocket(Command::fromMessage(message));
        }
    }

    try
    {
        std::cout << pf.begin()->fd << std::endl;
        close(pf.begin()->fd);
        pf.erase(pf.begin());
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }

    exit(0);
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

    signal(SIGINT, handler);

    try
    {
        Server server(&context, password, port);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}
