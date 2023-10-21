/* this is the interface to connect chat and hive, hc stands for hive-chat */

typedef struct hc {
	/* pad window to keep the status bar */
	WINDOW *status;
	/* this is false if the board and the server
	 * are not in sync, this is also false when
	 * there is no server to send moves to
	 */
	bool inSync;
	Hive hive;
	NetChat chat;
} HiveChat;

void hc_init(HiveChat *hc);
void hc_setposition(HiveChat *hc, int x, int y, int w, int h);
void hc_renderstatus(HiveChat *hc);

extern HiveChat hive_chat;

/* the aditional pointer parameter is to identify the specific
 * hive-chat connection but since there is just one, this pointer
 * is unused
 */
bool hc_hasconnection(void *ptr);
int hc_sendmoves(void *ptr, int socket);
/* a move is in the simple format:
 * [0|1] [x position],[y position] [x position],[y position] 
 */
/* send a notification to the server */
int hc_notifymove(void *ptr, const HiveMove *move);
bool hc_isplayer(void *ptr, int player);
/* used when a notification was received */
int hc_domove(void *ptr, const char *move);
