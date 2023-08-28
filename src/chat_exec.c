#include "hive.h"

int chat_join(struct chat *chat, char *args, int nArgs)
{
	static const char *err1 = "/join [ip address] [port]\n";

	if (nArgs != 2) {
		chat_writeout(chat, err1, strlen(err1));
		return -1;
	}

	return 0;
}

int chat_exec(struct chat *chat)
{
	static const struct {
		const char *name;
		int (*proc)(struct chat *chat, char *args, int nArgs);
	} *cmd, commands[] = {
		{ "join", chat_join },
	};
	static const char *err1 = "command does not exist\n";

	size_t i, s, n;
	int nArgs;
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
		chat_writeout(chat, err1, strlen(err1));
		goto end;
	}

	/* parse arguments by setting null terminators at the correct places */
	s = 0;
	nArgs = 0;
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
		nArgs++;
		if (i == chat->nIn)
			break;
		i++;
		s = i;
	}
	exitCode = cmd->proc(chat, chat->in, nArgs);
end:
	chat->nIn = 0;
	chat->iIn = 0;
	chat->flags |= CHAT_IN_CHANGED;
	return exitCode;
}
