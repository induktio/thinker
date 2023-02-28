#pragma once

#include "main.h"

void reader_file(vec_str_t& lines, const char* filename, const char* section, size_t max_len);
void reader_path(vec_str_t& lines, const char* filename, const char* section, size_t max_len);
char* parse_str(char* buf, size_t len, const char* s1, const char* s2, const char* s3, const char* s4);
char* strstrip(char* s);

