#pragma once

#include <iostream>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <map>
#include <cstring>
#include <vector>

bool parse_string_to_sockaddr(const std::string& addr, sockaddr_in& sockaddr);

std::vector<std::string> split(const std::string& str, const std::string& delimiter);

std::map<std::string, std::string> parse_headers(const std::string& headers_str);

std::string get_current_path();

bool starts_with(const std::string& str, const std::string& prefix);

