// g++ servidor.cpp -o servidor -pthread
// g++ cliente.cpp -o cliente

#include "common.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

const std::string DB_FILE = "banco.txt";

// Fila de requisições
std::queue<Request> fila;
std::mutex fila_mutex;
std::condition_variable fila_cv;

// Garante acesso seguro ao arquivo
std::mutex arquivo_mutex;

int gerarNovoId() {
    std::ifstream in(DB_FILE);
    int maxId = 0;

    std::string linha;
    while (std::getline(in, linha)) {
        std::istringstream iss(linha);
        int id;
        if (iss >> id) {
            if (id > maxId) maxId = id;
        }
    }

    return maxId + 1;
}

void inserir(const Registro& reg) {
    std::lock_guard<std::mutex> lock(arquivo_mutex);
    int novoId = gerarNovoId();
    std::ofstream out(DB_FILE, std::ios::app);
    out << novoId << " " << reg.dado << "\n";
    std::cout << "[INSERT] ID " << novoId << " -> " << reg.dado << "\n";
}

void selecionar(int idBusca) {
    std::lock_guard<std::mutex> lock(arquivo_mutex);
    std::ifstream in(DB_FILE);
    std::string linha;
    bool encontrado = false;

    std::cout << "[SELECT] Resultado da busca";
    if (idBusca != 0)
        std::cout << " por ID " << idBusca;
    std::cout << ":\n";

    while (std::getline(in, linha)) {
        std::istringstream iss(linha);
        int id;
        std::string dado;
        if (iss >> id && std::getline(iss >> std::ws, dado)) {
            if (idBusca == 0 || id == idBusca) {
                std::cout << "ID: " << id << ", Dado: " << dado << "\n";
                encontrado = true;
            }
        }
    }

    if (!encontrado && idBusca != 0)
        std::cout << "ID " << idBusca << " não encontrado.\n";
}


void atualizar(const Registro& reg) {
    std::lock_guard<std::mutex> lock(arquivo_mutex);
    std::ifstream in(DB_FILE);
    std::ofstream temp("temp.txt");

    std::string linha;
    bool encontrado = false;

    while (std::getline(in, linha)) {
        std::istringstream iss(linha);
        int id;
        std::string dado;
        if (iss >> id && std::getline(iss >> std::ws, dado)) {
            if (id == reg.id) {
                temp << id << " " << reg.dado << "\n";
                encontrado = true;
                std::cout << "[UPDATE] ID " << id << " atualizado para: " << reg.dado << "\n";
            } else {
                temp << id << " " << dado << "\n";
            }
        }
    }

    in.close();
    temp.close();
    rename("temp.txt", DB_FILE.c_str());

    if (!encontrado)
        std::cout << "[UPDATE] ID " << reg.id << " não encontrado.\n";
}

void deletar(const Registro& reg) {
    std::lock_guard<std::mutex> lock(arquivo_mutex);
    std::ifstream in(DB_FILE);
    std::ofstream temp("temp.txt");

    std::string linha;
    bool encontrado = false;

    while (std::getline(in, linha)) {
        std::istringstream iss(linha);
        int id;
        std::string dado;
        if (iss >> id && std::getline(iss >> std::ws, dado)) {
            if (id == reg.id) {
                encontrado = true;
                std::cout << "[DELETE] ID " << id << " removido.\n";
                continue; // pula o registro deletado
            } else {
                temp << id << " " << dado << "\n";
            }
        }
    }

    in.close();
    temp.close();
    rename("temp.txt", DB_FILE.c_str());

    if (!encontrado)
        std::cout << "[DELETE] ID " << reg.id << " não encontrado.\n";
}

void processarRequisicoes() {
    while (true) {
        Request req;

        {
            std::unique_lock<std::mutex> lock(fila_mutex);
            fila_cv.wait(lock, [] { return !fila.empty(); });

            req = fila.front();
            fila.pop();
        }

        switch (req.type) {
            case INSERT: inserir(req.data); break;
            case SELECT: selecionar(req.data.id); break;
            case UPDATE: atualizar(req.data); break;
            case DELETE: deletar(req.data); break;
        }
    }
}

int main() {
    mkfifo(FIFO_NAME, 0666);
    int fd = open(FIFO_NAME, O_RDONLY);
    if (fd < 0) {
        perror("Erro ao abrir FIFO");
        return 1;
    }

    std::cout << "[Servidor iniciado com threads]\n";

    // Criar threads de trabalho
    const int NUM_THREADS = 4;
    std::vector<std::thread> workers;
    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back(processarRequisicoes);
    }

    // Loop principal de leitura do FIFO
    while (true) {
        Request req;
        ssize_t bytes = read(fd, &req, sizeof(req));
        if (bytes == sizeof(req)) {
            {
                std::lock_guard<std::mutex> lock(fila_mutex);
                fila.push(req);
            }
            fila_cv.notify_one();
        }
    }

    close(fd);
    return 0;
}


