// Compilar com: g++ cliente.cpp -o cliente
#include "common.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>

void mostrarMenu() {
    std::cout << "\n=== MENU ===\n";
    std::cout << "1. INSERT\n";
    std::cout << "2. SELECT\n";
    std::cout << "3. UPDATE\n";
    std::cout << "4. DELETE\n";
    std::cout << "0. SAIR\n";
    std::cout << "Escolha a operação (0-4): ";
}

int main() {
    int fd;
    int retries = 0;
    const int max_retries = 10;

    // Tenta abrir o FIFO com O_NONBLOCK e feedback ao usuário
    while ((fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK)) < 0 && retries++ < max_retries) {
        std::cout << "Aguardando servidor...\n";
        sleep(1);
    }
    if (fd < 0) {
        std::cerr << "Servidor indisponível. Tente novamente mais tarde.\n";
        return 1;
    }

    std::cout << "Conectado ao servidor com sucesso!\n";

    while (true) {
        Request req;
        int escolha;

        mostrarMenu();
        std::cin >> escolha;
        std::cin.ignore(); // Limpa o '\n' do buffer

        if (escolha == 0) {
            std::cout << "Encerrando cliente.\n";
            break;
        }

        switch (escolha) {
            case 1: // INSERT
                req.type = INSERT;
                std::cout << "Digite o dado a ser inserido: ";
                std::cin.getline(req.data.dado, 50);
                req.data.id = 0; // Ignorado pelo servidor
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
                continue;
        }

        if (write(fd, &req, sizeof(req)) == -1) {
            perror("Erro ao enviar requisição");
        } else {
            std::cout << "Requisição enviada com sucesso!\n";
        }
    }

    close(fd);
    return 0;
}
