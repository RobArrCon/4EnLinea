all: servidor cliente

servidor: servidor.cpp
    g++ -o servidor servidor.cpp -pthread

cliente: cliente.cpp
    g++ -o cliente cliente.cpp

clean:
    rm -f servidor cliente
