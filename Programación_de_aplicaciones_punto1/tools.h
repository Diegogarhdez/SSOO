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
  bool sin_tamaño = false;
  std::string nombre_fichero;
  std::vector<std::string> additional_args; 
};


std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]);
void Uso(const std::string&);
void MostrarAyuda(const std::string&);
void send_response(std::string_view header, std::string_view body = {}, bool sin_tamaño = false);
std::expected<SafeMap, int> read_all(const std::string& path);


#endif