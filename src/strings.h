#pragma once

#include "main.h"

void reader_file(vec_str_t& lines, const char* filename, const char* section, size_t max_len);
void reader_path(vec_str_t& lines, const char* filename, const char* section, size_t max_len);
void parse_format_args(char* buf, const char* src, int args_count, size_t max_len);
int count_format_args(const char* src);
int __cdecl btoi(const char* src);
int __cdecl htoi(const char* src);
int __cdecl stoi(const char* src);
void __cdecl purge_spaces(char* buf);
void __cdecl kill_lf(char* buf);
char* strcpy_n(char* dst, size_t count, const char* src);
char* strtrail(char* s);
char* strtrim(char* s);
void strtrim(std::string& dst, const std::string& src);
