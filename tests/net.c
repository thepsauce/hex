#include "test.h"

int main(void)
{
	printf("Testing various port hashes:\n");
	printf("Name\t\tPort\n");
	printf("---------------------\n");
	printf("hello\t\t%d\n", net_porthash("hello"));
	printf("crusher\t\t%d\n", net_porthash("crusher"));
	printf("vaxeral\t\t%d\n", net_porthash("vaxeral"));
	printf("Reik\t\t%d\n", net_porthash("Reik"));
	printf("master\t\t%d\n", net_porthash("master"));
	printf("crazy\t\t%d\n", net_porthash("crazy"));
	printf("eng\t\t%d\n", net_porthash("eng"));
	printf("enm\t\t%d\n", net_porthash("enm"));
	printf("hnm\t\t%d\n", net_porthash("hnm"));
	printf("superlongname\t%d\n", net_porthash("superlongname"));
	printf("a\t\t%d\n", net_porthash("a"));
	return 0;
}
