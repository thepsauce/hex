#include "hex.h"

void hc_init(HiveChat *hc)
{
	memset(hc, 0, sizeof(*hc));
	hc->status = newwin(1, COLS, LINES - 1, 0);
	wbkgdset(hc->status, COLOR_PAIR(PAIR_STATUS_INFO));
	if (hc->status == NULL ||
			hive_init(&hc->hive, 0, 0,
				COLS / 2 - 1, LINES - 1) < 0 ||
			net_chat_init(&hc->chat, COLS / 2, 0,
				COLS - COLS / 2, LINES - 1, 10000) < 0) {
		endwin();
		fprintf(stderr, "failed initializing\n");
		exit(-1);
	}
}

void hc_setposition(HiveChat *hc, int x, int y, int w, int h)
{
	hive_setposition(&hc->hive, x, y, w / 2 - 1, h - 1);
	net_chat_setposition(&hc->chat, x + w / 2, y, w - w / 2, h - 1);
}

void hc_renderstatus(HiveChat *hc)
{
	WINDOW *const win = hc->status;
	Hive *const hive = &hc->hive;
	NetChat *const chat = &hc->chat;
	wattr_set(win, 0, PAIR_STATUS_INFO, NULL);
	wmove(win, 0, 0);
	if (hc->inSync)
		wprintw(win, "Synced with a server. ");
	wprintw(win, "%zu moves played. ", hive->history.count);
	if (chat->net.socket > 0)
		wprintw(win, chat->net.isServer ? "Hosting server: '%s'" :
				"Username: '%s'", chat->name);
	wclrtoeol(win);
	wnoutrefresh(win);
}

bool hc_hasconnection(void *ptr)
{
	(void) ptr;
	HiveChat *const hc = &hive_chat;
	return hc->chat.net.socket > 0;
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
		data += sizeof("true");
	} else if (strncmp(data, "false ", sizeof("false")) == 0) {
		move->fromInventory = false;
		data += sizeof("false");
	} else {
		return -1;
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

void hc_notifygamestart(void *ptr)
{
	(void) ptr;
	HiveChat *const hc = &hive_chat;
	hive_reset(&hc->hive);
}

int hc_notifymove(void *ptr, const HiveMove *move)
{
	char *data;

	(void) ptr;
	HiveChat *const hc = &hive_chat;
	data = hc_serializemove(move);
	net_receiver_sendany(&hc->chat.net, 0, NET_REQUEST_HIVE_MOVE, data);
	if (hc->chat.net.isServer)
		hive_domove(&hc->hive, move, false);
	return 0;
}

int hc_sendmoves(void *ptr, int socket)
{
	char *data;

	(void) ptr;
	HiveChat *const hc = &hive_chat;
	Hive *const hive = &hc->hive;
	for (size_t i = 0; i < hive->history.count; i++) {
		data = hc_serializemove(&hive->history.moves[i]);
		net_receiver_sendany(&hc->chat.net, socket,
				NET_REQUEST_HIVE_MOVE, data);
	}
	return 0;
}

bool hc_isplayer(void *ptr, int player)
{
	(void) ptr;
	HiveChat *const hc = &hive_chat;
	return (player == 0 && hc->hive.turn == HIVE_WHITE) ||
		(player == 1 && hc->hive.turn == HIVE_BLACK);
}

int hc_domove(void *ptr, const char *data)
{
	HiveMove move;

	(void) ptr;
	HiveChat *const hc = &hive_chat;
	NetChat *const chat = &hc->chat;
	if (hc_deserializemove(data, &move) < 0)
		return -1;
	hive_domove(&hc->hive, &move, false);
	if (hive_isqueensurrounded(&hc->hive)) {
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
		waddstr(chat->output.win,
				hc->hive.turn == HIVE_WHITE ?
					"Server> White wins!\n" :
					"Server> Black wins!\n");
		pthread_mutex_unlock(&chat->output.lock);
	}
	return 0;
}
