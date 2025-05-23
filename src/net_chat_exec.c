#include "hex.h"

static void *net_chat_help(void *arg);
static void *net_chat_clear(void *arg);
static void *net_chat_setname(void *arg);
static void *net_chat_host(void *arg);
static void *net_chat_join(void *arg);
static void *net_chat_leave(void *arg);
static void *net_chat_challenge(void *arg);

static const struct chat_cmd {
	const char *name;
	const char *args;
	const char *description;
	void *(*proc)(void *arg);
	bool isAsync;
} all_commands[] = {
	{ "help", "", "show this help", net_chat_help, false },
	{ "clear", "", "clear the chat window", net_chat_clear, false },
	{ "setname", "[name]", "set your user name or the server name", net_chat_setname, true },
	{ "host", "[port]", "start a server", net_chat_host, true },
	{ "join", "[ip/domain] [port]", "join a server", net_chat_join, true },
	{ "leave", "", "leave the current network", net_chat_leave, true },
	{ "challenge", "", "make a challenge or accept a challenge", net_chat_challenge, true },
};

static bool net_chat_iscorrectargs(NetChat *chat,
		const struct chat_cmd *cmd, const NetChatJob *job)
{
	int neededArgs;
	const char *args, *end;
	const char *cur;

	args = cmd->args;
	neededArgs = 0;
	while (*args != '\0') {
		end = strchr(args, ' ');
		if (end == NULL)
			end = args + strlen(args);
		else
			end++;
		neededArgs++;
		args = end;
	}
	if (neededArgs != job->numArgs)
		return false;
	args = cmd->args;
	cur = job->args;
	while (*args != '\0') {
		end = strchr(args, ' ');
		if (end == NULL)
			end = args + strlen(args);
		else
			end++;
		if (!memcmp(args, "[name]", sizeof("[name]") - 1)) {
			if (!net_isvalidname(cur)) {
				pthread_mutex_lock(&chat->output.lock);
				wattr_set(chat->output.win, 0, PAIR_ERROR,
						NULL);
				wprintw(chat->output.win, "Invalid name '%s'."
					" Only use letters and numbers and "
					"between 3 and 31 characters\n", cur);
				pthread_mutex_unlock(&chat->output.lock);
				return false;
			}
		}
		cur += strlen(cur) + 1;
		args = end;
	}
	return true;
}

static void *net_chat_help(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	WINDOW *const win = chat->output.win;

	pthread_mutex_lock(&chat->output.lock);
	wattr_set(win, 0, PAIR_NORMAL, NULL);
	waddstr(win, "This is an implementation of a chat service and the great board game hive, you can find the rules to the game on: https://www.gen42.com/download/rules/hive/Hive_English_Rules.pdf or navigate to them on the official website: https://www.gen42.com\n\n");
	waddstr(win, "You can use commands by typing '/[command name] [arguments]'\n");
	waddstr(win, "These are commands you can use inside this chat window, there are all commands:\n");
	for (size_t i = 0; i < ARRLEN(all_commands); i++) {
		const char *args;
		const char *end;

		waddch(win, '\t');
		wattr_set(win, 0, PAIR_COMMAND, NULL);
		waddstr(win, all_commands[i].name);
		args = all_commands[i].args;
		wattr_set(win, 0, PAIR_ARGUMENT, NULL);
		if (*args != '\0')
			waddch(win, ' ');
		while (*args != '\0') {
			end = strchr(args, ' ');
			if (end == NULL)
				end = args + strlen(args);
			else
				end++;
			waddnstr(win, args, end - args);
			args = end;
		}
		wattr_set(win, 0, PAIR_NORMAL, NULL);
		wprintw(win, " - %s\n", all_commands[i].description);
	}
	pthread_mutex_unlock(&chat->output.lock);
	return NULL;
}

static void *net_chat_clear(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	pthread_mutex_lock(&chat->output.lock);
	werase(chat->output.win);
	pthread_mutex_unlock(&chat->output.lock);
	return NULL;
}

static void *net_chat_setname(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	char *name;

	name = job->args;
	if (chat->net.socket > 0)
		net_receiver_sendany(&chat->net, 0, NET_REQUEST_SUN, name);
	strcpy(chat->name, name);
	job->threadId = 0;
	return NULL;
}

