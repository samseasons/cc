// cc bundle.c -o bundle.out && ./bundle.out a/a.js a/y.js

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char base64[65] = "$0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

typedef struct {
    char * key;
    char * value;
} pair;

typedef struct {
    int length;
    pair ** pairs;
} map;

map amap () {
    map a;
    a.length = 0;
    a.pairs = malloc(1);
    return a;
}

void delete (map * a, char * key) {
    for (int i = 0; i < a->length; i++) {
        if (strcmp(a->pairs[i]->key, key) == 0) {
            a->length--;
            for (int j = i; j < a->length; j++) {
                a->pairs[j] = a->pairs[j + 1];
            }
            return;
        }
    }
}

char * get (map * a, char * key) {
    for (int i = 0; i < a->length; i++) {
        if (strcmp(a->pairs[i]->key, key) == 0) {
            return a->pairs[i]->value;
        }
    }
    return 0;
}

int sizeof_char_p = sizeof(char *);
int sizeof_char_p2 = sizeof(char *) * 2;

void set (map * a, char * key, char * value) {
    delete(a, key);
    pair * p = malloc(sizeof_char_p2);
    p->key = key;
    p->value = value;
    a->pairs = realloc(a->pairs, (a->length + 1) * sizeof_char_p2);
    a->pairs[a->length++] = p;
}

char * lstrip (char * str) {
    while (* str == '\t' || * str == ' ') {
        str++;
    }
    return str;
}

char * rstrip (char * str) {
    int i = -1;
    for (int j = 0; str[j] != '\0'; j++) {
        if (str[j] != '\t' && str[j] != ' ') {
            i = j;
        }
    }
    str[i + 1] = '\0';
    return str;
}

int find_first_of (char * str, char * set, int start) {
    int length = strlen(str);
    if (start < length) {
        for (int i = start; i < length; i++) {
            if (strchr(set, str[i])) {
                return i;
            }
        }
    }
    return -1;
}

char * first_not_of (char * str) {
    while (* str && (* str == '\t' || * str == ' ')) {
        str++;
    }
    return str;
}

char ** splits (char * str, char * delimiter, int * length) {
    char * buffer = malloc(strlen(str) + 1);
    strcpy(buffer, str);
    int i = 0;
    char * p = strtok(str, delimiter);
    while (p) {
        i++;
        p = strtok(0, delimiter);
    }
    char ** array = malloc((i + 1) * sizeof_char_p);
    if (i == 0) {
        array[i] = str;
        return array;
    }
    i = 0;
    p = strtok(buffer, delimiter);
    while (p) {
        array[i++] = p;
        p = strtok(0, delimiter);
    }
    * length = i;
    return array;
}

bool starts_with (char * str, char * prefix) {
    while (* prefix) {
        if (* prefix++ != * str++) {
            return false;
        }
    }
    return true;
}

char * substr (char * str, int start, int length) {
    char * sub = malloc(length * sizeof_char_p);
    memcpy(sub, str + start, length);
    sub[length] = '\0';
    return sub;
}

char * substrs (char * str, int start) {
    return substr(str, start, strlen(str) - start);
}

char * resolve (char * f, char * file) {
    if (strncmp(f, "./", 2) == 0) {
        f += 2;
    }
    char i = f[0];
    if (i != '.' && i != '/') {
        int last = strrchr(file, '/') - file;
        int length = last + strlen(f) + 2;
        char * str = malloc(length);
        snprintf(str, length, "%s%s%s", substr(file, 0, last), "/", f);
        f = str;
    } else if (strncmp(f, "../", 3) == 0) {
        while (strncmp(f, "../", 3) == 0) {
            f = substrs(f, 3);
            file = substr(file, 0, strrchr(file, '/') - file);
        }
        int last = strrchr(file, '/') - file;
        int length = last + strlen(f) + 2;
        char * str = malloc(length);
        snprintf(str, length, "%s%s%s", substr(file, 0, last), "/", f);
        f = str;
    }
    if (strcmp(f + strlen(f) - 3, ".js") != 0) {
        f = realloc(f, strlen(f) + 4);
        strcat(f, ".js");
    }
    return f;
}

char * substitute (char * next, char * past, char * text) {
    int a = 0;
    int i = strlen(past);
    int j = strlen(next);
    text = realloc(text, strlen(text) * j / i);
    char * p = strstr(text, past);
    while (p) {
        a = p - text;
        if (strchr(base64, text[a + i]) || (a != 0 && (strchr(base64, text[a - 1]) || strchr("\"'.", text[a - 1])))) {
            p = strstr(text + a + i, past);
            continue;
        }
        memmove(text + a + j, p + i, strlen(text + a + i) + 1);
        memcpy(text + a, next, j);
        p = strstr(text + a + j, past);
    }
    return text;
}

