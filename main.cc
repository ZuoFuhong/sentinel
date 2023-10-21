#include <iostream>
#include "clipp.h"
#include "server.h"

int main(int argc, char *argv[]) {
    std::string address = "127.0.0.1:8080";
    std::string allow_cmd_dir = "/home";
    auto cli = (
        clipp::option("-addr").doc("Address") & clipp::value("address", address),
        clipp::option("-allow_cmd_dir").doc("Allow") & clipp::value("allow_cmd_dir", allow_cmd_dir)
    );
    parse(argc, argv, cli);

    auto server = Server::new_server(address);
    server->set_allow_cmd_dir(allow_cmd_dir);
    server->run();
    return 0;
}
