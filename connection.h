#ifndef _INCLUDE_CONNECTION_
#define _INCLUDE_CONNECTION_
#include "sts_net.h"

#define ADB_CON_BUF_LEN 512

struct AdbConnection {
	sts_net_socket_t  socket;
	// @note buffer could be removed since it is only being used as temporary storage
	char buffer[ADB_CON_BUF_LEN];	
	char host[32];
	char port[8];
};

struct AdbConnection* init_adb_connection();
int send_adb_message(struct AdbConnection* conn, char* message, char* response, int open_connection, int close_connection);
int execute_adb_command(struct AdbConnection* conn, char* command, char* response);
void teardown_adb_connection(struct AdbConnection* conn);

#endif
