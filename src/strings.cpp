
#include "strings.h"


void reader_path(vec_str_t& lines, const char* filename, const char* section, size_t max_len) {
    FILE* file;
    const int buf_size = 1024;
    char line[buf_size];
    bool start = false;
    file = fopen(filename, "r");
    if (!file) {
        return;
    }
    while (fgets(line, buf_size, file) != NULL) {
        line[strlen(line) - 1] = '\0';
        if (!start && stricmp(line, section) == 0) {
            start = true;
        } else if (start && line[0] == '#') {
            break;
        } else if (start && line[0] != ';') {
            char* p = strtrim(line);
            // Truncate long lines to max_len (including null)
            if (strlen(p) > 0 && p[0] != ';') {
                if (strlen(p) >= max_len) {
                    p[max_len - 1] = '\0';
                }
                lines.push_back(p);
            }
        }
    }
    fclose(file);
}

void reader_file(vec_str_t& lines, const char* filename, const char* section, size_t max_len) {
    const int buf_size = 1024;
    char filename_txt[buf_size];
    snprintf(filename_txt, buf_size, "%s.txt", filename);
    strlwr(filename_txt);
    reader_path(lines, filename_txt, section, max_len);
}

void parse_format_args(char* buf, const char* src, int args_count, size_t max_len) {
    int num = count_format_args(src);
    if (num >= 0 && num <= args_count && max_len > 0) {
        strncpy(buf, src, max_len);
        buf[max_len-1] = '\0';
    }
}

int count_format_args(const char* src) {
    int num = 0;
    int len = strlen(src);
    int i;

    for (i = 0; i < len - 1; i++) {
        if (src[i] == '%' && src[i + 1] != '%') {
            if (src[i + 1] != 'd' && src[i + 1] != 'i' && src[i + 1] != 'u') {
                return -1;
            }
            num++;
        }
    }
    return num;
}

void __cdecl purge_spaces(char* buf) {
    char* pos = buf;
    while (*pos == ' ') {
        pos++;
    }
    memmove(buf, pos, strlen(pos) + 1); // handle overlapping arrays
    pos = buf + strlen(buf);
    while (*(pos - 1) == ' ' && pos != buf) {
        pos--;
    }
    buf[(pos - buf)] = '\0';
}

void __cdecl kill_lf(char* buf) {
    char* pos = strrchr(buf, '\n');
    if (pos) {
        *pos = '\0';
    }
}

int __cdecl btoi(const char* src) {
    int value = 0;
    while (*src == '0' || *src == '1') {
        value = 2 * value + (*src == '1');
        src++;
    }
    return value;
}

int __cdecl htoi(const char* src) {
    int result = 0;
    while (isxdigit(*src)) {
        result *= 16;
        if (isdigit(*src)) {
            result += *src - '0';
        } else {
            result += toupper(*src) - '7';
        }
        src++;
    }
    return result;
}

int __cdecl stoi(const char* src) {
    if (*src == '0') {
        src++;
        switch (*src) {
        case 'B':
        case 'b':
            src++;
            return btoi(src);
        case 'X':
        case 'x':
            src++;
            return htoi(src);
        case 'D':
        case 'd':
            src++;
            return atoi(src);
        default:
            return atoi(src);
        }
    }
    return atoi(src);
}

char* strcpy_n(char* dst, size_t count, const char* src) {
    if (count > 0) {
        strncpy(dst, src, count);
        dst[count - 1] = '\0';
    }
    return dst;
}

char* strtrail(char* s) {
    size_t size;
    char* end;
    size = strlen(s);
    if (!size) {
        return s;
    }
    end = s + size - 1;
    while (end >= s && isspace(*end)) {
        end--;
    }
    *(end + 1) = '\0';
    return s;
}

char* strtrim(char* s) {
    size_t size;
    char* end;
    size = strlen(s);
    if (!size) {
        return s;
    }
    end = s + size - 1;
    while (end >= s && isspace(*end)) {
        end--;
    }
    *(end + 1) = '\0';
    while (*s && isspace(*s)) {
        s++;
    }
    return s;
}

void strtrim(std::string& dst, const std::string& src) {
    std::string::const_iterator a = src.begin(), b = src.end();
    while (isspace(*a)) {
        ++a;
    }
    while (a < b && isspace(*(b-1))) {
        --b;
    }
    dst.assign(a, b);
}