void parse (char * file, map * modules, map * texts) {
    FILE * f = fopen(file, "r");
    if (!f) {
        set(texts, file, "");
        return;
    }
    fseek(f, 0, SEEK_END);
    int length = ftell(f);
    rewind(f);
    char * text = malloc(length + 1);
    fread(text, 1, length, f);
    fclose(f);
    text[length] = '\0';
    char * lines = text;
    bool remove = false;
    text = malloc(strlen(text) + 2);
    text[0] = '\0';
    char * line = strtok(lines, "\n");
    while (line) {
        if (starts_with(lstrip(line), "//")) {
            line = strtok(0, "\n");
            continue;
        }
        char * i = strstr(line, "/*");
        if (!remove && i && !strstr(line, "//*")) {
            char * j = strstr(line, "*/");
            if (j) {
                length = strlen(line) + 2;
                char * str = malloc(length);
                snprintf(str, length, "%s%s%s", substr(line, 0, i - line), " ", substrs(line, j - line + 2));
                line = str;
            } else {
                line = substr(line, 0, i - line);
                remove = true;
            }
        }
        if (remove) {
            if ((i = strstr(line, "*/"))) {
                line = substrs(line, i - line + 2);
                remove = false;
            } else {
                line = strtok(0, "\n");
                continue;
            }
        }
        line = rstrip(line);
        if (line) {
            strcat(text, line);
            strcat(text, "\n");
        }
        line = strtok(0, "\n");
    }
    map files = amap();
    set(& files, file, "");
    char ** order = malloc(1);
    int order_length = 0;
    int order_size = 1;
    char * texta = text;
    char * ip;
    while ((ip = strstr(text, "import "))) {
        int i = ip - text;
        if (i != 0) {
            char j = text[i - 1];
            if (j != '\t' && j != '\n' && j != ' ') {
                text = substrs(text, i + 6);
                continue;
            }
        }
        i += 6;
        while (text[i] == ' ') {
            i++;
        }
        text = substrs(text, i);
        ip = strstr(text, "from");
        i = ip - text;
        char * jp = strchr(text, '"');
        char * kp = strchr(text, '\'');
        char ** names = malloc(sizeof_char_p);
        int names_length = 0;
        int names_size = 1;
        if (ip && (!jp || i < jp - text) && (!kp || i < kp - text)) {
            while (i < strlen(text)) {
                char j = text[i - 1];
                char k = text[i + 4];
                if ((j == ' ' || j == '}') && (k == ' ' || k == '"' || k == '\'')) {
                    break;
                }
                i += 4;
                ip = substrs(text, i);
                i += strstr(ip, "from") - ip;
            }
            int j = 0, k;
            char * t = substr(text, 0, i);
            while ((k = find_first_of(t, " ,{}", j)) != -1) {
                if (j < k) {
                    char * name = substr(t, j, k - j);
                    names_size += strlen(name) + 1;
                    names = realloc(names, (names_length + 1) * sizeof_char_p);
                    names[names_length++] = name;
                }
                j = k + 1;
            }
            if (j > strlen(t)) {
                char * name = substrs(t, j);
                names_size += strlen(name) + 1;
                names = realloc(names, (names_length + 1) * sizeof_char_p);
                names[names_length++] = name;
            }
            i += 5;
            while (text[i] == ' ') {
                i++;
            }
        } else {
            i = 0;
        }
        char f = text[i];
        if (f == '"' || f == '\'') {
            text = substrs(text, i + 1);
            ip = strchr(text, f);
            char * f = resolve(substr(text, 0, ip - text), file);
            char * imported = get(& files, f);
            if (!imported) {
                imported = "";
                order = realloc(order, (order_length + 1) * sizeof_char_p);
                order[order_length++] = f;
                order_size += strlen(f) + 1;
            }
            char * imports = malloc(strlen(imported) + names_size);
            strcpy(imports, imported);
            for (int j = 0; j < names_length; j++) {
                strcat(imports, names[j]);
                strcat(imports, "\n");
            }
            set(& files, f, imports);
        }
    }
    char * mods = malloc(order_size);
    strcpy(mods, "");
    for (int i = 0; i < order_length; i++) {
        char * f = order[i];
        if (!get(texts, f)) {
            strcat(mods, f);
            strcat(mods, "\n");
            if (!get(modules, f)) {
                set(modules, file, mods);
                return;
            }
        }
    }
    char declares[7][9] = {"async", "class", "const", "default", "function", "let", "var"};
    char defines[6] = {'\n', ' ', '(', ',', '.', '['};
    text = texta;
    while ((ip = strstr(text, "export "))) {
        int i = ip - text;
        text = substrs(text, i + 7);
        for (i = 0; i < 7; i++) {
            char * name = declares[i];
            ip = strstr(text, name);
            int j = ip - text;
            if (ip && j < 3) {
                text = substrs(text, j + strlen(name));
            }
        }
        char * names = "";
        if ((ip = strchr(text, '\n'))) {
            names = substr(text, 0, ip - text);
        }
        i = 0;
        while (i < strlen(names) && names[i] == ' ') {
            i++;
        }
        char ** split = malloc(sizeof_char_p);
        length = 0;
        if (i < strlen(names) && names[i] == '{') {
            names = substrs(names, i + 1);
            split = splits(substr(names, 0, strchr(names, '}') - names), ",", & length);
        } else {
            ip = strchr(names, '(');
            i = ip - names;
            char * jp = strchr(names, '=');
            int j = jp - names;
            if (!jp || (i < j && ip)) {
                split[length++] = names;
            } else {
                while (jp && names[j + 1] != '>') {
                    split = realloc(split, (length + 1) * sizeof_char_p);
                    split[length++] = substr(names, 0, j);
                    names = substrs(names, j);
                    if (!(jp = strchr(names, ','))) {
                        break;
                    }
                    names = substrs(names, jp - names);
                    jp = strchr(names, '=');
                    j = jp - names;
                }
            }
        }
        char * f = get(& files, file);
        for (i = 0; i < length; i++) {
            char * name = split[i];
            bool subs = true;
            while (strlen(name) && subs) {
                char n = name[0];
                subs = false;
                for (int j = 0; j < 6; j++) {
                    if (defines[j] == n) {
                        name = substrs(name, 1);
                        subs = true;
                        break;
                    }
                }
            }
            for (int j = 0; j < 6; j++) {
                char * jp = strchr(name, defines[j]);
                if (jp) {
                    name = substr(name, 0, jp - name);
                }
            }
            int len = strlen(f) + strlen(name) + 2;
            char * str = malloc(len);
            snprintf(str, len, "%s%s%s", f, name, "\n");
            f = str;
        }
        set(& files, file, f);
    }
    text = texta;
    for (int i = 0, files_length = files.length; i < files_length; i++) {
        pair * p = files.pairs[i];
        char * f = p->key;
        length = strlen(f);
        char * path = substr(f, 0, length - 3);
        for (int j = 0; j < length; j++) {
            if (!strchr(base64, path[j])) {
                path[j] = '_';
            }
        }
        length = 0;
        char ** names = splits(p->value, "\n", & length);
        for (int j = 0; j < length; j++) {
            char * name = names[j];
            int len = strlen(name) + strlen(path) + 2;
            char * next = malloc(len);
            snprintf(next, len, "%s%s%s", name, "_", path);
            text = substitute(next, name, text);
        }
    }
    lines = text;
    text = malloc(strlen(text) + 1);
    text[0] = '\0';
    line = strtok(lines, "\n");
    while (line) {
        char * a = first_not_of(line);
        if (strncmp(a, "export default ", 15) == 0) {
            line = substrs(line, 15);
        } else if (strncmp(a, "export ", 7) == 0) {
            line = substrs(line, 7);
            a = first_not_of(line);
            if (* a == '{') {
                line = strtok(0, "\n");
                continue;
            }
        }
        if (line && strncmp(a, "import ", 7) != 0) {
            strcat(text, line);
            strcat(text, "\n");
        }
        line = strtok(0, "\n");
    }
    set(texts, file, text);
}

