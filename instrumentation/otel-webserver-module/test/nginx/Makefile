CC ?= cc
CFLAGS ?= -O1 -g -Wall -Wextra -Werror

.PHONY: test asan clean

test: propagation_headers_test
	./propagation_headers_test

asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: clean test

propagation_headers_test: propagation_headers_test.c ../../src/nginx/ngx_http_opentelemetry_module_internal.h
	$(CC) $(CFLAGS) -std=c11 propagation_headers_test.c -o $@

clean:
	rm -f propagation_headers_test
