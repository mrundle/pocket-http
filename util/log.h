#ifndef LOG_H
#define LOG_H

#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define fatal(fmt, args...) do { \
	fprintf(stderr, "[FATAL] %s:%u " fmt "\n", \
		basename(__FILE__), \
		__LINE__, \
		##args); \
	exit(EXIT_FAILURE); \
} while (0)

#define log_strerror(fmt, args...) do { \
	fprintf(stderr, "[ERROR] %s:%u " fmt " (err=%s)\n", \
		basename(__FILE__), \
		__LINE__, \
		##args, \
		strerror(errno)); \
} while (0)

#define log_error(fmt, args...) do { \
	fprintf(stderr, "[ERROR] %s:%u " fmt "\n", \
		basename(__FILE__), \
		__LINE__, \
		##args); \
} while (0)

#define log_info(fmt, args...) do { \
	fprintf(stdout, "[INFO] " fmt "\n", ##args); \
} while (0)

#endif // LOG_H
