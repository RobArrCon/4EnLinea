#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>

void playGame(int sock) {
    char buffer[1024] = {0};
    while (true) {
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            std::string serverMessage(buffer, valread);
            std::cout << serverMessage;
            if (serverMessage.find("Ganaste") != std::string::npos ||
                serverMessage.find("Gana el Servidor") != std::string::npos ||
                serverMessage.find("Empate") != std::string::npos) {
                break;
            }
            if (serverMessage.find("Ingresa la columna") != std::string::npos) {
                std::string col;
                std::cin >> col;
                send(sock, col.c_str(), col.size(), 0);
            }
        }
    }
}

int main(int argc, char const* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <dirección IP> <puerto>" << std::endl;
        return -1;
    }

    const char* ip = argv[1];
    int port = std::stoi(argv[2]);

    int sock = 0;
    struct sockaddr_in serv_addr;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Error al crear socket" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Dirección inválida/No soportada" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Error al conectar al servidor" << std::endl;
        return -1;
    }

    playGame(sock);

    close(sock);
    return 0;
}
