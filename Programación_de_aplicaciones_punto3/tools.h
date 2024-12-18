// Universidad de La Laguna
// Escuela Superior de Ingeniería y Tecnología
// Grado en Ingeniería Informática
// Asignatura: Sistemas Operativos
// Curso: 2º
// Práctica 2: Programación de aplicaciones
// Autor: Diego García Hernández
// Correo: alu0101633732@ull.edu.es

#include <iostream>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <expected>
#include <string>
#include <string_view>
#include <vector>
#include <format>
#include <sstream>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <arpa/inet.h>  
#include <sys/types.h>  
#include <sys/socket.h> 
#include <limits.h>
#include <sys/wait.h>

#include "SafeMap.h"

#ifndef TOOLS_H_
#define TOOLS_H_

enum class parse_args_errors {
  missing_argument,
  unknown_option,
  no_access_file,
  no_file_exists,
  invalid_argument,
};

struct program_options {
  bool show_help = false;
  bool modo_ampliado = false;
  bool puerto = false;
  std::string nombre_fichero;
  int DOCSERVER_PORT = 8080;
  std::vector<std::string> additional_args; 
  std::string DOCSERVER_BASEDIR = "/home/usuario/Sistemas_Operativos/SSOO/Programación_de_aplicaciones_punto3/";
};

struct execute_program_error {
  int exit_code;  // Código de salida del programa
  int error_code; // Código de error del sistema (errno)
};

struct exec_environment {
  std::string request_path;
  std::string server_basedir;
  std::string remote_ip;
  int remote_port;
};

std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]);
void Uso(const std::string&);
void MostrarAyuda(const std::string&);
int send_response(const SafeFD& socket, std::string_view header, std::string_view body = {});
std::expected<SafeMap, int> read_all(const std::string& path, const bool& modo_ampliado);
std::expected<SafeFD, int> make_socket(int port);
int listen_connection(const SafeFD& socket);
std::expected<SafeFD, int> accept_connection(const SafeFD& socket, sockaddr_in& client_addr);
std::expected<std::string, int> receive_request(const SafeFD& socket, size_t max_size);
std::expected<std::string, int> process_request(const std::string& request, const std::string& base_dir);
std::string build_http_response(const std::string& status, size_t content_length, const std::string& content_type = "text/plain");
std::string getenvv(const std::string& name);
std::expected<std::string, execute_program_error> execute_program(const std::string& path, const exec_environment& env);

#endif