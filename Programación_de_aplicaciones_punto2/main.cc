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
#include <arpa/inet.h>
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/socket.h> 

#include "tools.h"


int main(int argc, char* argv[]) {
  auto options = parse_args(argc, argv);
  if (!options.has_value()) {
    // Usar options.error() para comprobar el motivo del error
    if (options.error() == parse_args_errors::missing_argument) {
      // Mostrar mensaje de error por falta de argumento
      std::cerr << "Faltan argumentos.\n Use " << argv[0] << " [-h|--help] para más información.\n";
    } else if (options.error() == parse_args_errors::unknown_option) {
      std::cerr << "Opcion Desconocida.\n Use " << argv[0] << " [-h|--help] para más información.\n";
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
  auto socket_result = make_socket(options.value().DOCSERVER_PORT);
  if (!socket_result) {
    std::cerr << "Error al crear el socket: " << socket_result.error() << "\n";
    return EXIT_FAILURE;
  }
  SafeFD server_socket = std::move(socket_result.value());
  
  if (int err = listen_connection(server_socket); err != 0) {
    std::cerr << "Error al poner el socket en modo de escucha.\n";
    return EXIT_FAILURE;
  }

  std::cout << "Socket creado y escuchando en el puerto " << options.value().DOCSERVER_PORT << "\n";

  sockaddr_in client_addr{};
  while (true) {
    auto client_socket_result = accept_connection(server_socket, client_addr);
    if (!client_socket_result) {
      std::cerr << "Error al aceptar una conexión: " << strerror(client_socket_result.error()) << "\n";
      continue;
    }

    SafeFD client_socket = std::move(client_socket_result.value());
    std::cout << "Conexión completada!\n"; 

    std::string header;
    std::string body;
    auto resultado = read_all(options.value().nombre_fichero.c_str(), options.value().modo_ampliado);
    if (!resultado) {
      int error_code = resultado.error();
      // Manejo de errores específicos
      if (error_code == 404) {
        std::cerr << "404 Not Found\n";
        return EXIT_FAILURE;
      } else if (error_code == 403) {
        std::cerr <<"403 Forbidden\n";
        return EXIT_FAILURE;
      } else if (error_code > 0) {
        // Manejo de otros errores desconocidos
        std::cout << "ERROR DESCONOCIDO: " << error_code << "\n";
        return EXIT_FAILURE;
      } 
    } else {
      header = std::format("Content-Length: {}\n", resultado.value().size());
      body = resultado.value().get();
    }
    int respuesta = send_response(client_socket, header, body);
    if (respuesta == 1) {
      std::cerr << "El archivo de entrada esta vacío\n";
    }
  }
  return EXIT_SUCCESS;
}
