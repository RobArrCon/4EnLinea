# Makefile para compilar los programas cliente y servidor

# Compilador y flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -pthread

# Nombres de los ejecutables
SERVER = servidor
CLIENT = cliente

# Regla para compilar todo
all: $(SERVER) $(CLIENT)

# Regla para compilar el servidor
$(SERVER): servidor.cpp
	$(CXX) $(CXXFLAGS) -o $(SERVER) servidor.cpp

# Regla para compilar el cliente
$(CLIENT): cliente.cpp
	$(CXX) $(CXXFLAGS) -o $(CLIENT) cliente.cpp

# Regla para limpiar los archivos compilados
clean:
	rm -f $(SERVER) $(CLIENT)
