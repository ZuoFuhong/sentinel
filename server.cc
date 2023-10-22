#include <cstdio>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <vector>
#include <unistd.h>
#include <thread>
#include <sstream>
#include <fstream>
#include <map>
#include "server.h"
#include "util.h"

constexpr int BUFFER_SIZE = 8192;

Server::Server(std::string addr) {
    this->addr = addr; 
}

Server::~Server() {
}

void send_http_response(int socket_fd, const std::string& content);

void bad_request(int socket_fd);

void not_found(int socket_fd);

Server* Server::new_server(std::string addr) {
    return new Server(addr);
}

void Server::set_allow_cmd_dir(std::string allow_cmd_dir) {
    this->allow_cmd_dirs = split(allow_cmd_dir, "|"); 
}
 
void Server::run() {
    int server_fd;
    int opt = 1;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Failed to create socket: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        std::cerr << "Failed to set socket option: " << strerror(errno) << std::endl;
        exit(EXIT_FAILURE);
    }

    sockaddr_in address;
    int addrlen = sizeof(address);
    if (!parse_string_to_sockaddr(this->addr, address)) {
        std::cerr << "Failed to parse socket address." << std::endl; 
        exit(EXIT_FAILURE);
    }
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Error bind on socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 5) < 0) {
        std::cerr << "Error listening on socket." << std::endl;
        exit(EXIT_FAILURE);
    } 
    std::cout << "The service is running on " << this->addr << std::endl; 
    while (true) {
        int socket_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (socket_fd < 0) {
            std::cerr << "Failed to accept incoming connection" << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // Spawn a new thread to handle the connection
        auto t = std::thread(&Server::serve, this, socket_fd);
        t.detach();
    }
}

void Server::serve(int socket_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t received = recv(socket_fd, buffer, BUFFER_SIZE, 0);
    if (received <= 0) {
        std::cerr << "Error receiving data from client.\n";
        bad_request(socket_fd);
        return;
    }

    std::istringstream request_stream(buffer);
    std::string method, uri, version;
    request_stream >> method >> uri >> version;

    if (method != "POST") {
        std::cerr << "Invalid HTTP method. Only POST is supported.\n";
        bad_request(socket_fd);
        return;
    }
    if (uri != "/upload" && uri != "/command") {
        std::cerr << "404 Not Found." << std::endl;
        not_found(socket_fd);
        return;
    }
    // Skip empty line
    std::string line;
    std::getline(request_stream, line);
    // Parse http request headers
    std::string content_type, content_length;
    while (std::getline(request_stream, line) && line != "\r") {
        if (line.find("Content-Type:") != std::string::npos) {
            content_type = line.substr(line.find(":") + 1);
        }
        if (line.find("Content-Length:") != std::string::npos) {
            content_length = line.substr(line.find(":") + 1);
        }
    }
    if (content_type.empty() || content_length.empty()) {
        std::cerr << "Invalid HTTP request. Missing Content-Type or Content-Length.\n";
        bad_request(socket_fd);
        return;
    } 
    // Offset of the request body begin position
    std::streampos offset = request_stream.tellg();

    // Read the request body
    std::vector<char> request_body_buffer;
    if (received > offset) {
        std::string last_content(buffer + offset, received - offset);
        request_body_buffer.insert(request_body_buffer.end(), last_content.begin(), last_content.end());  
    }
    ssize_t request_body_length = std::stoi(content_length) - received;
    while (request_body_length > 0) {
        received = recv(socket_fd, buffer, BUFFER_SIZE, 0);
        if (received <= 0) {
            std::cerr << "Error receiving data from client.\n";
            bad_request(socket_fd);
            return;
        }
        std::vector<char> received_data(buffer, buffer + received);
        request_body_buffer.insert(request_body_buffer.end(), received_data.begin(), received_data.end());
        request_body_length -= received;
    }
    std::string request_body(request_body_buffer.begin(), request_body_buffer.end());

    // Receive uploaded files
    if (uri == "/upload") {
        size_t pos = content_type.find("multipart/form-data");
        if (pos == std::string::npos) {
            std::cerr << "Invalid Content-Type. Only multipart/form-data is supported.\n";
            bad_request(socket_fd);
            return;
        }
        // Extract boundary from Content-Type
        std::string boundary = content_type.substr(content_type.find("boundary=") + 9);

        std::size_t boundary_pos = request_body.find(boundary);
        if (boundary_pos == std::string::npos) {
            std::cerr << "Invalid multipart/form-data request: boundary not found" << std::endl;
            bad_request(socket_fd);
            return;
        }
        boundary = "--" + boundary;
        std::vector<std::string> parts = split(request_body, boundary);
        for (const std::string& part : parts) {
            if (part.empty() || part == "--\r\n" || part == "--") {
                continue;
            }
            std::size_t header_end = part.find("\r\n\r\n");
            if (header_end == std::string::npos) {
                std::cerr << "Invalid part: header not found" << std::endl;
                continue;
            }
            std::string headers_str = part.substr(0, header_end);
            std::map<std::string, std::string> headers = parse_headers(headers_str);
            if (headers.count("Content-Disposition") == 0) {
                std::cerr << "Invalid part: Content-Disposition header not found" << std::endl;
                continue;
            }
            std::string content_disposition = headers["Content-Disposition"];
            pos = content_disposition.find("filename=");
            if (pos == std::string::npos) {
                std::cerr << "Invalid part: filename not found" << std::endl;
                continue;
            }
            std::string filename = content_disposition.substr(pos + 10);
            filename = filename.substr(0, filename.find('"'));
            std::string file_content = part.substr(header_end + 4, part.size() - header_end - 6);

            // Write the file content to a disk file
            std::ofstream outfile(filename, std::ios::binary);
            if (outfile.is_open()) {
                outfile.write(file_content.data(), file_content.size());
                outfile.close();

                std::string filepath = get_current_path() + "/" + filename;
                std::cout << "File saved: " << filepath << std::endl;
            } else {
                std::cerr << "Failed to open file for writing: " << filename << std::endl;
            }
        }
    }
    
    // Receive remote commands
    if (uri == "/command") {
        std::string command = request_body;
        if (command == "") {
            bad_request(socket_fd);
            send_http_response(socket_fd, "The request body cannot be empty.");
            return;
        }
        bool allow = false;
        for (const std::string& allow_cmd_dir : allow_cmd_dirs) {
            if (starts_with(command, allow_cmd_dir)) {
                allow = true;
                break;
            }
        }
        if (allow == false) {
            std::cerr << "The command is not allowed: " << command << std::endl;
            send_http_response(socket_fd, "The command is not allowed.");
            return;
        }
        std::cout << "Remote shell command execution: " << command << std::endl;
        int result = std::system(command.c_str());
        if (result != 0) {
            send_http_response(socket_fd, "The shell command execution failed.");
            return;
        }
    }
    send_http_response(socket_fd, "OK");
}

// 200 OK
void send_http_response(int socket_fd, const std::string& content) {
    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
    send(socket_fd, response.c_str(), response.size(), 0);
    close(socket_fd);
}

// 400 Bad Request
void bad_request(int socket_fd) {
    std::string content = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: 15\r\n\r\n400 Bad Request";
    send(socket_fd, content.c_str(), content.length(), 0);
    close(socket_fd);
}

// 404 Not Found
void not_found(int socket_fd) {
    std::string content = "HTTP/1.1 404 Not Found\r\n";
    send(socket_fd, content.c_str(), content.length(), 0);
    close(socket_fd);
}

