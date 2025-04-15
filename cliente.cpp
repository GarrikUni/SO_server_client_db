#include "common.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

// Compilar com: g++ cliente.cpp -o cliente

void mostrarMenu() {
    std::cout << "\n=== MENU ===\n";
    std::cout << "1. INSERT\n";
    std::cout << "2. SELECT\n";
    std::cout << "3. UPDATE\n";
    std::cout << "4. DELETE\n";
    std::cout << "Escolha a operação (1-4): ";
}

int main() {
    int fd = open(FIFO_NAME, O_WRONLY);
    if (fd < 0) {
        perror("Erro ao abrir FIFO");
        return 1;
    }

    Request req;
    int escolha;

    while (true) {

        Request req;
        int escolha;

        mostrarMenu();
        std::cin >> escolha;
        std::cin.ignore(); // Limpar buffer do enter

        if (escolha == 0) {
            std::cout << "Encerrando cliente.\n";
            break;
        }

        switch (escolha) {
            case 1: // INSERT
                req.type = INSERT;
                std::cout << "Digite o dado a ser inserido: ";
                std::cin.getline(req.data.dado, 50);
                req.data.id = 0; // ignorado pelo servidor
                break;

            case 2: // SELECT
                req.type = SELECT;
                std::cout << "Digite o ID a buscar (0 para todos): ";
                std::cin >> req.data.id;
                std::cin.ignore();
                strcpy(req.data.dado, "");
                break;

            case 3: // UPDATE
                req.type = UPDATE;
                std::cout << "Digite o ID do registro a atualizar: ";
                std::cin >> req.data.id;
                std::cin.ignore();
                std::cout << "Digite o novo dado: ";
                std::cin.getline(req.data.dado, 50);
                break;

            case 4: // DELETE
                req.type = DELETE;
                std::cout << "Digite o ID do registro a deletar: ";
                std::cin >> req.data.id;
                std::cin.ignore();
                strcpy(req.data.dado, "");
                break;

            default:
                std::cout << "Opção inválida.\n";
                continue; // volta pro menu
        }

        write(fd, &req, sizeof(req));
        std::cout << "Requisição enviada!\n";
    }

    close(fd);
    return 0;
}

