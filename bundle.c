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

char * substitute (char * match, char * next, char * text) {
    int a;
    int i = strlen(match);
    int j = strlen(next);
    text = realloc(text, strlen(text) * j / i);
    char * p = strstr(text, match);
    while (p) {
        a = p - text;
        if (strchr(base64, text[a + i]) || (a != 0 && (strchr(base64, text[a - 1]) || strchr("\"'.", text[a - 1])))) {
            p = strstr(text + a + i, match);
        } else {
            memmove(text + a + j, p + i, strlen(text + a + i) + 1);
            memcpy(text + a, next, j);
            p = strstr(text + a + j, match);
        }
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
        if (strlen(line) != 0) {
            strcat(text, line);
            strcat(text, "\n");
        }
        line = strtok(0, "\n");
    }
    map defaults = amap();
    map exports = amap();
    set(& exports, file, "");
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
        char * defaulted = "";
        map names = amap();
        int names_size = 1;
        text = substrs(text, i);
        ip = strstr(text, "from");
        i = ip - text;
        char * jp = strchr(text, '"');
        char * kp = strchr(text, '\'');
        if (ip && (!jp || i < jp - text) && (!kp || i < kp - text)) {
            length = strlen(text);
            while (i < length) {
                char j = text[i - 1];
                char k = text[i + 4];
                if ((j == ' ' || j == '}') && (k == ' ' || k == '"' || k == '\'')) {
                    break;
                }
                i += 4;
                ip = substrs(text, i);
                i += strstr(ip, "from") - ip;
            }
            char * variables = substr(text, 0, i);
            if ((jp = strchr(variables, '{'))) {
                int j = jp - variables;
                kp = strchr(variables, '}');
                length = 0;
                char ** split = splits(substr(variables, j + 1, kp - variables - j - 1), ",", & length);
                variables = substr(variables, 0, j);
                for (j = 0; j < length; j++) {
                    char * name = rstrip(lstrip(split[j]));
                    kp = strstr(name, " as ");
                    if (kp) {
                        int k = kp - name;
                        names_size += strlen(name) - 2;
                        set(& names, substr(name, 0, k), substrs(name, k + 4));
                    } else if (strlen(name) != 0) {
                        names_size += strlen(name) * 2 + 2;
                        set(& names, name, name);
                    }
                }
            }
            char * name = variables;
            if ((jp = strchr(name, ','))) {
                name = substr(name, 0, jp - name);
            }
            name = rstrip(lstrip(name));
            if (strlen(name) != 0) {
                defaulted = name;
                names_size += strlen(name) * 2 + 2;
                set(& names, name, name);
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
            char * imported = get(& exports, f);
            if (!imported) {
                imported = "";
                order = realloc(order, (order_length + 1) * sizeof_char_p);
                order[order_length++] = f;
                order_size += strlen(f) + 1;
            }
            if (strlen(defaulted) != 0) {
                set(& defaults, f, defaulted);
            }
            char * imports = malloc(strlen(imported) + names_size);
            strcpy(imports, imported);
            for (int j = 0, length = names.length; j < length; j++) {
                strcat(imports, names.pairs[j]->key);
                strcat(imports, " ");
                strcat(imports, names.pairs[j]->value);
                strcat(imports, "\n");
            }
            set(& exports, f, imports);
        }
    }
    int dependencies = 0;
    char * mods = malloc(order_size);
    mods[0] = '\0';
    for (int i = 0; i < order_length; i++) {
        char * f = order[i];
        if (!get(texts, f)) {
            strcat(mods, f);
            strcat(mods, "\n");
            if (!get(modules, f)) {
                dependencies = 1;
            }
        }
    }
    if (dependencies) {
        set(modules, file, mods);
        return;
    }
    char declares[7][9] = {"async", "class", "const", "default", "function", "let", "var"};
    char defines[6] = {'\n', ' ', '(', ',', '.', '['};
    text = texta;
    while ((ip = strstr(text, "export "))) {
        int i = ip - text;
        text = substrs(text, i + 7);
        if (strstr(text, "default ") - text == 0) {
            continue;
        }
        for (i = 0; i < 7; i++) {
            char * name = declares[i];
            ip = strstr(text, name);
            int j = ip - text;
            if (ip && j < 3) {
                text = substrs(text, j + strlen(name));
            }
        }
        if ((ip = strchr(text, '\n'))) {
            char * variables = substr(text, 0, ip - text);
            i = 0;
            length = strlen(variables);
            while (i < length && variables[i] == ' ') {
                i++;
            }
            char ** split = malloc(sizeof_char_p);
            if (i < length && variables[i] == '{') {
                length = 0;
                variables = substrs(variables, i + 1);
                split = splits(substr(variables, 0, strchr(variables, '}') - variables), ",", & length);
            } else {
                ip = strchr(variables, '(');
                i = ip - variables;
                char * jp = strchr(variables, '=');
                int j = jp - variables;
                length = 0;
                if (!jp || (i < j && ip)) {
                    split[length++] = variables;
                } else {
                    while (jp && variables[j + 1] != '>') {
                        split = realloc(split, (length + 1) * sizeof_char_p);
                        split[length++] = substr(variables, 0, j);
                        variables = substrs(variables, j);
                        if (!(jp = strchr(variables, ','))) {
                            break;
                        }
                        variables = substrs(variables, jp - variables);
                        jp = strchr(variables, '=');
                        j = jp - variables;
                    }
                }
            }
            char * f = get(& exports, file);
            for (i = 0; i < length; i++) {
                char * name = split[i];
                bool subs = true;
                while (strlen(name) != 0 && subs) {
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
                    if ((ip = strchr(name, defines[j]))) {
                        name = substr(name, 0, ip - name);
                    }
                }
                int len = strlen(f) + strlen(name) * 2 + 3;
                char * str = malloc(len);
                snprintf(str, len, "%s%s%s%s%s", f, name, " ", name, "\n");
                f = str;
            }
            set(& exports, file, f);
        }
    }
    char * defaulted = "";
    text = texta;
    for (int i = 0, exports_length = exports.length; i < exports_length; i++) {
        pair * p = exports.pairs[i];
        char * f = p->key;
        length = strlen(f);
        char * path = substr(f, 0, length - 3);
        for (int j = 0; j < length; j++) {
            if (!strchr(base64, path[j])) {
                path[j] = '_';
            }
        }
        if (strcmp(f, file) == 0) {
            defaulted = path;
        }
        length = 0;
        char ** exported = splits(p->value, "\n", & length);
        for (int j = 0; j < length; j++) {
            int exported_length = 0;
            char ** split = splits(exported[j], " ", & exported_length);
            char * named = split[0];
            char * name = split[1];
            char * module = get(& defaults, f);
            if (module && strcmp(module, name) == 0) {
                int len = strlen(path) + 2;
                char * next = malloc(len);
                snprintf(next, len, "%s%s", "_", path);
                text = substitute(name, next, text);
            } else {
                int len = strlen(named) + strlen(path) + 2;
                char * next = malloc(len);
                snprintf(next, len, "%s%s%s", named, "_", path);
                text = substitute(name, next, text);
            }
        }
    }
    lines = text;
    text = malloc(strlen(text) + strlen(defaulted) + 5);
    text[0] = '\0';
    line = strtok(lines, "\n");
    while (line) {
        char * a = first_not_of(line);
        if (strncmp(a, "export default ", 15) == 0) {
            line = substrs(line, 15);
            int len = strlen(defaulted) + strlen(line) + 5;
            char * b = malloc(len);
            snprintf(b, len, "%s%s%s%s", "_", defaulted, " = ", line);
            line = b;
        } else if (strncmp(a, "export ", 7) == 0) {
            line = substrs(line, 7);
            a = first_not_of(line);
            if (* a == '{') {
                line = strtok(0, "\n");
                continue;
            }
        }
        if (strlen(line) != 0 && strncmp(a, "import ", 7) != 0) {
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
                memmove(& imports[0], & imports[1], --imports_length * sizeof_char_p);
            }
        }
    }
    int length = 1;
    for (int i = 0; i < imported_length; i++) {
        length += strlen(get(& texts, imported[i]));
    }
    char * text = malloc(length);
    text[0] = '\0';
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