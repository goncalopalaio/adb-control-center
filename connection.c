#include <stdlib.h>
#include <stdio.h>

#include "connection.h"
#define STS_NET_IMPLEMENTATION
#include "sts_net.h"

#ifndef M_MIN
#define M_MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

int error(const char* msg) {
    printf("Error Message: %s\n\n", msg);
    return 1;
}

void teardown_adb_connection(struct AdbConnection* conn) {
    //sts_net_close_socket(&conn->socket);
    free(conn);
    sts_net_shutdown();
}

int execute_adb_command(struct AdbConnection* conn, char* command, char* response_buf) {
    int err = send_adb_message(conn, "host:transport-any", NULL,1, 0);
    if (!err) {
        printf("EXEC:\n%s\n", command);
        send_adb_message(conn, command, response_buf,0, 0);
    }
    return 0;
}

int send_adb_message(struct AdbConnection* conn, char* message, char* response, int open_connection, int close_connection) {
    if (open_connection) {
        printf("Connecting to %s %s\n", conn->host, conn->port );
        if (sts_net_open_socket(&conn->socket, conn->host, conn->port) < 0) {
            return error(sts_net_get_last_error());
        }
    } else {
        printf("Not opening connection\n");
    }

    unsigned int len = strlen(message);
    sprintf(conn->buffer, "%04X%s",len, message);
    printf("Sending:\n%s\n", conn->buffer);

    if (sts_net_send(&conn->socket, conn->buffer, strlen(conn->buffer) * sizeof(char)) < 0) {
        return error(sts_net_get_last_error());
    }

    int sizeof_buffer = sizeof(conn->buffer);
    int bytes = 0;
    memset(conn->buffer, 0, sizeof_buffer);
    bytes = sts_net_recv(&conn->socket, conn->buffer, sizeof_buffer);
    printf("Bytes %d -> response:%s\n\n",bytes, conn->buffer);

    if (bytes <= 0) {
        printf("Error: Read %d bytes read\n", bytes);
        return error(sts_net_get_last_error());
    }

    if (conn->buffer[0] == 'F' &&
            conn->buffer[1] == 'A' &&
            conn->buffer[2] == 'I' &&
            conn->buffer[3] == 'L'
       ) {
        printf("Error: Got FAIL\n");
        return error(sts_net_get_last_error());
    }

    if (response == NULL) {
        printf("Returning without reading response body\n");
        return 0;
    }

    response[0] = '\0';
    memset(conn->buffer, 0, sizeof_buffer);
    printf("Reading response, sizeof_buffer %d\n", sizeof_buffer);

    int response_remaining = sizeof_buffer;
    char* tresp = response;
    int in = 0;
    // @note lets just assume that the response has the same size as the buffer
    while ((in=sts_net_recv(&conn->socket, conn->buffer, sizeof_buffer))) {
        printf("Remaining(%d) .In(%d)", response_remaining, in);
        memcpy(tresp, conn->buffer, M_MIN(response_remaining, in));
        tresp+=in;
        response_remaining-=in;
        if (response_remaining <= 0) {
            break;
        }
    }
    printf("\nResponse:\n%s\n-END-\n", response);

    if (close_connection) {
        sts_net_close_socket(&conn->socket);
    }

    return 0;
}

struct AdbConnection* init_adb_connection() {
    struct AdbConnection* conn = (struct AdbConnection*)malloc(sizeof(struct AdbConnection));
    strcpy(conn->host, "localhost");
    strcpy(conn->port, "5037");
    sts_net_init();
    return conn;
}
