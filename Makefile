# Makefile para HULK Compiler
# Ubicación: Raíz del proyecto

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = 

# Directorios
SRC_DIR = src
BUILD_DIR = build
TARGET = hulk

# Buscar todos los archivos .cpp en src/ y subdirectorios
SOURCES = $(shell find $(SRC_DIR) -name "*.cpp")
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Directorios de inclusión
INCLUDES = -I$(SRC_DIR)

# Colores para output (opcional)
RED = \033[0;31m
GREEN = \033[0;32m
YELLOW = \033[1;33m
NC = \033[0m

.PHONY: all build clean run test help

# Target por defecto
all: build

# Compilar el ejecutable
build: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "$(GREEN)🔨 Linking...$(NC)"
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "$(GREEN)✅ Compilation successful!$(NC)"
	@echo "   Run: ./$(TARGET) <file.hulk>"

# Compilar cada archivo .cpp a .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "   Compiling $<"
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Limpiar archivos generados
clean:
	@echo "$(YELLOW)🧹 Cleaning...$(NC)"
	rm -rf $(BUILD_DIR) $(TARGET)
	@echo "$(GREEN)✅ Clean complete!$(NC)"

# Ejecutar el compilador (ejemplo)
run: build
	@echo "$(GREEN)🚀 Running...$(NC)"
	./$(TARGET) examples/test.hulk

# Ejecutar en modo REPL
repl: build
	@echo "$(GREEN)🚀 Starting REPL...$(NC)"
	./$(TARGET)

# Ejecutar pruebas
test: build
	@echo "$(GREEN)🧪 Running tests...$(NC)"
	@for test in tests/input/*.hulk; do \
		echo "   Testing $$test"; \
		./$(TARGET) "$$test"; \
	done

# Mostrar ayuda
help:
	@echo "HULK Compiler - Makefile Commands:"
	@echo ""
	@echo "  make build    - Compilar el compilador"
	@echo "  make clean    - Limpiar archivos generados"
	@echo "  make run      - Compilar y ejecutar con examples/test.hulk"
	@echo "  make repl     - Ejecutar en modo REPL interactivo"
	@echo "  make test     - Ejecutar todas las pruebas"
	@echo "  make help     - Mostrar esta ayuda"
	@echo ""
	@echo "Uso normal:"
	@echo "  ./hulk archivo.hulk"