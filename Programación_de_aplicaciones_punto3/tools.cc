#include "tools.h"

void Uso(const std::string& nombre_programa) {
  std::cerr << "Faltan parametros del programa.\n  Uso del programa: "
            << nombre_programa
            << " [-v |--verbose] [-h |--help] [-p <puerto>| --port <puerto>] "
               "ARCHIVO\n";
}

void MostrarAyuda(const std::string& nombre_programa) {
  std::cout << "El programa " << nombre_programa
            << " se encarga de obtener el tamaño de un archivo.\nSi se ejecuta "
               "con -v(--verbose) se añadirá el modo detallado.\n";
  std::cout << "Si se ejecuta con -p(--port) se ejecuta pidiendo el archivo a "
               "un puerto.\n";
}

std::expected<program_options, parse_args_errors> parse_args(int argc,
                                                             char* argv[]) {
  std::vector<std::string_view> args(argv + 1, argv + argc);
  program_options options;

  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    if (*it == "-h" || *it == "--help") {
      options.show_help = true;
    } else if (*it == "-v" || *it == "--verbose") {
      options.modo_ampliado = true;
    } else if (*it == "-p" || *it == "--port") {
      if (++it == end || it->starts_with("-")) {
        return std::unexpected(parse_args_errors::missing_argument);
      }
      int port = std::stoi(std::string(*it));
      if (port < 1 || port > 65535) {
        return std::unexpected(parse_args_errors::invalid_argument);
      }
      options.DOCSERVER_PORT = port;

    } else if (*it == "-b" || *it == "--base") {
      if (++it == end || it->starts_with("-")) {
        return std::unexpected(parse_args_errors::missing_argument);
      }
      options.DOCSERVER_BASEDIR = std::string(*it);
    } else {
      return std::unexpected(parse_args_errors::unknown_option);
    }
  }

  if (options.DOCSERVER_BASEDIR.empty()) {
    const char* env_base_dir = std::getenv("DOCSERVER_BASEDIR");
    if (env_base_dir) {
      options.DOCSERVER_BASEDIR = env_base_dir;
    } else {
      char cwd[PATH_MAX];
      if (!getcwd(cwd, sizeof(cwd))) {
        std::cerr << "Error al obtener el directorio actual de trabajo: "
                  << strerror(errno) << "\n";
        return std::unexpected(parse_args_errors::missing_argument);
      }
      options.DOCSERVER_BASEDIR = cwd;
    }
  }

  return options;
}

std::string getenvv(const std::string& name) {
  char* value{getenv(name.c_str())};
  if (value) {
    return std::string{value};
  } else {
    return std::string{};
  }
}

int send_response(const SafeFD& socket, std::string_view header,
                  std::string_view body) {
  std::string response =
      std::string(header) + "\r\n\r\n" + std::string(body) + "\r\n";
  ssize_t bytes_sent = send(socket.get(), response.data(), response.size(), 0);

  if (bytes_sent < 0) {
    std::cerr << "Error al enviar la respuesta: " << strerror(errno) << "\n";
    return errno;
  }

  return 0;  // Éxito
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

std::expected<SafeMap, int> read_all(const std::string& path,
                                     const bool& modo_ampliado) {
  if (access(path.c_str(), F_OK)) {
    return std::unexpected(403);  // Archivo no accesible
  }

  auto fd = open_file(path, O_RDONLY);
  if (fd.get() < 0) {
    return std::unexpected(404);  // Archivo no encontrado
  }

  off_t length = lseek(fd.get(), 0, SEEK_END);
  if (length < 0) {
    return std::unexpected(errno);  // Error al obtener tamaño del archivo
  }

  if (lseek(fd.get(), 0, SEEK_SET) < 0) {
    return std::unexpected(errno);  // Error al volver al inicio
  }

  auto map_result = map_file(fd, static_cast<size_t>(length));
  if (!map_result) {
    return std::unexpected(errno);  // Error al mapear archivo
  }

  if (modo_ampliado) {
    std::cout << "Archivo mapeado correctamente en memoria: " << path << "\n";
  }

  return std::move(map_result.value());
}

std::expected<SafeFD, int> make_socket(int port) {
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
  addr.sin_family = AF_INET;    // IPv4
  addr.sin_port = htons(port);  // Convertir puerto a orden de bytes de red
  addr.sin_addr.s_addr =
      INADDR_ANY;  // Aceptar conexiones de cualquier interfaz
  // Vincular el socket al puerto
  if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    close(sockfd);
    return std::unexpected(errno);
  }

  int option = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  return SafeFD(sockfd);
}

int listen_connection(const SafeFD& socket) {
  constexpr int conexiones = 5;  // Número máximo de conexiones pendientes
  if (listen(socket.get(), conexiones) < 0) {
    return errno;
  }
  return 0;  // Éxito
}

