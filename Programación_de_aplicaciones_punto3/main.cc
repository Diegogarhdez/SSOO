// Universidad de La Laguna
// Escuela Superior de Ingeniería y Tecnología
// Grado en Ingeniería Informática
// Asignatura: Sistemas Operativos
// Curso: 2º
// Práctica 2: Programación de aplicaciones
// Autor: Diego García Hernández
// Correo: alu0101633732@ull.edu.es

#include <arpa/inet.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "tools.h"

int main(const int argc, char* argv[]) {
  auto options = parse_args(argc, argv);
  if (!options.has_value()) {
    switch (options.error()) {
      case parse_args_errors::missing_argument:
        std::cerr << "Error: Faltan argumentos\n";
        break;
      case parse_args_errors::invalid_argument:
        std::cerr << "Error: Argumento inválido\n";
        break;
      default:
        std::cerr << "Error: Argumento desconocido\n";
    }
    return EXIT_FAILURE;
  }

  if (options->show_help) {
    MostrarAyuda(argv[0]);
    return EXIT_SUCCESS;
  }

  bool modo_ampliado = options->modo_ampliado;
  int puerto = options->DOCSERVER_PORT;

  // Configurar el directorio base
  std::string base_dir = options->DOCSERVER_BASEDIR;
  if (base_dir == "getenv") {
    base_dir = getenv("DOCSERVER_BASEDIR");
    if (base_dir.empty()) {
      char cwd[PATH_MAX];
      if (!getcwd(cwd, sizeof(cwd))) {
        std::cerr << "Error al obtener el directorio actual de trabajo\n";
        return EXIT_FAILURE;
      }
      base_dir = cwd;
    }
  }

  // Crear el socket del servidor
  auto socket_result = make_socket(puerto);
  if (!socket_result.has_value()) {
    std::cerr << "Error al crear el socket\n";
    return EXIT_FAILURE;
  }

  SafeFD server_socket = std::move(socket_result.value());

  // Poner el socket en modo escucha
  auto listen_result = listen_connection(server_socket);
  if (listen_result != 0) {
    std::cerr << "Error al poner el socket en modo escucha\n";
    return EXIT_FAILURE;
  }

  std::cout << "Servidor escuchando en el puerto " << puerto << "\n";

  // Bucle principal para aceptar conexiones
  while (true) {
    sockaddr_in client_addr{};
    auto client_socket_result = accept_connection(server_socket, client_addr);
    if (!client_socket_result.has_value()) {
      std::cerr << "Error al aceptar la conexión\n";
      continue;
    }

    SafeFD client_socket = std::move(client_socket_result.value());

    // Leer la petición del cliente
    auto request_result = receive_request(client_socket, 1024);
    if (!request_result.has_value()) {
      if (request_result.error() == ECONNRESET) {
        std::cerr << "Conexión reseteada por el cliente\n";
      } else {
        std::cerr << "Error al recibir la petición: "
                  << strerror(request_result.error()) << "\n";
      }
      continue;
    }

    std::string peticion = request_result.value();
    std::istringstream iss(peticion);
    std::string metodo, archivo;
    iss >> metodo >> archivo;

    if (metodo != "GET" || archivo.empty()) {
      send_response(client_socket, "400 Bad Request", "");
      continue;
    }

    // Construir la ruta completa del archivo
    std::string full_path = base_dir + archivo;
    if (modo_ampliado) {
      std::cerr << "Ruta completa del archivo: " << full_path << "\n";
    }

    // Leer el archivo solicitado
    auto file_result = read_all(full_path, modo_ampliado);
    if (!file_result.has_value()) {
      send_response(client_socket, "404 Not Found", "");
      continue;
    }

    // Acceder al contenido del archivo usando SafeMap
    const SafeMap& file_map = file_result.value();
    std::string_view file_content(reinterpret_cast<const char*>(file_map.data()), file_map.size());

    // Preparar el encabezado
    std::string header = std::format("Content-Length: {}\r\n", file_map.size());

    // Enviar la respuesta
    if (modo_ampliado) {
      std::cerr << "Enviando respuesta al cliente\n";
    }

    int respuesta = send_response(client_socket, header, file_content);
    if (respuesta == ECONNRESET) {
      std::cerr << "Error: Conexión reseteada al enviar la respuesta\n";
    } else if (respuesta != 0) {
      std::cerr << "Error: No se ha podido enviar la respuesta\n";
    }

    return EXIT_SUCCESS;
  }
}
