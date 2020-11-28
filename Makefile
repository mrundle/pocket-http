# Dependencies: gcc

APP = pocket-http
LIBS = \
	-lcrypto \
	-lpthread \
	-lssl
CFLAGS = \
	-Wall \
	-Werror

CERT = certs/cert.pem

UTIL_SRCS = $(wildcard util/*.c)
UTIL_OBJS = $(patsubst %.c,%.o,$(UTIL_SRCS))

all: $(CERT) $(APP)

$(APP): main.c $(UTIL_OBJS)
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

$(CERT):
	@mkdir -p $(dir $@)
	@openssl req -x509 -nodes -days 365 -newkey rsa:1024 -keyout $@ -out $@

clean:
	rm -f $(APP) $(UTIL_OBJS) $(CERT)
