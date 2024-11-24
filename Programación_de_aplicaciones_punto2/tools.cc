#include "tools.h"

void Uso(const std::string& nombre_programa) {
  std::cout << "Faltan parametros del programa.\n  Uso del programa: " << nombre_programa << " [-v |--verbose] [-h |--help] [-p <puerto>| --port <puerto>] ARCHIVO\n";
}

void MostrarAyuda(const std::string& nombre_programa) {
  std::cout << "El programa " << nombre_programa << " se encarga de obtener el tamaño de un archivo.\nSi se ejecuta con -v(--verbose) se añadirá el modo detallado.\n";
  std::cout << "Si se ejecuta con -p(--port) se ejecuta pidiendo el archivo a un puerto.\n";
}


std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]) {
  std::vector<std::string_view> args(argv + 1, argv + argc);
  program_options options;

  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    if (*it == "-h" || *it == "--help") {
      options.show_help = true;
    } else if (*it == "-v" || *it == "--verbose") {
      options.modo_ampliado = true;
    } else if (*it == "-p" || *it == "--port") {
      // Validar que exista un argumento para el puerto
      if (++it == end || it->starts_with("-")) {
        return std::unexpected(parse_args_errors::missing_argument);
      }
      options.puerto = true;
      options.DOCSERVER_PORT = static_cast<uint16_t>(std::stoi(std::string(*it)));
    } else if (!it->starts_with("-")) {
      // Procesar argumentos sin guion como nombre de archivo o argumentos adicionales
      if (options.nombre_fichero.empty()) {
        options.nombre_fichero = std::string(*it);
      } else {
        options.additional_args.push_back(std::string(*it));
      }
    } else {
      // Opción desconocida
      return std::unexpected(parse_args_errors::unknown_option);
    }
  }

  // Validar que se haya proporcionado un archivo si no se solicita ayuda
  if (!options.show_help && options.nombre_fichero.empty()) {
    return std::unexpected(parse_args_errors::missing_argument);
  }
  return options;
}

//enviar una respuesta
int send_response(const SafeFD& socket, std::string_view header, std::string_view body) {
  if (body.empty()) {
    return 1;
  }
  std::cout << "Socket:" << socket.get() << "\n";
  std::cout << header << "\n";
  std::cout << body << "\n";
  return 0;
}

std::expected<SafeMap, std::string> map_file(SafeFD& fd, size_t length) {
  void* addr = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd.get(), 0);
  if (addr == MAP_FAILED) {
    return std::unexpected(std::strerror(errno));
  }
  return SafeMap{addr, length};
}

SafeFD open_file(const std::string& path, int flags, mode_t mode = 0) {
  int fd = open(path.c_str(), flags, mode);
  return SafeFD{fd};
}

std::expected<SafeMap, int> read_all(const std::string& path, const bool& modo_ampliado) {
  // Abrir el archivo de forma segura
  if (access(path.c_str(), F_OK)) {
    return std::unexpected(403);
  }
  auto fd = open_file(path, O_RDONLY);
  if (fd.get() < 0) {
    return std::unexpected(404);
  }
  if (modo_ampliado == true) {
    std::cout << "open: se abre el archivo <" << path << ">\n";
  }
  
  // Obtener el tamaño del archivo
  off_t length = lseek(fd.get(), 0, SEEK_END);
  if (length < 0) {
    return std::unexpected(errno);
  }
  if (modo_ampliado == true) {
    std::cout << " read: se leen " << length << " bytes del archivo <" << path << ">\n\n";
  }

  // Volver al inicio del archivo
  if (lseek(fd.get(), 0, SEEK_SET) < 0) {
    return std::unexpected(errno);  
  }

  // Mapear el archivo en memoria
  auto map_result = map_file(fd, static_cast<size_t>(length));
  if (!map_result) {
    return std::unexpected(errno);
  }

  return std::move(map_result.value());
}


std::expected<SafeFD, int> make_socket(uint16_t port) {
  // Crear el socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    return std::unexpected(errno);
  }

  // Configurar el socket para reutilizar direcciones
  int opt = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    close(sockfd);
    return std::unexpected(errno);
  }

  // Configurar la dirección y puerto para el bind
  sockaddr_in addr{};
  addr.sin_family = AF_INET;         // IPv4
  addr.sin_port = htons(port);      // Convertir puerto a orden de bytes de red
  addr.sin_addr.s_addr = INADDR_ANY; // Aceptar conexiones de cualquier interfaz

  // Vincular el socket al puerto
  if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    close(sockfd);
    return std::unexpected(errno);
  }

  return SafeFD(sockfd);
}

int listen_connection(const SafeFD& socket) {
  constexpr int conexiones = 5; // Número máximo de conexiones pendientes
  if (listen(socket.get(), conexiones) < 0) {
    return errno;
  }
  return 0; // Éxito
}


// Función para aceptar conexiones entrantes
std::expected<SafeFD, int> accept_connection(const SafeFD& socket, sockaddr_in& client_addr) {
  socklen_t addr_len = sizeof(client_addr);
  int client_fd = accept(socket.get(), reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
  if (client_fd < 0) {
    return std::unexpected(errno);
  }
  return SafeFD(client_fd);
}