std::expected<SafeFD, int> accept_connection(const SafeFD& socket,
                                             sockaddr_in& client_addr) {
  socklen_t addr_len = sizeof(client_addr);
  int client_fd = accept(socket.get(),
                         reinterpret_cast<sockaddr*>(&client_addr), &addr_len);
  if (client_fd < 0) {
    std::cerr << "Error al aceptar la conexión: " << strerror(errno) << "\n";
    return std::unexpected(errno);
  }
  return SafeFD(client_fd);
}

std::expected<std::string, int> receive_request(const SafeFD& socket,
                                                size_t max_size) {
  std::string message_text;
  message_text.resize(max_size);
  ssize_t bytes_read{recv(socket.get(), message_text.data(), max_size, 0)};
  if (bytes_read < 0) {
    return std::unexpected(errno);
  }
  message_text.resize(bytes_read);
  return message_text;
}

std::expected<std::string, int> process_request(const std::string& request,
                                                const std::string& base_dir) {
  // Analizar la solicitud
  std::istringstream stream(request);
  std::string method, file_path;
  stream >> method >> file_path;

  // Validar el método HTTP
  if (method != "GET") {
    std::cout << "No GET\n";
    return std::unexpected(400);  // Bad Request
  }

  // Validar el archivo solicitado
  if (file_path.empty() || file_path[0] != '/') {
    std::cout << "Ruta no absoluta o archivo vacío\n";
    std::cout << file_path << "\n";
    return std::unexpected(400);  // Bad Request
  }

  // Normalizar el directorio base y construir la ruta completa
  std::filesystem::path normalized_base =
      std::filesystem::path(base_dir).lexically_normal();
  std::string trimmed_path = file_path.substr(1);  // Eliminar el '/' inicial
  std::filesystem::path full_path = normalized_base / trimmed_path;
  full_path = full_path.lexically_normal();  // Normalizar la ruta completa

  // Verificar que la ruta esté dentro del directorio base
  if (full_path.string().find(normalized_base.string()) != 0) {
    std::cout << "Ruta fuera del directorio base\n";
    std::cout << full_path << "\n";
    return std::unexpected(403);  // Forbidden
  }

  // Verificar la existencia y validez del archivo
  if (!std::filesystem::exists(full_path) ||
      !std::filesystem::is_regular_file(full_path)) {
    std::cout << "Archivo no encontrado o no regular\n";
    return std::unexpected(404);  // Not Found
  }

  // Retornar la ruta completa del archivo
  return full_path.string();
}

std::expected<std::string, execute_program_error> execute_program(
    const std::string& path,
    const exec_environment& env) {
  
  // Verificar permisos de ejecución
  if (access(path.c_str(), X_OK) != 0) {
    return std::unexpected(execute_program_error{
        .exit_code = -1, .error_code = errno});
  }

  // Crear una tubería
  int pipe_fd[2];
  if (pipe(pipe_fd) == -1) {
    return std::unexpected(execute_program_error{
        .exit_code = -1, .error_code = errno});
  }

  pid_t pid = fork();
  if (pid == -1) { // Error al hacer fork
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return std::unexpected(execute_program_error{
        .exit_code = -1, .error_code = errno});
  }

  if (pid == 0) { // Proceso hijo
    dup2(pipe_fd[1], STDOUT_FILENO); // Redirigir stdout a la tubería
    close(pipe_fd[0]);               // Cerrar lectura de la tubería
    close(pipe_fd[1]);

    // Configurar variables de entorno
    setenv("REQUEST_PATH", env.request_path.c_str(), 1);
    setenv("SERVER_BASEDIR", env.server_basedir.c_str(), 1);
    setenv("REMOTE_IP", env.remote_ip.c_str(), 1);
    setenv("REMOTE_PORT", std::to_string(env.remote_port).c_str(), 1);

    // Ejecutar el programa
    execl(path.c_str(), path.c_str(), nullptr);
    _exit(127); // Terminar si execl falla
  }

  // Proceso padre
  close(pipe_fd[1]); // Cerrar escritura de la tubería

  std::string output;
  char buffer[1024];
  ssize_t bytes_read;

  while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
    output.append(buffer, bytes_read);
  }

  close(pipe_fd[0]); // Cerrar lectura de la tubería

  // Esperar al hijo
  int status;
  if (waitpid(pid, &status, 0) == -1) {
    return std::unexpected(execute_program_error{
        .exit_code = -1, .error_code = errno});
  }

  // Verificar si el hijo terminó correctamente
  if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
    return output;
  }

  return std::unexpected(execute_program_error{
      .exit_code = WEXITSTATUS(status), .error_code = 0});
}