#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "connection.h"

int main(int argc, char const *argv[]) {

    // Connection to adb server
    // @note connection errors would be handled in the future
    // @note doing this in another thread would be nice
    struct AdbConnection* connection = init_adb_connection();

    // @note hardcoded data
    char* package_name = {"pt.stratio.mapicoimbra"};
    char* sql_db_name = {"mapi_coimbra.db"};

    char response_buf[ADB_CON_BUF_LEN];
    //send_adb_message(connection, "host:version", NULL,1, 1);
    //send_adb_message(connection, "host:transport-any", NULL,1, 0);
    //send_adb_message(connection, "shell:ls", response_buf,0, 1);
    //send_adb_message(connection, "host:transport-any", NULL,1, 0);
    //send_adb_message(connection, "shell:time", response_buf,0, 0);
    memset(response_buf, 0, sizeof(response_buf));
    execute_adb_command(connection,"shell:time", response_buf);
    printf("SHELL:TIME:\n%s\n", response_buf);

    memset(response_buf, 0, sizeof(response_buf));
    execute_adb_command(connection,"host:devices-l", response_buf);
    printf("HOST:DEVICES-L:\n%s\n", response_buf);


    memset(response_buf, 0, sizeof(response_buf));
    execute_adb_command(connection,"shell:ls", response_buf);
    printf("SHELL:LS:\n%s\n", response_buf);



    teardown_adb_connection(connection);
    return 0;
}
