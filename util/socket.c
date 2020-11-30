#include "socket.h"
#include "log.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
socket_create(const uint16_t port, int *const socket_desc_out)
{
	if (socket_desc_out == NULL) {
		log_error("invalid input");
		return -1;
	}

	/* create tcp socket */
	int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sd == -1) {
		log_strerror("failed to create socket");
		return sd;
	}

	/* allow for socket reuse */
	int optval = 1;
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
		log_strerror("failed to set SO_REUSEADDR");
		close(sd);
		return -1;
	}

	/* bind socket to the specified port */
	struct sockaddr_in saddr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(port),
	};
	if (bind(sd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) {
		log_strerror("failed to bind socket on port %u", port);
		return -1;
	}

	/*
 	 * mark the socket as passive, indicating that it will be used
 	 * to accept incoming connections
 	 */
	if (listen(sd, 8) != 0) {
		log_strerror("failed to listen");
		return -1;
	}

	*socket_desc_out = sd;
	return 0;
}

int
socket_handle(const int socket_desc, int (*handle)(const int socket_desc))
{
	/* accept */
	struct sockaddr_in saddr;
	unsigned len = sizeof(saddr);
	int client_desc = accept(socket_desc, (struct sockaddr *)&saddr, &len);
	if (client_desc < 0) {
		log_strerror("failed to accept");
		return -1;
	}

	/* run the handler */
	if (handle(client_desc) != 0) {
		log_error("failed to handle request");
	}

	shutdown(client_desc, SHUT_RDWR);
	close(client_desc);
	return 0;
}
