#include "server.hpp"
#include "context.hpp"

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "invalid args\n";
        return 1;
    }

    std::string port = argv[1];
    std::string password = argv[2];

    try
    {
        Context context;
        Server server(&context, password, port);
        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}
