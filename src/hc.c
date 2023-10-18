#include "hex.h"

void hc_init(HiveChat *hc)
{
	if (hive_init(&hc->hive, 0, 0, COLS / 2 - 1, LINES - 1) < 0 ||
			net_chat_init(&hc->chat, COLS / 2, 0,
				COLS - COLS / 2, LINES - 1, 10000) < 0) {
		endwin();
		fprintf(stderr, "failed initializing\n");
		exit(-1);
	}
}

static char *hc_serializemove(const HiveMove *move)
{
	static char data[256];

	if (snprintf(data, sizeof(data), "%s %d,%d %d,%d",
			move->fromInventory ? "true" : "false",
			move->from.x, move->from.y,
			move->to.x, move->to.y) == sizeof(data))
		return NULL;
	return data;
}

static int hc_deserializemove(const char *data, HiveMove *move)
{
	if (strncmp(data, "true ", sizeof("true")) == 0) {
		move->fromInventory = true;
		data += sizeof("true ");
	} else if (strncmp(data, "false ", sizeof("false")) == 0) {
		move->fromInventory = false;
		data += sizeof("false ");
	}

	move->from.x = strtol(data, (char**) &data, 10);
	if (*data != ',')
		return -1;
	data++;
	move->from.y = strtol(data, (char**) &data, 10);
	while (isblank(*data))
		data++;

	move->to.x = strtol(data, (char**) &data, 10);
	if (*data != ',')
		return -1;
	data++;
	move->to.y = strtol(data, (char**) &data, 10);
	while (isblank(*data))
		data++;
	if (*data != '\0')
		return -1;
	return 0;
}

int hc_notifymove(void *ptr, const HiveMove *move)
{
	char *data;

	HiveChat *const hc = &hive_chat;

	(void) ptr;

	data = hc_serializemove(move);
	net_receiver_sendany(&hc->chat.net, -1, NET_REQUEST_HIVE_MOVE, data);
	return 0;
}

int hc_domove(void *ptr, const char *data)
{
	HiveMove move;

	HiveChat *const hc = &hive_chat;
	(void) ptr;
	if (hc_deserializemove(data, &move) < 0)
		return -1;
	hive_domove(&hc->hive, &move, false);
	return 0;
}
