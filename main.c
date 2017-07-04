#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "connection.h"

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_GLFW_GL3_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_glfw_gl3.h"

#define WINDOW_W 1024
#define WINDOW_H 600
#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

#define SQL_QUERY_MAX_LEN 256

// Glfw callbacks
void glfw_window_close_callback(GLFWwindow * window);
void glfw_window_size_callback(GLFWwindow * window, int width, int height);
void glfw_cursor_pos_callback(GLFWwindow * window, double mx, double my);
void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void glfw_mouse_button_callback(GLFWwindow * window, int button, int action, int mods);
void glfw_error_callback(int error, const char* description);

int main(int argc, char const *argv[]) {
    printf("\n\n\nadb-control-center\nEsc or Q to quit.\n\n\n");

    // Create window
    GLFWwindow* window;

    // Set error callback early
    glfwSetErrorCallback(glfw_error_callback);
    if(!glfwInit()) {
        printf("glfwInit Failed\n");
    } else {
        printf("glfwInit Success\n");
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(WINDOW_W, WINDOW_H, "adb-control-center", NULL, NULL);
    if(!window) {
        printf("glfwCreateWindow Failed\n");
        glfwTerminate();
        return 1;
    } else {
        printf("glfwCreateWindow Success\n");
    }

    printf("Setting callbacks\n");
    glfwSetWindowCloseCallback(window, glfw_window_close_callback);
    glfwSetWindowSizeCallback(window, glfw_window_size_callback);
    glfwSetKeyCallback(window, glfw_key_callback);
    glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
    glfwSetCursorPosCallback(window, glfw_cursor_pos_callback);

    printf("Setting context\n");
    glfwMakeContextCurrent(window);

    float aspect = WINDOW_W / (float)WINDOW_H;

    // Setup nuklear
    struct nk_context* ctx = nk_glfw3_init(window, NK_GLFW3_INSTALL_CALLBACKS);

    struct nk_font_atlas *atlas;
    nk_glfw3_font_stash_begin(&atlas);
    struct nk_font *font = nk_font_atlas_add_from_file(atlas, "iosevka-medium.ttf", 13, 0);
    nk_glfw3_font_stash_end();
    static struct nk_color background = {130, 50, 50, 255};
    //set_style(ctx, THEME_WHITE);
    nk_init_default(ctx, &font->handle);

    float frame_time_speed = 0;
    int frame_count = 0;

    // Connection to adb server
    // @note connection errors would be handled in the future
    // @note doing this in another thread would be nice
    struct AdbConnection* connection = init_adb_connection();

    // @note hardcoded data
    char* package_name = {"pt.stratio.mapicoimbra"};
    char* sql_db_name = {"mapi_coimbra.db"};

    // Keep this data across frames
    char* resp_adb_devices = (char*) malloc(sizeof(connection->buffer));
    strcpy(resp_adb_devices, "NO_CONNECTION");

    char* sql_query = (char*) malloc(SQL_QUERY_MAX_LEN * sizeof(char));
    int sql_query_len = 0;
    char* resp_sql_query = (char*) malloc(2 * SQL_QUERY_MAX_LEN * sizeof(char));


    // @note requires root and sqlite3 binary installed on the device,
    // an alternative option would be create a socket server within the application
    // to accept the commands
    char* sql_query_format = {"shell:su -c 'sqlite3 -batch /data/data/%s/databases/%s \"%s\"'"};
    char* adb_sql_query_command = (char*) malloc(3 * SQL_QUERY_MAX_LEN * sizeof(char));
    // @note handwaving the required allocation for the commands

    char* adb_clear_format = {"shell:pm clear %s"};
    char* adb_clear_command = (char*) malloc((strlen(adb_clear_format) + strlen(package_name)) * sizeof(char));
    sprintf(adb_clear_command, adb_clear_format, package_name);


    // Retrieve initial data from adb server
    execute_adb_command(connection,"host:devices-l", resp_adb_devices);


    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    while(!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        nk_glfw3_new_frame();
        if (nk_begin(ctx, "devices", nk_rect(10, 10, 280, 100),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {

            nk_layout_row_static(ctx, 30, 300, 1);
            nk_text_wrap(ctx, resp_adb_devices, strlen(resp_adb_devices));
        }
        nk_end(ctx);

        if (nk_begin(ctx, "sqlite", nk_rect(310, 10, 700, 420),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {

            nk_layout_row_static(ctx, 30, 300, 2);
            nk_edit_string(ctx, NK_EDIT_FIELD, sql_query, &sql_query_len, SQL_QUERY_MAX_LEN, 0);
            if (nk_button_label(ctx, "query")) {
                sql_query[sql_query_len] = '\0';
                resp_sql_query[0] = '\0';
                sprintf(adb_sql_query_command, sql_query_format, package_name, sql_db_name, sql_query);
                execute_adb_command(connection, adb_sql_query_command, resp_sql_query);
                //execute_adb_command(connection, "host:devices-l", resp_sql_query);

            }
            nk_layout_row_static(ctx, 600, 500, 1);
            nk_text_wrap(ctx, resp_sql_query, strlen(resp_sql_query));
        }
        nk_end(ctx);

        if (nk_begin(ctx, "commands", nk_rect(10, 120, 280, 200),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {

            nk_layout_row_static(ctx, 30, 250, 1);
            if (nk_button_label(ctx, "clear")) {
                execute_adb_command(connection, adb_clear_command, NULL);
            }
            if (nk_button_label(ctx, "uninstall")) {
                printf("Not implemented\n");
            }
            nk_layout_row_static(ctx, 30, 250, 1);
            if (nk_button_label(ctx, "clear & uninstall")) {
                printf("Not implemented\n");
            }
            nk_layout_row_static(ctx, 30, 250, 1);
            if (nk_button_label(ctx, "send boot")) {
                printf("Not implemented\n");
            }

        }
        nk_end(ctx);

        nk_glfw3_new_frame();
        if (nk_begin(ctx, "application", nk_rect(10, 330, 280, 100),
                     NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                     NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
            nk_layout_row_static(ctx, 30, 300, 1);
            nk_text_wrap(ctx, package_name, strlen(package_name));
            nk_text_wrap(ctx, sql_db_name, strlen(sql_db_name));
        }
        nk_end(ctx);



        nk_glfw3_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_BUFFER, MAX_ELEMENT_BUFFER);

        glfwSwapBuffers(window);
        glfwPollEvents();
        ++frame_count;
    }

    nk_glfw3_shutdown();
    glfwMakeContextCurrent(NULL);
    glfwDestroyWindow(window);
    glfwTerminate();

    printf("Finish\n");

    teardown_adb_connection(connection);






    return 0;
}


void glfw_window_close_callback(GLFWwindow * window) {}

void glfw_window_size_callback(GLFWwindow * window, int width, int height) {}

void glfw_cursor_pos_callback(GLFWwindow * window, double mx, double my) {}

void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {

    switch(key) {
    case GLFW_KEY_RIGHT:
        break;
    case GLFW_KEY_LEFT:
        break;
    case GLFW_KEY_UP:
        break;
    case GLFW_KEY_DOWN:
        break;
    }

    if(key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void glfw_mouse_button_callback(GLFWwindow * window, int button, int action, int mods) {}

void glfw_error_callback(int error, const char* description) {
    printf("glfw_error_callback: %d -> %s\n", error, description );
}
