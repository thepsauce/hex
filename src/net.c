#include "hex.h"

static const char *typeNames[] = {
	[NET_REQUEST_NONE] = "",
	[NET_REQUEST_MSG] = "MSG",
	[NET_REQUEST_SRV] = "SRV",
	[NET_REQUEST_SUN] = "SUN",
	[NET_REQUEST_JIN] = "JIN",
	[NET_REQUEST_LVE] = "LVE",
	[NET_REQUEST_KCK] = "KCK",

	[NET_REQUEST_HIVE_CHALLENGE] = "HCH",
	[NET_REQUEST_HIVE_MOVE] = "HMV",
	[NET_REQUEST_HIVE_RESET] = "HRT",
};

bool net_isvalidname(const char *name)
{
	size_t n = 0;

	for (char ch; (ch = *name) != '\0'; name++, n++)
		if (n >= NET_MAX_NAME && !isalpha(ch))
			return false;
	return n >= NET_MIN_NAME;
}

int net_porthash(const char *name)
{
	int port = 20101;
	int pat = 77;

	for (; *name != '\0'; name++) {
		port ^= pat | *name;
		pat <<= 1;
		pat ^= *name;
	}
	if (port == 0)
		return 16623;
	while (port < 10000)
		port *= 10;
	while (port > 60000)
		port /= 2;
	return port;
}

int net_request_vinit(NetRequest *req, net_request_type_t type, va_list lorig)
{
	va_list l;
	const char *name, *extra;

	gettimeofday(&req->time, NULL);
	req->type = type;
	va_copy(l, lorig);
	switch (type) {
	case NET_REQUEST_HIVE_CHALLENGE:
		/* no parameters */
		break;
	case NET_REQUEST_MSG:
		name = va_arg(l, const char*);
		extra = va_arg(l, const char*);
		if (strlen(extra) >= sizeof(req->extra) ||
				strlen(name) >= sizeof(req->name))
			return -1;
		strcpy(req->name, name);
		strcpy(req->extra, extra);
		break;
	case NET_REQUEST_HIVE_MOVE:
	case NET_REQUEST_SRV:
		extra = va_arg(l, const char*);
		if (strlen(extra) >= sizeof(req->extra))
			return -1;
		strcpy(req->extra, extra);
		break;
	case NET_REQUEST_SUN:
		name = va_arg(l, const char*);
		if (strlen(name) >= sizeof(req->name))
			return -1;
		strcpy(req->name, name);
		break;
	default:
		return -1;
	}
	return 0;
}

int net_request_init(NetRequest *req, net_request_type_t type, ...)
{
	va_list l;

	va_start(l, type);
	const int r = net_request_vinit(req, type, l);
	va_end(l);
	return r;
}

const char *net_request_serialize(const NetRequest *req)
{
	/* this array has an arbitrary size, it's based off estimates
	 * and heavy rounding up
	 */
	static char data[128 + NET_MAX_NAME + 8 + NET_EXTRA_SIZE + 8];
	size_t n;

	n = sprintf(data, "%ld.%ld %s:",
			req->time.tv_sec, req->time.tv_usec,
			typeNames[req->type]);
	switch(req->type) {
	case NET_REQUEST_HIVE_CHALLENGE:
	case NET_REQUEST_HIVE_RESET:
		break;
	case NET_REQUEST_MSG:
		strcpy(data + n, req->name);
		n += strlen(req->name);
		data[n++] = ' ';
		/* fall through */
	case NET_REQUEST_HIVE_MOVE:
	case NET_REQUEST_SRV:
		strcpy(data + n, req->extra);
		n += strlen(req->extra);
		break;
	case NET_REQUEST_SUN:
		strcpy(data + n, req->name);
		n += strlen(req->name);
		break;
	default:
		return NULL;
	}
	data[n++] = '\r';
	data[n] = 0;
	return data;
}

int net_request_deserialize(NetRequest *req, const char *data)
{
	net_request_type_t type;
	size_t i;

	req->time.tv_sec = strtoll(data, (char**) &data, 10);
	if (*data != '.')
		return -1;
	data++;
	req->time.tv_usec = strtoll(data, (char**) &data, 10);
	while (isblank(*data))
		data++;
	for (i = 0; isalpha(data[i]); i++);
	if (i != 3 || data[i] != ':')
		return -1;
	type = (net_request_type_t) -1;
	for (net_request_type_t i = 0;
			i < (net_request_type_t) ARRLEN(typeNames); i++)
		if (!memcmp(data, typeNames[i], 3)) {
			type = i;
			break;
		}
	if (type == (net_request_type_t) -1)
		return -1;
	data += i + 1; /* +1 for colon (:) */
	req->type = type;
	switch (type) {
	case NET_REQUEST_HIVE_CHALLENGE:
	case NET_REQUEST_HIVE_RESET:
		break;
	case NET_REQUEST_MSG:
		for (i = 0; isalpha(data[i]); i++) {
			if (i + 1 == sizeof(req->name))
				return -1;
			req->name[i] = data[i];
		}
		if (i < NET_MIN_NAME)
			return -1;
		req->name[i] = 0;
		data += i;
		while (isblank(*data))
			data++;
		/* fall through */
	case NET_REQUEST_HIVE_MOVE:
	case NET_REQUEST_SRV:
		for (i = 0; data[i] != '\r'; i++) {
			if (i + 1 == sizeof(req->extra))
				return -1;
			req->extra[i] = data[i];
		}
		if (i == 0)
			return -1;
		req->extra[i] = 0;
		data += i;
		break;
	case NET_REQUEST_SUN:
		for (i = 0; isalpha(data[i]); i++) {
			if (i + 1 == sizeof(req->name))
				return -1;
			req->name[i] = data[i];
		}
		if (i < NET_MIN_NAME)
			return -1;
		req->name[i] = 0;
		data += i;
		break;
	default:
		return -1;
	}
	while (isblank(*data))
		data++;
	if (*data != '\r')
		return -1;
	return 0;
}

