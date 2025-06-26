#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"

void print_timestamp() {
    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    printf("[%02d:%02d:%02d] ", tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
}

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
