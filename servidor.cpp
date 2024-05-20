#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <cstdlib>
#include <ctime>
#include <netinet/in.h>
#include <unistd.h>

#define PORT 7777
#define ROWS 6
#define COLS 7

std::mutex mtx;

class Game {
public:
    Game(int clientSocket) : clientSocket(clientSocket), currentPlayer('C') {
        board.resize(ROWS, std::vector<char>(COLS, ' '));
    }

    void play() {
        sendMessage("Juego iniciado. Tu ficha es 'C'\n");
        while (true) {
            if (currentPlayer == 'C') {
                handleClientMove();
                if (checkWin('C')) {
                    sendMessage("Ganaste!\n");
                    break;
                }
                currentPlayer = 'S';
            } else {
                handleServerMove();
                if (checkWin('S')) {
                    sendMessage("Gana el Servidor\n");
                    break;
                }
                currentPlayer = 'C';
            }
            if (isBoardFull()) {
                sendMessage("Empate\n");
                break;
            }
        }
        close(clientSocket);
    }

private:
    int clientSocket;
    char currentPlayer;
    std::vector<std::vector<char>> board;

    void sendMessage(const std::string& message) {
        send(clientSocket, message.c_str(), message.size(), 0);
    }

    std::string receiveMessage() {
        char buffer[1024] = {0};
        int valread = read(clientSocket, buffer, 1024);
        return std::string(buffer, valread);
    }

    void handleClientMove() {
        sendMessage("Tu turno. Ingresa la columna (1-7): ");
        int col = std::stoi(receiveMessage()) - 1;
        makeMove(col, 'C');
        sendBoard();
    }

    void handleServerMove() {
        srand(time(0));
        int col;
        do {
            col = rand() % COLS;
        } while (board[0][col] != ' ');
        makeMove(col, 'S');
        sendBoard();
    }

    void makeMove(int col, char player) {
        for (int row = ROWS - 1; row >= 0; --row) {
            if (board[row][col] == ' ') {
                board[row][col] = player;
                break;
            }
        }
    }

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

    bool checkWin(char player) {
        // Check horizontal, vertical and diagonal conditions
        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLS; ++col) {
                if (checkDirection(row, col, 1, 0, player) || // Horizontal
                    checkDirection(row, col, 0, 1, player) || // Vertical
                    checkDirection(row, col, 1, 1, player) || // Diagonal /
                    checkDirection(row, col, 1, -1, player)) { // Diagonal \
                    return true;
                }
            }
        }
        return false;
    }

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

void handleClient(int clientSocket) {
    Game game(clientSocket);
    game.play();
}

int main() {
    int serverFd, clientSocket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverFd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Esperando conexiones ..." << std::endl;

    while (true) {
        if ((clientSocket = accept(serverFd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        std::cout << "Juego nuevo[" << inet_ntoa(address.sin_addr) << ":" << ntohs(address.sin_port) << "]" << std::endl;
        std::thread t(handleClient, clientSocket);
        t.detach();
    }

    return 0;
}
