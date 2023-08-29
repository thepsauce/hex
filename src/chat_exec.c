#include "hive.h"

void *chat_setuser(void *arg)
{
	static const char *usage = "/setuser [name]\n";

	struct chat_job *const job = (struct chat_job*) arg;
	struct chat *const chat = (struct chat*) job->chat;
	char *name;

	if (job->nargs != 1) {
		chat_writeout(chat, usage, strlen(usage));
		return NULL;
	}
	name = job->args;
	if (strlen(name) >= sizeof(chat->name)) {
		chat_printf(chat, "That name is too long.\n");
		return NULL;
	}
	strcpy(chat->name, name);
	chat_printf(chat, "Successfully changed your name to '%s'\n", name);
	return NULL;
}

void *chat_host(void *arg)
{
	static const char *usage = "/host [port]\n";

	struct chat_job *const job = (struct chat_job*) arg;
	struct chat *const chat = (struct chat*) job->chat;
	char *args;
	int port;
	int server, socket;
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);
	char buf[1024];
	ssize_t n;

	args = job->args;
	if (job->nargs != 1) {
		chat_writeout(chat, usage, strlen(usage));
		goto end;
	}

	if (!isdigit(*args)) {
		chat_printf(chat, "Invalid port. Must be an integer\n");
		goto end;
	}
	port = atoi(args);

	if (chat->socket >= 0) {
		chat_printf(chat, "There already is an active socket.\n");
		goto end;
	}

	if ((server = net_socket()) < 0) {
		chat_printf(chat, "Failed creating socket: %s\n",
			strerror(errno));
		goto end;
	}

	if (net_host(server, port) < 0) {
		chat_printf(chat, "Failed binding socket: %s\n",
			strerror(errno));
		goto end;
	}
	if (listen(server, 3) < 0) {
		chat_printf(chat, "Failed marking socket: %s\n",
			strerror(errno));
		goto end;
	}

	chat_printf(chat, "Successfully started host server at port %d!\n",
		port);

	chat->socket = server;
next:
	if ((socket = accept(server,
			(struct sockaddr*) &address,
			(socklen_t*) &addrlen)) < 0) {
		chat_printf(chat, "Failed accepting socket: %s\n",
			strerror(errno));
		if (chat->socket >= 0)
			close(server);
		chat->socket = -1;
		goto end;
	}

	chat_printf(chat, "Established a connection!\n");
	chat->sender = socket;
	while (1) {
		n = net_receive(socket, buf, sizeof(buf));
		if (n < 0) {
			chat_printf(chat, "Failed receiving: %s\n",
				strerror(errno));
			close(socket);
			break;
		}
		if (n == 0) {
			chat_printf(chat, "Closed connection!\n");
			goto next;
		}
		chat_writeout(chat, buf, n);
	}
	chat->sender = -1;
	if (chat->socket >= 0)
		close(server);
	chat->socket = -1;

end:
	job->tid = 0;
	return NULL;
}

void *chat_join(void *arg)
{
	static const char *usage = "/join [ip address] [port]\n";

	struct chat_job *const job = (struct chat_job*) arg;
	struct chat *const chat = (struct chat*) job->chat;
	char *args;
	int socket;
	const char *ip;
	int port;
	char buf[1024];
	ssize_t n;

	args = job->args;
	if (job->nargs != 2) {
		chat_writeout(chat, usage, strlen(usage));
		goto end;
	}

	ip = args;
	args += strlen(args) + 1;

	if (!isdigit(*args)) {
		chat_printf(chat, "Invalid port. Must be an integer.\n");
		goto end;
	}
	port = atoi(args);

	if (chat->socket >= 0) {
		chat_printf(chat, "There already is an active socket.\n");
		goto end;
	}

	if ((socket = net_socket()) < 0) {
		chat_printf(chat, "Failed creating socket: %s\n",
			strerror(errno));
		goto end;
	}

	chat->socket = socket;
	chat_printf(chat, "Trying to connect to %s:%d...\n", ip, port);
	if (net_connect(socket, ip, port) < 0) {
		chat_printf(chat, "Failed connecting: %s\n", strerror(errno));
		if (chat->socket >= 0)
			close(socket);
		chat->socket = -1;
		goto end;
	}
	chat_printf(chat, "Established a connection to %s:%d!\n", ip, port);

	chat->sender = socket;
	while (1) {
		n = net_receive(socket, buf, sizeof(buf));
		if (n < 0) {
			chat_printf(chat, "Failed receiving: %s\n",
				strerror(errno));
			break;
		}
		if (n == 0) {
			chat_printf(chat, "Closed connection!\n");
			break;
		}
		chat_writeout(chat, buf, n);
	}
	chat->sender = -1;
	close(socket);
	chat->socket = -1;

end:
	job->tid = 0;
	return NULL;
}

