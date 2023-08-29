#include "hive.h"

void *chat_setuser(void *arg)
{
	static const char *usage = "/setuser [name]\n";
	static const char *err1 = "that name is too long.\n";
	static const char *msg1 = "successfully changed your name!\n";

	struct chat_job *const job = (struct chat_job*) arg;
	struct chat *const chat = (struct chat*) job->chat;
	char *name;

	if (job->nargs != 1) {
		chat_syncwriteout(chat, usage, strlen(usage));
		return NULL;
	}
	name = job->args;
	if (strlen(name) >= sizeof(chat->name)) {
		chat_syncwriteout(chat, err1, strlen(err1));
		return NULL;
	}
	strcpy(chat->name, name);
	chat_syncwriteout(chat, msg1, strlen(msg1));

	return NULL;
}

void *chat_host(void *arg)
{
	static const char *usage = "/host [port]\n";
	static const char *err1 = "invalid argument. port must be an integer.\n";
	static const char *err2 = "failed creating socket.\n";
	static const char *err3 = "failed creating host server.\n";
	static const char *msg1 = "now hosting server!\n";
	static const char *msg2 = "connection closed!\n";

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
		chat_syncwriteout(chat, usage, strlen(usage));
		goto end;
	}

	if (!isdigit(*args)) {
		chat_syncwriteout(chat, err1, strlen(err1));
		goto end;
	}
	port = atoi(args);

	if ((server = net_socket()) < 0) {
		chat_syncwriteout(chat, err2, strlen(err2));
		goto end;
	}

	if (net_host(server, port) < 0) {
		chat_syncwriteout(chat, err3, strlen(err3));
		goto end;
	}
	chat_syncwriteout(chat, msg1, strlen(msg1));

	if (listen(server, 3) < 0)
		goto end;

	if ((socket = accept(server,
			(struct sockaddr*) &address,
			(socklen_t*) &addrlen)) < 0) {
		close(server);
		goto end;
	}

	chat->socket = socket;
	chat->flags |= CHAT_CONNECTED;
	while (1) {
		n = net_receive(socket, buf, sizeof(buf));
		if (n <= 0)
			break;
		chat_syncwriteout(chat, buf, n);
	}
	chat->flags &= ~CHAT_CONNECTED;
	close(socket);
	close(server);
	chat_syncwriteout(chat, msg2, strlen(msg2));
end:
	job->tid = 0;
	return NULL;
}

void *chat_join(void *arg)
{
	static const char *usage = "/join [ip address] [port]\n";
	static const char *err1 = "there already exists and open connection\n";
	static const char *err2 = "failed to connect to the server\n";
	static const char *msg1 = "trying to connect to the server...\n";
	static const char *msg2 = "connection established!\n";
	static const char *msg3 = "connection closed!\n";

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
		chat_syncwriteout(chat, usage, strlen(usage));
		goto end;
	}

	ip = args;
	args += strlen(args) + 1;

	if (!isdigit(*args)) {
		chat_syncwriteout(chat, usage, strlen(usage));
		goto end;
	}
	port = atoi(args);

	if (chat->flags & CHAT_CONNECTED) {
		chat_syncwriteout(chat, err1, strlen(err1));
		goto end;
	}

	if ((socket = net_socket()) < 0)
		goto end;

	chat_syncwriteout(chat, msg1, strlen(msg1));
	if (net_connect(socket, ip, port) < 0) {
		chat_syncwriteout(chat, err2, strlen(err2));
		goto end;
	}
	chat_syncwriteout(chat, msg2, strlen(msg2));

	chat->socket = socket;
	chat->flags |= CHAT_CONNECTED;
	while (1) {
		n = net_receive(socket, buf, sizeof(buf));
		if (n <= 0)
			break;
		chat_syncwriteout(chat, buf, n);
	}
	chat->flags &= ~CHAT_CONNECTED;
	close(socket);
	chat_syncwriteout(chat, msg3, strlen(msg3));
end:
	job->tid = 0;
	return NULL;
}

void *chat_leave(void *arg)
{
	static const char *usage = "/leave\n";

	struct chat_job *const job = (struct chat_job*) arg;
	struct chat *const chat = (struct chat*) job->chat;

	if (job->nargs != 0) {
		chat_syncwriteout(chat, usage, strlen(usage));
		goto end;
	}
	close(chat->socket);
end:
	job->tid = 0;
	return NULL;
}

int chat_exec(struct chat *chat)
{
	static const struct {
		const char *name;
		void *(*proc)(void *arg);
		bool isAsync;
	} *cmd, commands[] = {
		{ "setuser", chat_setuser, false },
		{ "join", chat_join, true },
		{ "host", chat_host, true },
		{ "leave", chat_leave, false },
	};
	static const char *err1 = "command does not exist\n";
	static const char *err2 = "too many jobs are running already\n";

	size_t i, s, n;
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
		chat_syncwriteout(chat, err1, strlen(err1));
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
			chat_syncwriteout(chat, err2, strlen(err2));
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