static void *net_chat_host(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	const char *args;
	const char *name;
	int port;
	int sock;
	struct sockaddr_in addr;
	NetRequest req;
	struct net_entry *ent;

	args = job->args;
	if (chat->net.socket > 0) {
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		wprintw(chat->output.win, "You are already part of a network.\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	name = args;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
			net_receiver_init(&chat->net, sock, true) < 0) {
		close(sock);
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		wprintw(chat->output.win, "Unable to create server: %s\n", strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	port = strtol(name, NULL, 10);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		net_receiver_uninit(&chat->net);
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		wprintw(chat->output.win,
				"Unable to bind server %s: %s\n",
				name, strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}

	if (listen(sock, SOMAXCONN) < 0) {
		net_receiver_uninit(&chat->net);
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		wprintw(chat->output.win,
			"Unable set the socket to listen: %s\n",
			strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}

	pthread_mutex_lock(&chat->output.lock);
	wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
	wprintw(chat->output.win, "Now hosting server '%s' at %d!\n",
		name, port);
	pthread_mutex_unlock(&chat->output.lock);
	while (net_receiver_nextrequest(&chat->net, &ent, &req)) {
		switch (req.type) {
		case NET_REQUEST_NONE:
			break;
		case NET_REQUEST_JIN:
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
			wprintw(chat->output.win, "User 'Anon' has joined!\n");
			pthread_mutex_unlock(&chat->output.lock);
			/* send all moves to anon */
			hc_sendmoves(chat, ent->socket);
			break;
		case NET_REQUEST_LVE:
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
			wprintw(chat->output.win, "User '%s' has left!\n",
					ent->name);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_KCK:
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
			wprintw(chat->output.win,
				"User '%s' was kicked!\n",
				ent->name);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_MSG:
			net_receiver_send(&chat->net, &req);
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_COMMAND, NULL);
			wprintw(chat->output.win, "%s> ", ent->name);
			wattr_set(chat->output.win, 0, PAIR_NORMAL, NULL);
			wprintw(chat->output.win, "%s\n", req.extra);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_SUN:
			if (!strcmp(ent->name, req.name))
				break;
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
			wprintw(chat->output.win,
					"User '%s' set their name to: '%s'\n",
					ent->name, req.name);
			strcpy(ent->name, req.name);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_HIVE_CHALLENGE:
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
			wprintw(chat->output.win,
					"Got challenge from: %s (%d, %d).\n",
					ent->name, chat->players[0].socket,
					chat->players[1].socket);
			pthread_mutex_unlock(&chat->output.lock);
			if (chat->players[0].socket == 0 ||
					net_receiver_indexof(&chat->net,
					chat->players[0].socket) == (nfds_t) -1) {
				chat->players[0].socket = ent->socket;
				strcpy(chat->players[0].name, ent->name);
				net_receiver_sendformatted(&chat->net, 0,
					NET_REQUEST_SRV,
					"User '%s' has issued a challenge.\n"
					"Type '/challenge' to accept!\n",
					ent->name);
			} else if (chat->players[1].socket == 0) {
				net_receiver_sendany(&chat->net, 0,
						NET_REQUEST_HIVE_RESET);
				chat->players[1].socket = ent->socket;
				strcpy(chat->players[1].name, ent->name);
				net_receiver_sendformatted(&chat->net, 0,
					NET_REQUEST_SRV,
					"User '%s' has accepted the challenge.\n",
					ent->name);
			} else {
				net_receiver_sendformatted(&chat->net,
					ent->socket, NET_REQUEST_SRV,
					"There already is an active game.\n",
					ent->name);
			}
			break;
		case NET_REQUEST_HIVE_MOVE:
			if (ent->socket == chat->players[0].socket) {
				hc_domove(chat, req.extra);
				net_receiver_sendany(&chat->net, 0,
						NET_REQUEST_HIVE_MOVE,
						req.extra);
			} else if (ent->socket == chat->players[1].socket) {
				hc_domove(chat, req.extra);
				net_receiver_sendany(&chat->net, 0,
						NET_REQUEST_HIVE_MOVE,
						req.extra);
			}
			break;
		default:
			break;
		}
	}

end:
	job->threadId = 0;
	return NULL;
}

static void *net_chat_join(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	const char *ip, *strPort;
	int port;
	int sock;
	struct sockaddr_in addr;
	NetRequest req;
	struct net_entry *ent;

	struct addrinfo hints, *result;
	struct sockaddr_in *resolvedAddr;

	if (chat->net.socket > 0) {
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		waddstr(chat->output.win, "You are already part of a network.\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	ip = job->args;
	strPort = ip + strlen(ip) + 1;
	port = strtol(strPort, NULL, 10);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
			net_receiver_init(&chat->net, sock, false) < 0) {
		close(sock);
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		/* TODO: maybe change error message */
		wprintw(chat->output.win, "Unable to create socket: %s\n", strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	pthread_mutex_lock(&chat->output.lock);
	wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
	wprintw(chat->output.win, "Trying to connect to %s:%d...\n", ip, port);
	pthread_mutex_unlock(&chat->output.lock);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	const int status = getaddrinfo(ip, NULL, &hints, &result);
	if (status != 0) {
		if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
			const int err = errno;
			net_receiver_uninit(&chat->net);
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
			wprintw(chat->output.win, "Unable to read address '%s': %s\n",
					ip, err == 0 ? "invalid format" : strerror(err));
			pthread_mutex_unlock(&chat->output.lock);
			goto end;
		}
	} else {
		resolvedAddr = (struct sockaddr_in*) result->ai_addr;
		addr.sin_addr = resolvedAddr->sin_addr;
		freeaddrinfo(result);
	}

	if (connect(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		net_receiver_uninit(&chat->net);
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		wprintw(chat->output.win,
				"Unable to connect to the server %s:%d: %s\n",
				ip, port, strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	pthread_mutex_lock(&chat->output.lock);
	wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
	wprintw(chat->output.win, "Joined server %s:%d!\n", ip, port);
	pthread_mutex_unlock(&chat->output.lock);

	net_receiver_sendany(&chat->net, 0, NET_REQUEST_SUN, chat->name);
	while (net_receiver_nextrequest(&chat->net, &ent, &req)) {
		switch (req.type) {
		case NET_REQUEST_MSG:
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_COMMAND, NULL);
			wprintw(chat->output.win, "%s> ", req.name);
			wattr_set(chat->output.win, 0, PAIR_NORMAL, NULL);
			wprintw(chat->output.win, "%s\n", req.extra);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_SRV:
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
			wprintw(chat->output.win, "Server> %s\n", req.extra);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_HIVE_MOVE:
			hc_domove(chat, req.extra);
			break;
		case NET_REQUEST_HIVE_RESET:
			hc_notifygamestart(chat);
			break;
		default:
			/* just ignore the request */
			break;
		}
	}

end:
	job->threadId = 0;
	return NULL;
}

static void *net_chat_leave(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	net_receiver_uninit(&chat->net);
	job->threadId = 0;
	return NULL;
}

static void *net_chat_challenge(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	if (chat->net.socket == 0) {
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		waddstr(chat->output.win, "You are not part of a network.\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	net_receiver_sendany(&chat->net, 0, NET_REQUEST_HIVE_CHALLENGE);

end:
	job->threadId = 0;
	return NULL;
}

int net_chat_exec(NetChat *chat)
{
	size_t i, s, n;
	const struct chat_cmd *cmd;
	NetChatJob *job;
	int exitCode;

	for (i = 0; i < chat->input.length; i++)
		if(!isspace(chat->input.buffer[i]))
			break;
	if (chat->input.buffer[i] != '/') {
		exitCode = -1;
		goto end;
	}
	i++;

	/* get command name */
	s = i;
	while (isalnum(chat->input.buffer[i]) && i != chat->input.length)
		i++;
	if (s == i) {
		exitCode = -1;
		goto end;
	}
	n = i - s;

	/* try to find the command */
	cmd = NULL;
	for (size_t c = 0; c < ARRLEN(all_commands); c++)
		if (!strncmp(all_commands[c].name, chat->input.buffer + s, n)
				&& all_commands[c].name[n] == '\0') {
			cmd = all_commands + c;
			break;
		}
	if (cmd == NULL) {
		exitCode = -1;
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		wprintw(chat->output.win, "Command '%.*s' does not exist.\n",
			(int) n, chat->input.buffer + s);
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}

	/* find an available job id */
	if (!cmd->isAsync) {
		job = &chat->syncJob;
	} else {
		job = NULL;
		for (size_t j = 0; j < ARRLEN(chat->jobs); j++)
			if (chat->jobs[j].threadId == 0) {
				job = chat->jobs + j;
				break;
			}
		if (job == NULL) {
			exitCode = -1;
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
			waddstr(chat->output.win,
				"Too many jobs are already running!\n");
			pthread_mutex_unlock(&chat->output.lock);
			goto end;
		}
	}

	/* parse arguments by setting null terminators at the correct places */
	s = 0;
	job->numArgs = 0;
	while(1) {
		while (isblank(chat->input.buffer[i]) && i != chat->input.length)
			i++;
		if (i == chat->input.length)
			break;
		n = i - s;
		chat->input.length -= n;
		memmove(chat->input.buffer + s,
			chat->input.buffer + s + n,
			chat->input.length - s);
		i = s;
		while (!isblank(chat->input.buffer[i]) && i != chat->input.length)
			i++;
		chat->input.buffer[i] = '\0';
		job->numArgs++;
		if (i == chat->input.length) {
			chat->input.length++;
			break;
		}
		i++;
		s = i;
	}

	/* create a job (in the background if isAsync) */
	char *const newArgs = realloc(job->args, chat->input.length);
	if (newArgs == NULL) {
		exitCode = -1;
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
		waddstr(chat->output.win, "Allocation error occurred.\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	job->args = newArgs;
	memcpy(job->args, chat->input.buffer, chat->input.length);
	if (!net_chat_iscorrectargs(chat, cmd, job)) {
		exitCode = -1;
		pthread_mutex_lock(&chat->output.lock);
		wattr_set(chat->output.win, 0, PAIR_NORMAL, NULL);
		waddstr(chat->output.win, "usage: ");
		wattr_set(chat->output.win, 0, PAIR_COMMAND, NULL);
		wprintw(chat->output.win, "/%s ", cmd->name);
		wattr_set(chat->output.win, 0, PAIR_ARGUMENT, NULL);
		wprintw(chat->output.win, "%s\n", cmd->args);
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	if (cmd->isAsync)
		pthread_create(&job->threadId, NULL, cmd->proc, job);
	else
		cmd->proc(job);

end:
	chat->input.length = 0;
	chat->input.index = 0;
	return exitCode;
}
