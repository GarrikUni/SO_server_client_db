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

using namespace std;

const string DB_FILE = "banco.txt";

// Fila de requisições
queue<Request> fila;
mutex fila_mutex;
condition_variable fila_cv;

// Garante acesso seguro ao arquivo
mutex arquivo_mutex;

int gerarNovoId() {
    ifstream in(DB_FILE);
    int maxId = 0;

    string linha;
    while (getline(in, linha)) {
        istringstream iss(linha);
        int id;
        if (iss >> id) {
            if (id > maxId) maxId = id;
        }
    }

    return maxId + 1;
}

void inserir(const Registro& reg) {
    lock_guard<mutex> lock(arquivo_mutex);
    int novoId = gerarNovoId();
    ofstream out(DB_FILE, ios::app);
    out << novoId << " " << reg.dado << "\n";
    cout << "[INSERT] ID " << novoId << " -> " << reg.dado << "\n";
}

void selecionar(int idBusca) {
    lock_guard<mutex> lock(arquivo_mutex);
    ifstream in(DB_FILE);
    string linha;
    bool encontrado = false;

    cout << "[SELECT] Resultado da busca";
    if (idBusca != 0)
        cout << " por ID " << idBusca;
    cout << ":\n";

    while (getline(in, linha)) {
        istringstream iss(linha);
        int id;
        string dado;
        if (iss >> id && getline(iss >> ws, dado)) {
            if (idBusca == 0 || id == idBusca) {
                cout << "ID: " << id << ", Dado: " << dado << "\n";
                encontrado = true;
            }
        }
    }

    if (!encontrado && idBusca != 0)
        cout << "ID " << idBusca << " não encontrado.\n";
}


void atualizar(const Registro& reg) {
    lock_guard<mutex> lock(arquivo_mutex);
    ifstream in(DB_FILE);
    ofstream temp("temp.txt");

    string linha;
    bool encontrado = false;

    while (getline(in, linha)) {
        istringstream iss(linha);
        int id;
        string dado;
        if (iss >> id && getline(iss >> ws, dado)) {
            if (id == reg.id) {
                temp << id << " " << reg.dado << "\n";
                encontrado = true;
                cout << "[UPDATE] ID " << id << " atualizado para: " << reg.dado << "\n";
            } else {
                temp << id << " " << dado << "\n";
            }
        }
    }

    in.close();
    temp.close();
    rename("temp.txt", DB_FILE.c_str());

    if (!encontrado)
        cout << "[UPDATE] ID " << reg.id << " não encontrado.\n";
}

void deletar(const Registro& reg) {
    lock_guard<mutex> lock(arquivo_mutex);
    ifstream in(DB_FILE);
    ofstream temp("temp.txt");

    string linha;
    bool encontrado = false;

    while (getline(in, linha)) {
        istringstream iss(linha);
        int id;
        string dado;
        if (iss >> id && getline(iss >> ws, dado)) {
            if (id == reg.id) {
                encontrado = true;
                cout << "[DELETE] ID " << id << " removido.\n";
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
        cout << "[DELETE] ID " << reg.id << " não encontrado.\n";
}

void processarRequisicoes() {
    while (true) {
        Request req;

        {
            unique_lock<mutex> lock(fila_mutex);
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
        perror("Erro ao abrir pipe");
        return 1;
    }

    cout << "[Servidor iniciado com threads]\n";

    // Criar threads
    const int NUM_THREADS = 4; // thread::hardware_concurrency(); // permitiria que o num de threads sejá adaptável para o hardware disponível;
    vector<thread> workers;
    for (int i = 0; i < NUM_THREADS; ++i) {
        workers.emplace_back(processarRequisicoes);
    }

    // Loop principal de leitura do pipe
    while (true) {
        Request req;
        ssize_t bytes = read(fd, &req, sizeof(req));
        if (bytes == sizeof(req)) {
            {
                lock_guard<mutex> lock(fila_mutex);
                fila.push(req);
            }
            fila_cv.notify_one();
        }
    }

    close(fd);
    return 0;
}


