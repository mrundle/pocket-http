#include "util/http.h"
#include "util/https.h"
#include "util/log.h"
#include "util/socket.h"
#include "util/types.h"

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define PORT_HTTP 80
#define PORT_HTTPS 443

/* TODO move to util/handler.h */
struct handler_config {
	uint16_t port;
	int (*setup)(void);
	int (*handle)(const int socket_desc);
};

struct handler_state {
	pthread_t pt;
	int socket_desc;
	volatile bool running;
};

struct handler {
	struct handler_config config;
	struct handler_state state;
};

static struct handler handlers[] = {
	{
		.config = {
			.port = PORT_HTTP,
			.setup = NULL,
			.handle = http_handle,
		}
	},
	{
		.config = {
			.port = PORT_HTTPS,
			.setup = https_setup,
			.handle = https_handle,
		}
	},
};

static void *
handler(void *arg)
{
	if (arg == NULL) {
		return NULL;
	}
	struct handler *const hh = (struct handler *)arg;
	hh->state.running = true;

	while (socket_handle(hh->state.socket_desc, hh->config.handle) == 0) {
		log_info("handled request on port %u", hh->config.port);
	}

	log_error("failed to handle request on port %u", hh->config.port);
	close(hh->state.socket_desc);
	return NULL;
}

static int
launch_handler(struct handler *const hh)
{
	if (hh == NULL) {
		return -EINVAL;
	}

	/* create socket */
	if (socket_create(hh->config.port, &hh->state.socket_desc) != 0) {
		log_error("failed to create socket");
		return -1;
	}

	/* optional setup */
	if (hh->config.setup != NULL) {
		if (hh->config.setup() != 0) {
			log_error("failed to set up handler");
			return -1;
		}
	}

	/* launch handler thread */
	if (pthread_create(&hh->state.pt, NULL, handler, hh) != 0) {
		log_error("failed to launch handler");
		return -1;
	}

	return 0;
}

int
main(int argc, char **argv)
{
	if (getuid() != 0) {
		fprintf(stderr, "%s: needs to run as root\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if (argc != 2) {
		fprintf(stderr, "usage: %s /path/to/docroot\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* change to docroot */
	const char *const root = argv[1];
	if (chroot(root) == -1) {
		log_strerror("failed to chroot %s", root);
		exit(EXIT_FAILURE);
	}

	for (unsigned i = 0; i < elementsof(handlers); i++) {
		if (launch_handler(&handlers[i]) != 0) {
			fatal("failed to launch handler");
		}
	}

	while (true) {
		sleep(1);
		for (unsigned i = 0; i < elementsof(handlers); i++) {
			if (!handlers[i].state.running) {
				goto out;
			}
		}
	}

out:
	log_info("finished");
	return 0;
}
