#include "hex.h"

void *net_chat_setname(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	char *name;

	if (job->numArgs != 1) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_NORMAL);
		waddstr(chat->output.win, "usage: ");
		wattrset(chat->output.win, ATTR_COMMAND);
		waddstr(chat->output.win, "/setname ");
		wattrset(chat->output.win, ATTR_ARGUMENT);
		waddstr(chat->output.win, "[name]\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	name = job->args;
	if (!net_isvalidname(name)) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win, "Invalid name '%s'."
			"Only use letters and numbers and "
			"between 3 and 31 characters\n", name);
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}

	if (chat->net.socket > 0)
		net_receiver_sendany(&chat->net, 0, NET_REQUEST_SUN, name);
	strcpy(chat->name, name);

end:
	job->threadId = 0;
	return NULL;
}

void *net_chat_host(void *arg)
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

	if (job->numArgs != 1) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_NORMAL);
		waddstr(chat->output.win, "usage: ");
		wattrset(chat->output.win, ATTR_COMMAND);
		waddstr(chat->output.win, "/host ");
		wattrset(chat->output.win, ATTR_ARGUMENT);
		waddstr(chat->output.win, "[name]\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	args = job->args;

	if (chat->net.socket > 0) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win, "You are already part of a network.\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	name = args;
	if (!net_isvalidname(name)) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win, "Invalid name '%s'."
			"Only use letters and numbers and "
			"between 3 and 31 characters\n", name);
		pthread_mutex_unlock(&chat->output.lock);
	}

	strcpy(chat->name, name);
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
			net_receiver_init(&chat->net, sock, true) < 0) {
		close(sock);
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win, "Unable to create net: %s\n", strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	port = net_porthash(name);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		net_receiver_uninit(&chat->net);
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win,
				"Unable to bind net %s: %s\n",
				name, strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	if (listen(sock, SOMAXCONN) < 0) {
		net_receiver_uninit(&chat->net);
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win,
			"Unable set the socket to listen: %s\n",
			strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	pthread_mutex_lock(&chat->output.lock);
	wattrset(chat->output.win, ATTR_INFO);
	wprintw(chat->output.win, "Now hosting net '%s' at %d!\n",
		name, port);
	pthread_mutex_unlock(&chat->output.lock);
	while (net_receiver_nextrequest(&chat->net, &ent, &req)) {
		switch (req.type) {
		case NET_REQUEST_NONE:
			break;
		case NET_REQUEST_JIN:
			pthread_mutex_lock(&chat->output.lock);
			wattrset(chat->output.win, ATTR_INFO);
			wprintw(chat->output.win, "User 'Anon' has joined!\n");
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_LVE:
			pthread_mutex_lock(&chat->output.lock);
			wattrset(chat->output.win, ATTR_INFO);
			wprintw(chat->output.win, "User '%s' has left!\n",
					ent->name);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_KCK:
			pthread_mutex_lock(&chat->output.lock);
			wattrset(chat->output.win, ATTR_INFO);
			wprintw(chat->output.win,
				"User '%s' was kicked!\n",
				ent->name);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_MSG:
			net_receiver_send(&chat->net, &req);
			pthread_mutex_lock(&chat->output.lock);
			wattrset(chat->output.win, ATTR_COMMAND);
			wprintw(chat->output.win, "%s> ", ent->name);
			wattrset(chat->output.win, ATTR_NORMAL);
			wprintw(chat->output.win, "%s\n", req.extra);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_SUN:
			if (!strcmp(ent->name, req.name))
				break;
			pthread_mutex_lock(&chat->output.lock);
			wattrset(chat->output.win, ATTR_INFO);
			wprintw(chat->output.win,
					"User '%s' set their name to: '%s'\n",
					ent->name, req.name);
			strcpy(ent->name, req.name);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_HIVE_CHALLENGE:
			if (chat->blackPlayer == 0 ||
					net_receiver_indexof(&chat->net,
					chat->blackPlayer) == (nfds_t) -1) {
				chat->blackPlayer = ent->socket;
				net_receiver_sendformatted(&chat->net, 0,
					NET_REQUEST_SRV,
					"User '%s' has issued a challenge.\n"
					"Type '/challenge' to accept!\n",
					ent->name);
			} else if (chat->whitePlayer == 0) {
				chat->whitePlayer = ent->socket;
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
			if (ent->socket == chat->blackPlayer ||
					ent->socket == chat->whitePlayer)
				hc_makemove(req.extra);
			break;
		default:
			break;
		}
	}

end:
	job->threadId = 0;
	return NULL;
}

void *net_chat_join(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;
	const char *ip, *name;
	int port;
	int sock;
	struct sockaddr_in addr;
	NetRequest req;
	struct net_entry *ent;

	if (job->numArgs != 2) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_NORMAL);
		waddstr(chat->output.win, "usage: ");
		wattrset(chat->output.win, ATTR_COMMAND);
		waddstr(chat->output.win, "/join ");
		wattrset(chat->output.win, ATTR_ARGUMENT);
		waddstr(chat->output.win, "[ip] [name]\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	if (chat->net.socket > 0) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		waddstr(chat->output.win, "You are already part of a network.\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	ip = job->args;
	name = ip + strlen(ip) + 1;
	port = net_porthash(name);

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0 ||
			net_receiver_init(&chat->net, sock, false) < 0) {
		close(sock);
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		/* TODO: maybe change error message */
		wprintw(chat->output.win, "Unable to create socket: %s\n", strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	pthread_mutex_lock(&chat->output.lock);
	wattrset(chat->output.win, ATTR_INFO);
	wprintw(chat->output.win, "Trying to connect to %s:%d (%s)...\n", ip, port, name);
	pthread_mutex_unlock(&chat->output.lock);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
		net_receiver_uninit(&chat->net);
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win, "Unable to read ip address %s: %s\n",
				ip, strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	if (connect(sock, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
		net_receiver_uninit(&chat->net);
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win,
				"Unable to connect to the net %s:%d (%s): %s\n",
				ip, port, name, strerror(errno));
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	pthread_mutex_lock(&chat->output.lock);
	wattrset(chat->output.win, ATTR_INFO);
	wprintw(chat->output.win, "Joined net %s!\n", name);
	pthread_mutex_unlock(&chat->output.lock);

	net_receiver_sendany(&chat->net, 0, NET_REQUEST_SUN, chat->name);
	while (net_receiver_nextrequest(&chat->net, &ent, &req)) {
		switch (req.type) {
		case NET_REQUEST_MSG:
			pthread_mutex_lock(&chat->output.lock);
			wattrset(chat->output.win, ATTR_COMMAND);
			wprintw(chat->output.win, "%s> ", req.name);
			wattrset(chat->output.win, ATTR_NORMAL);
			wprintw(chat->output.win, "%s\n", req.extra);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_SRV:
			pthread_mutex_lock(&chat->output.lock);
			wattrset(chat->output.win, ATTR_INFO);
			wprintw(chat->output.win, "Server> %s\n", req.extra);
			pthread_mutex_unlock(&chat->output.lock);
			break;
		case NET_REQUEST_HIVE_MOVE:
			hc_makemove(req.extra);
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

void *net_chat_leave(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;

	if (job->numArgs != 0) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_NORMAL);
		waddstr(chat->output.win, "usage: ");
		wattrset(chat->output.win, ATTR_COMMAND);
		waddstr(chat->output.win, "/leave\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	net_receiver_uninit(&chat->net);

end:
	job->threadId = 0;
	return NULL;
}

static void *net_chat_challenge(void *arg)
{
	NetChatJob *const job = (NetChatJob*) arg;
	NetChat *const chat = (NetChat*) job->chat;

	if (job->numArgs != 0) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_NORMAL);
		waddstr(chat->output.win, "usage: ");
		wattrset(chat->output.win, ATTR_COMMAND);
		waddstr(chat->output.win, "/challenge\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	if (chat->net.socket == 0) {
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
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
	static const struct cmd {
		const char *name;
		void *(*proc)(void *arg);
		bool isAsync;
	} commands[] = {
		{ "setname", net_chat_setname, true },
		{ "host", net_chat_host, true },
		{ "join", net_chat_join, true },
		{ "leave", net_chat_leave, true },
		{ "challenge", net_chat_challenge, true },
	};

	size_t i, s, n;
	const struct cmd *cmd;
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
	for (size_t c = 0; c < ARRLEN(commands); c++)
		if (!strncmp(commands[c].name, chat->input.buffer + s, n)
				&& commands[c].name[n] == '\0') {
			cmd = commands + c;
			break;
		}
	if (cmd == NULL) {
		exitCode = -1;
		pthread_mutex_lock(&chat->output.lock);
		wattrset(chat->output.win, ATTR_ERROR);
		wprintw(chat->output.win, "Command '%.*s' does not exist\n",
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
			wattrset(chat->output.win, ATTR_ERROR);
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
		wattrset(chat->output.win, ATTR_ERROR);
		waddstr(chat->output.win, "Allocation error occurred.\n");
		pthread_mutex_unlock(&chat->output.lock);
		goto end;
	}
	job->args = newArgs;
	memcpy(job->args, chat->input.buffer, chat->input.length);
	if (cmd->isAsync)
		pthread_create(&job->threadId, NULL, cmd->proc, job);
	else
		cmd->proc(job);

end:
	chat->input.length = 0;
	chat->input.index = 0;
	return exitCode;
}