void prepend (char *** array, int * length, char ** list, int list_length) {
    char ** result = malloc((* length + list_length) * sizeof_char_p);
    for (int i = 0; i < list_length; i++) {
        result[i] = list[i];
    }
    for (int i = 0; i < * length; i++) {
        result[i + list_length] = (* array)[i];
    }
    * array = result;
    * length += list_length;
}

void build (char * file, char * output) {
    char ** imported = malloc(1);
    int imported_length = 0;
    char ** imports = malloc(sizeof_char_p);
    int imports_length = 1;
    imports[0] = file;
    map modules = amap();
    map texts = amap();
    while (imports_length != 0) {
        file = imports[0];
        bool import = true;
        for (int i = 0; i < imported_length; i++) {
            if (strcmp(imported[i], file) == 0) {
                memmove(& imports[0], & imports[1], --imports_length * sizeof_char_p);
                import = false;
                break;
            }
        }
        if (import) {
            parse(file, & modules, & texts);
            char * mods_string = get(& modules, file);
            if (mods_string) {
                int length = 0;
                char ** mods = splits(mods_string, "\n", & length);
                prepend(& imports, & imports_length, mods, length);
            }
            if (get(& texts, file)) {
                imported = realloc(imported, (imported_length + 1) * sizeof_char_p);
                imported[imported_length++] = file;
            }
        }
    }
    int length = 1;
    for (int i = 0; i < imported_length; i++) {
        length += strlen(get(& texts, imported[i]));
    }
    char text[length];
    for (int i = 0; i < imported_length; i++) {
        strcat(text, get(& texts, imported[i]));
    }
    FILE * f = fopen(output, "w");
    if (f) {
        fputs(text, f);
        fclose(f);
    }
}

int main (int argc, char * argv[]) {
    build(argc > 1 ? argv[1] : "a/a.js", argc > 2 ? argv[2] : "a/y.js");
    return 0;
}