
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

int count_format_args(const char* buf) {
    int num = 0;
    int len = strlen(buf);
    int i;

    for (i = 0; i < len - 1; i++) {
        if (buf[i] == '%' && buf[i + 1] != '%') {
            if (buf[i + 1] != 'd' && buf[i + 1] != 'i' && buf[i + 1] != 'u') {
                return -1;
            }
            num++;
        }
    }
    return num;
}

uint32_t btoi(const char* src) {
    uint32_t value = 0;
    while (*src == '0' || *src == '1') {
        value = 2 * value + (*src == '1');
        src++;
    }
    return value;
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

