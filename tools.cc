#include "tools.h"

void Uso(const std::string& nombre_programa) {
  std::cout << "Faltan parametros del programa.\n  Uso del programa: " << nombre_programa << " [-v |--verbose] [-h |--help] ARCHIVO\n";
}

void MostrarAyuda(const std::string& nombre_programa) {
  std::cout << "El programa " << nombre_programa << " se encarga de obtener el tamaño de un archivo.\nSi se ejecuta con -v(--verbose) se añadirá el modo detallado.\n";
}


std::expected<program_options, parse_args_errors> parse_args(int argc, char* argv[]) {
  std::vector<std::string_view> args(argv + 1, argv + argc);
  program_options options;

  for (auto it = args.begin(), end = args.end(); it != end; ++it) {
    if (*it == "-h" || *it == "--help") {
      options.show_help = true;
    } else if (*it == "-v" || *it == "--verbose") {
      options.modo_ampliado = true;
    } else if (!it->starts_with("-")) {
      if (options.nombre_fichero.empty()) {
        // El primer argumento sin "-" se asume como el archivo principal.
        options.nombre_fichero = *it;
      } else {
        // Otros argumentos no reconocidos se almacenan como adicionales.
        options.additional_args.push_back(std::string(*it));
      }
    } else {
      // Cualquier otra opción desconocida se considera un error.
      return std::unexpected(parse_args_errors::unknown_option);
    }
  }

  // Mostrar ayuda solo si se solicitó explícitamente.
  if (options.show_help) {
    return options;
  }
  // Validar que se proporcionó un archivo si no se solicita ayuda.
  if (options.nombre_fichero.empty()) {
    return std::unexpected(parse_args_errors::missing_argument);
  }

  return options;
}

void send_response(std::string_view header, std::string_view body) {
  std::cout << header << "\n";
  std::cout << body << "\n";
}

std::expected<SafeMap, std::string> map_file(SafeFD& fd, size_t length) {
  void* addr = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd.get(), 0);
  if (addr == MAP_FAILED) {
    return std::unexpected(std::strerror(errno));
  }
  return SafeMap{addr, length};
}

std::expected<std::vector<uint8_t>, std::string> read_file(SafeFD& fd, size_t max_size) {
  std::vector<uint8_t> buffer(max_size);
  ssize_t bytes_read = read(fd.get(), buffer.data(), max_size);
  if (bytes_read < 0) {
    return std::unexpected(std::strerror(errno));
  }
  buffer.resize(bytes_read);
  return buffer;
}

SafeFD open_file(const std::string& path, int flags, mode_t mode = 0) {
  int fd = open(path.c_str(), flags, mode);
  return SafeFD{fd};
}

std::expected<SafeMap, int> read_all(const std::string& path) {
  // Abrir el archivo de forma segura
  if (access(path.c_str(), F_OK)) {
    return std::unexpected(403);
  }
  auto fd = open_file(path, O_RDONLY);
  if (fd.get() < 0) {
    return std::unexpected(404);
  }

  // Obtener el tamaño del archivo
  off_t length = lseek(fd.get(), 0, SEEK_END);
  if (length < 0) {
    return std::unexpected(errno);
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
