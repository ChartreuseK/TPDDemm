/*
 * Hayden Kroepfl - 2017
 */
#ifndef LOG_H
#define LOG_H
#include <stdio.h>

#define loginfo(fmt, ...) fprintf(stderr, "INFO    %s:%d - " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logwarn(fmt, ...) fprintf(stderr, "WARNING %s:%d - " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define logerror(fmt, ...) fprintf(stderr, "ERROR   %s:%d - " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)


#endif
