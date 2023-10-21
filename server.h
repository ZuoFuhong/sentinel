#pragma once
#include <iostream>

class Server {
public:
    Server(std::string addr);
    ~Server();

    static Server* new_server(std::string addr);

    void set_allow_cmd_dir(std::string allow_cmd_dir);
 
    void run();

private:
    std::string addr;

    std::vector<std::string> allow_cmd_dirs;

    void serve(int socket_fd); 
};