/* FIXME: chat_leave() is bugged, it's not correctly leaving when
 * a host calls it while being connected to a client
 */
void *chat_leave(void *arg)
{
	static const char *usage = "/leave\n";
	int socket;

	struct chat_job *const job = (struct chat_job*) arg;
	struct chat *const chat = (struct chat*) job->chat;

	if (job->nargs != 0) {
		chat_writeout(chat, usage, strlen(usage));
		goto end;
	}
	if (chat->socket < 0) {
		chat_printf(chat, "No active connection.\n");
		goto end;
	}
	socket = chat->socket;
	chat->socket = -1;
	shutdown(socket, SHUT_RDWR);
	close(socket);

end:
	job->tid = 0;
	return NULL;
}

int chat_exec(struct chat *chat)
{
	static const struct cmd {
		const char *name;
		void *(*proc)(void *arg);
		bool isAsync;
	} commands[] = {
		{ "setuser", chat_setuser, false },
		{ "join", chat_join, true },
		{ "host", chat_host, true },
		{ "leave", chat_leave, true },
	};

	size_t i, s, n;
	const struct cmd *cmd;
	struct chat_job *job;
	int exitCode;

	for (i = 0; i < chat->nIn; i++)
		if(!isspace(chat->in[i]))
			break;
	if (chat->in[i] != '/') {
		exitCode = -1;
		goto end;
	}
	i++;

	/* get command name */
	s = i;
	while (isalnum(chat->in[i]) && i != chat->nIn)
		i++;
	if (s == i) {
		exitCode = -1;
		goto end;
	}
	n = i - s;

	/* try to find the command */
	cmd = NULL;
	for (size_t c = 0; c < ARRLEN(commands); c++)
		if (!strncmp(commands[c].name, chat->in + s, n)
				&& commands[c].name[n] == '\0') {
			cmd = commands + c;
			break;
		}
	if (cmd == NULL) {
		exitCode = -1;
		chat_printf(chat, "Command '%.*s' does not exist\n",
			(int) n, chat->in + s);
		goto end;
	}

	/* find an available job id */
	if (!cmd->isAsync) {
		job = &chat->syncJob;
	} else {
		job = NULL;
		for (size_t j = 0; j < ARRLEN(chat->jobs); j++)
			if (chat->jobs[j].tid == 0) {
				job = chat->jobs + j;
				break;
			}
		if (job == NULL) {
			exitCode = -1;
			chat_printf(chat,
				"Too many jobs are already running!\n");
			goto end;
		}
	}

	/* parse arguments by setting null terminators at the correct places */
	s = 0;
	job->nargs = 0;
	while(1) {
		while (isblank(chat->in[i]) && i != chat->nIn)
			i++;
		if (i == chat->nIn)
			break;
		n = i - s;
		chat->nIn -= n;
		memmove(chat->in + s, chat->in + s + n, chat->nIn - s);
		i = s;
		while (!isblank(chat->in[i]) && i != chat->nIn)
			i++;
		chat->in[i] = '\0';
		job->nargs++;
		if (i == chat->nIn) {
			chat->nIn++;
			break;
		}
		i++;
		s = i;
	}

	/* create a job (in the background if isAsync) */
	job->args = realloc(job->args, chat->nIn);
	memcpy(job->args, chat->in, chat->nIn);
	if (cmd->isAsync)
		pthread_create(&job->tid, NULL, cmd->proc, job);
	else
		cmd->proc(job);

end:
	chat->nIn = 0;
	chat->iIn = 0;
	chat->flags |= CHAT_IN_CHANGED;
	return exitCode;
}
