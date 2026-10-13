// cc serve.c -o serve.out && ./serve.out a 1234

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char types[4][2][23] = {
    {"css", "text/css"},
    {"html", "text/html"},
    {"ico", "image/x-icon"},
    {"js", "application/javascript"}
};

char * substr (char * str, int start, int length) {
    char * sub = malloc(length * sizeof(char *));
    memcpy(sub, str + start, length);
    sub[length] = '\0';
    return sub;
}

void serve (char * folder, int port) {
    char * index = "/x.html";
    if ((strstr(folder, ".html") - folder) == strlen(folder) - 5) {
        int i = strrchr(folder, '/') - folder;
        if (i != -1) {
            index = substr(folder, i, strlen(folder) - i);
            folder = substr(folder, 0, i);
        }
    }
    int server = socket(AF_INET, SOCK_STREAM, 0);
    int i = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, & i, sizeof(i));
    struct sockaddr_in sock;
    sock.sin_family = AF_INET;
    sock.sin_port = htons(port);
    socklen_t len = sizeof(sock);
    bind(server, (struct sockaddr *) & sock, len);
    listen(server, 5);
    printf("localhost:%d\n", port);
    while (1) {
        int client = accept(server, (struct sockaddr *) & sock, & len);
        char buffer[4096];
        read(client, buffer, 4096);
        char * p = strchr(buffer, ' ') + 1;
        int length = strchr(p, ' ') - p;
        char file[length + 1];
        strncpy(file, p, length);
        file[length] = '\0';
        if (file[0] != '/' || strstr(file, "..")) {
            continue;
        }
        while ((p = strstr(file, "%20"))) {
            memcpy(p, " ", 1);
            memmove(p + 1, p + 3, strlen(p + 3) + 1);
        }
        length = strlen(folder) + strlen(file) + 1;
        char * path = malloc(length);
        char type[23];
        int found = 0;
        char * ip = strrchr(file, '.');
        if (ip) {
            i = ip - file + 1;
            char * extension = substr(file, i, strlen(file) - i);
            for (int i = 0, len = sizeof(types) / sizeof(types[0]); i < len; i++) {
                if (strcmp(types[i][0], extension) == 0) {
                    snprintf(path, length, "%s%s", folder, file);
                    strcpy(type, types[i][1]);
                    found = 1;
                    break;
                }
            }
        }
        if (found == 0) {
            length = strlen(folder) + strlen(index) + 1;
            path = realloc(path, length);
            snprintf(path, length, "%s%s", folder, index);
            strcpy(type, "text/html");
        }
        FILE * f = fopen(path, "r");
        if (f) {
            length = strlen(type) + 24;
            char * header = malloc(length);
            snprintf(header, length, "%s%s%s", "HTTP/1.\ncontent-type:", type, "\n\n");
            send(client, header, length - 1, 0);
            while ((length = fread(buffer, 1, 4096, f)) > 0) {
                if (send(client, buffer, length, MSG_NOSIGNAL) == -1) {
                    break;
                }
            }
            fclose(f);
        }
        close(client);
    }
}

int main (int argc, char * argv[]) {
    serve(argc > 1 ? argv[1] : "a", argc > 2 ? atoi(argv[2]) : 1234);
    return 0;
}