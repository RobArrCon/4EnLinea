#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <cstdlib>
#include <ctime>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <chrono>

#define ROWS 6
#define COLS 7

std::mutex mtx;
bool waitingForClient = true;

class Game {
public:
    Game(int clientSocket, std::string clientAddr, int clientPort)
        : clientSocket(clientSocket), currentPlayer('C'), clientAddr(clientAddr), clientPort(clientPort) {
        board.resize(ROWS, std::vector<char>(COLS, ' '));
        log("inicia juego el cliente.");
    }

    // Función principal del juego
    void play() {
        sendMessage("Juego iniciado. Tu ficha es 'C'\n");
        try {
            while (true) {
                if (currentPlayer == 'C') {
                    handleClientMove();
                    if (checkWin('C')) {
                        sendMessage("Ganaste!\n");
                        log("gana cliente.");
                        break;
                    }
                    currentPlayer = 'S';
                } else {
                    handleServerMove();
                    if (checkWin('S')) {
                        sendMessage("Gana el Servidor\n");
                        log("gana servidor.");
                        break;
                    }
                    currentPlayer = 'C';
                }
                if (isBoardFull()) {
                    sendMessage("Empate\n");
                    log("empate.");
                    break;
                }
            }
        } catch (const std::exception& e) {
            log("error: " + std::string(e.what()));
        }
        close(clientSocket);
        log("fin del juego.");
    }

private:
    int clientSocket;
    char currentPlayer;
    std::vector<std::vector<char>> board;
    std::string clientAddr;
    int clientPort;

    // Función para registrar eventos del juego
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "Juego [" << clientAddr << ":" << clientPort << "]: " << message << std::endl;
    }

    // Función para enviar mensajes al cliente
    void sendMessage(const std::string& message) {
        send(clientSocket, message.c_str(), message.size(), 0);
    }

    // Función para recibir mensajes del cliente
    std::string receiveMessage() {
        char buffer[1024] = {0};
        int valread = read(clientSocket, buffer, 1024);
        if (valread <= 0) {
            throw std::runtime_error("cliente desconectado");
        }
        return std::string(buffer, valread);
    }

    // Función para manejar el movimiento del cliente
    void handleClientMove() {
        sendMessage("Tu turno. Ingresa la columna (1-7): ");
        int col = std::stoi(receiveMessage()) - 1;
        log("cliente juega columna " + std::to_string(col + 1) + ".");
        makeMove(col, 'C');
        sendBoard();
    }

    // Función para manejar el movimiento del servidor
    void handleServerMove() {
        srand(time(0));
        int col;
        do {
            col = rand() % COLS;
        } while (board[0][col] != ' ');
        log("servidor juega columna " + std::to_string(col + 1) + ".");
        makeMove(col, 'S');
        sendBoard();
    }

    // Función para realizar un movimiento en el tablero
    void makeMove(int col, char player) {
        for (int row = ROWS - 1; row >= 0; --row) {
            if (board[row][col] == ' ') {
                board[row][col] = player;
                break;
            }
        }
    }

    // Función para enviar el estado actual del tablero al cliente
    void sendBoard() {
        std::string boardStr = "TABLERO\n";
        for (const auto& row : board) {
            for (char cell : row) {
                boardStr += (cell == ' ' ? '.' : cell);
                boardStr += ' ';
            }
            boardStr += '\n';
        }
        sendMessage(boardStr);
    }

    // Función para verificar si un jugador ha ganado
    bool checkWin(char player) {
        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLS; ++col) {
                if (checkDirection(row, col, 1, 0, player) || // Horizontal
                    checkDirection(row, col, 0, 1, player) || // Vertical
                    checkDirection(row, col, 1, 1, player) || // Diagonal
                    checkDirection(row, col, 1, -1, player)) { // Diagonal inversa
                    return true;
                }
            }
        }
        return false;
    }

    // Función auxiliar para verificar si hay 4 en línea en una dirección específica
    bool checkDirection(int row, int col, int dRow, int dCol, char player) {
        int count = 0;
        for (int i = 0; i < 4; ++i) {
            int r = row + i * dRow;
            int c = col + i * dCol;
            if (r >= 0 && r < ROWS && c >= 0 && c < COLS && board[r][c] == player) {
                ++count;
            } else {
                break;
            }
        }
        return count == 4;
    }

    // Función para verificar si el tablero está lleno
    bool isBoardFull() {
        for (const auto& row : board) {
            for (char cell : row) {
                if (cell == ' ') {
                    return false;
                }
            }
        }
        return true;
    }
};

// Función para manejar la conexión de un cliente
void handleClient(int clientSocket, struct sockaddr_in clientAddr) {
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);

    {
        std::lock_guard<std::mutex> lock(mtx);
        waitingForClient = false; // Cliente conectado, dejar de esperar
    }
    Game game(clientSocket, std::string(ip), ntohs(clientAddr.sin_port));
    game.play();
    {
        std::lock_guard<std::mutex> lock(mtx);
        waitingForClient = true; // Cliente desconectado, reanudar espera
    }
}

// Función para esperar y mostrar mensajes periódicos de espera de jugadores
void waitForClients() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::lock_guard<std::mutex> lock(mtx);
        if (waitingForClient) {
            std::cout << "Esperando jugador..." << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <PUERTO>" << std::endl;
        return EXIT_FAILURE;
    }

    int port = std::atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        std::cerr << "Puerto inválido: " << argv[1] << std::endl;
        return EXIT_FAILURE;
    }

    int serverFd, clientSocket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Crear el socket del servidor
    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Opción para reutilizar la dirección del socket
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Configuración de la dirección del servidor
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Enlazar el socket a la dirección
    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones entrantes
    if (listen(serverFd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Iniciar el hilo para mostrar mensajes de espera de jugadores
    std::thread(waitForClients).detach();

    while (true) {
        // Aceptar una nueva conexión de cliente
        if ((clientSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        getpeername(clientSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ip, INET_ADDRSTRLEN);
        int clientPort = ntohs(clientAddr.sin_port);
        std::cout << "Juego nuevo[" << ip << ":" << clientPort << "]" << std::endl;

        // Iniciar un nuevo hilo para manejar la conexión del cliente
        std::thread t(handleClient, clientSocket, clientAddr);
        t.detach();
    }


    return 0;
}

