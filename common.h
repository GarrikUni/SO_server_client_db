#ifndef COMMON_H
#define COMMON_H

#include <string>

#define FIFO_NAME "/tmp/db_fifo"

enum RequestType {
    SELECT,
    INSERT,
    UPDATE,
    DELETE
};
struct Registro {
    int id;
    char dado[50];
};

struct Request {
    RequestType type;
    Registro data;
};

#endif
