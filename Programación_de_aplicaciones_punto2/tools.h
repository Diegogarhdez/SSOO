#include <iostream>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <expected>
#include <string>
#include <string_view>
#include <vector>
#include <format>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <arpa/inet.h>  
#include <sys/types.h>  
#include <sys/socket.h> 

#include "SafeMap.h"

#ifndef TOOLS_H_
#define TOOLS_H_

enum class parse_args_errors {
  missing_argument,
  unknown_option,
  no_access_file,
  no_file_exists,
};

struct program_options {
  bool show_help = false;
  bool modo_ampliado = false;
  bool puerto = false;
  std::string nombre_fichero;
  uint16_t DOCSERVER_PORT;
  std::vector<std::string> additional_args; 
};


std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]);
void Uso(const std::string&);
void MostrarAyuda(const std::string&);
int send_response(const SafeFD& socket, std::string_view header, std::string_view body = {});
std::expected<SafeMap, int> read_all(const std::string& path, const bool& modo_ampliado);
std::expected<SafeFD, int> make_socket(uint16_t port);
int listen_connection(const SafeFD& socket);
std::expected<SafeFD, int> accept_connection(const SafeFD& socket, sockaddr_in& client_addr);


#endif