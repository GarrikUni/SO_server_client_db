// Compilar com: g++ cliente.cpp -o cliente
#include "common.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>

using namespace std;

void mostrarMenu() {
    cout << "\n=== MENU ===\n";
    cout << "1. INSERT\n";
    cout << "2. SELECT\n";
    cout << "3. UPDATE\n";
    cout << "4. DELETE\n";
    cout << "0. SAIR\n";
    cout << "Escolha a operação (0-4): ";
}

int main() {
    int fd;
    int retries = 0;
    const int max_retries = 10;

    // Tenta abrir o FIFO com O_NONBLOCK e feedback ao usuário
    while ((fd = open(FIFO_NAME, O_WRONLY | O_NONBLOCK)) < 0 && retries++ < max_retries) {
        cout << "Aguardando servidor...\n";
        sleep(1);
    }
    if (fd < 0) {
        cerr << "Servidor indisponível. Tente novamente mais tarde.\n";
        return 1;
    }

    cout << "Conectado ao servidor com sucesso!\n";

    while (true) {
        Request req;
        int escolha;

        mostrarMenu();
        cin >> escolha;
        cin.ignore(); // Limpa o '\n' do buffer

        if (escolha == 0) {
            cout << "Encerrando cliente.\n";
            break;
        }

        switch (escolha) {
            case 1: // INSERT
                req.type = INSERT;
                cout << "Digite o dado a ser inserido: ";
                cin.getline(req.data.dado, 50);
                req.data.id = 0; // Ignorado pelo servidor
                break;

            case 2: // SELECT
                req.type = SELECT;
                cout << "Digite o ID a buscar (0 para todos): ";
                cin >> req.data.id;
                cin.ignore();
                strcpy(req.data.dado, "");
                break;

            case 3: // UPDATE
                req.type = UPDATE;
                cout << "Digite o ID do registro a atualizar: ";
                cin >> req.data.id;
                cin.ignore();
                cout << "Digite o novo dado: ";
                cin.getline(req.data.dado, 50);
                break;

            case 4: // DELETE
                req.type = DELETE;
                cout << "Digite o ID do registro a deletar: ";
                cin >> req.data.id;
                cin.ignore();
                strcpy(req.data.dado, "");
                break;

            default:
                cout << "Opção inválida.\n";
                continue;
        }

        if (write(fd, &req, sizeof(req)) == -1) {
            perror("Erro ao enviar requisição");
        } else {
            cout << "Requisição enviada com sucesso!\n";
        }
    }

    close(fd);
    return 0;
}
