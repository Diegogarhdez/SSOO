# Nombre del ejecutable
EXEC = docserver

# Archivos fuente
SRCS = main.cc tools.cc

# Archivos objeto (el sufijo .o se refiere a los archivos objeto generados a partir de los archivos fuente)
OBJS = $(SRCS:.cpp=.o)

# Compilador
CXX = g++

# Flags del compilador
CXXFLAGS = -std=c++23 -Wall -Wextra -Werror -Wpedantic 

# Reglas

# Regla por defecto, compila y genera el ejecutable
all: $(EXEC)

# Regla para generar el ejecutable
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(OBJS)

# Regla para limpiar los archivos objeto y el ejecutable
clean:
	rm $(EXEC)

# Regla para ejecutar el programa
run: $(EXEC)
	./$(EXEC) 

# Regla para mostrar ayuda
help:
	@echo "Uso del Makefile:"
	@echo "  make          - Compila el proyecto"
	@echo "  make run      - Ejecuta el programa"
	@echo "  make clean    - Limpia archivos objeto y el ejecutable"
	@echo "  make help     - Muestra esta ayuda"
