#ifndef SOCKET_H
#define SOCKET_H

#include "types.h"

int socket_create(const uint16_t port, int *const socket_desc);
int socket_handle(const int socket_desc, int (*handle)(const int socket_desc));

#endif // SOCKET_H
