#include "util.h"

bool isValidIPAddress(const char* ipAddress) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ipAddress, &(sa.sin_addr)) != 0;
}

bool isValidPort(int port) {
    return port > 0 && port <= 65535;
}

bool parse_string_to_sockaddr(const std::string& addr, sockaddr_in& sockaddr) {
    char ip[16];
    int port;
    std::sscanf(addr.data(), "%[^:]:%d", ip, &port);
    if (!isValidIPAddress(ip)) {
        return false;
    }
    if (!isValidPort(port)) {
        return false;
    }
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET; // IPv4
    sockaddr.sin_addr.s_addr = inet_addr(ip);
    sockaddr.sin_port = htons(port);
    return true;
}

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    std::size_t end = str.find(delimiter);

    while (end != std::string::npos) {
        parts.push_back(str.substr(start, end - start));
        start = end + delimiter.length();
        end = str.find(delimiter, start);
    }
    parts.push_back(str.substr(start));
    return parts;
} 

// Parse http multipart/form-data request headers.
std::map<std::string, std::string> parse_headers(const std::string& headers_str) {
    std::map<std::string, std::string> headers;
    std::stringstream ss(headers_str);
    std::string line;

    while (std::getline(ss, line, '\n')) {
        std::size_t pos = line.find(": ");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2);
            headers[key] = value;
        }
    }
    return headers;
}

std::string get_current_path() {
    char buffer[FILENAME_MAX];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        return std::string(buffer);
    } else {
        return "";
    }
}

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.compare(0, prefix.length(), prefix) == 0;
}

