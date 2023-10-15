#include "test.h"

#define test_req(...) { \
	NetRequest req; \
	const char *d; \
	net_request_init(&req, __VA_ARGS__); \
	d = net_request_serialize(&req); \
	if (d == NULL) { \
		fprintf(stderr, "failed serializing request\n"); \
		return -1; \
	} \
	printf("Request: %s", d); \
	if (net_request_deserialize(&req, d) < 0) { \
		fprintf(stderr, "failed deserializing request\n"); \
		return -1; \
	} \
}

int main(void)
{
	test_req(NET_REQUEST_SUN, "name");
	test_req(NET_REQUEST_MSG, "hello there!");
	return 0;
}
