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
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "tools.h"


int main(int argc, char* argv[]) {
  auto options = parse_args(argc, argv);
  if (!options.has_value()) {
    // Usar options.error() para comprobar el motivo del error
    if (options.error() == parse_args_errors::missing_argument) {
      // Mostrar mensaje de error por falta de argumento
      std::cout << "Faltan argumentos.\n Use " << argv[0] << " [-h|--help] para más información.\n";
    } else if (options.error() == parse_args_errors::unknown_option) {
      std::cout << "Opcion Desconocida.\n Use " << argv[0] << " [-h|--help] para más información.\n";
    }
    return EXIT_FAILURE;
  }

  if (options.value().show_help) {
    MostrarAyuda(argv[0]);
    return EXIT_SUCCESS;
  }

  if (options.value().nombre_fichero.empty()) {
    Uso(argv[0]);
    return EXIT_FAILURE;
  } 

  std::string header;
  std::string body;
  auto resultado = read_all(options.value().nombre_fichero.c_str());
  if (!resultado) {
    int error_code = resultado.error();
    // Manejo de errores específicos
    if (error_code == 404) {
      header = "404 Not Found";
      body = "";
    } else if (error_code == 403) {
      header = "403 Forbidden";
      body = "";
    } else if (error_code > 0) {
      // Manejo de otros errores desconocidos
      std::cout << "ERROR DESCONOCIDO: " << error_code << "\n";
      return EXIT_FAILURE;
    } 
  } else {
    header = std::format("Content-Length: {}\n", resultado.value().size());
    body = resultado.value().get();
  }
  
  send_response(header, body, options.value().sin_tamaño);

  return EXIT_SUCCESS;
}
