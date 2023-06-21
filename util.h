/*******************************************************************************
 * Name          : util.h
 * Author        : Brian S. Borowski
 * Version       : 1.2
 * Date          : April 24, 2020
 * Last modified : April 23, 2023
 * Description   : Helpful utility functions for chat server/client.
 ******************************************************************************/
#ifndef _UTIL_H_
#define _UTIL_H_

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_MSG_LEN  1024
#define MAX_NAME_LEN 20
#define BUFLEN       MAX_MSG_LEN + MAX_NAME_LEN + 4
                     // +4 = '[' before name, "]: " after name

enum parse_string_t { OK, NO_INPUT, END_OF_FILE, TOO_LONG };

/* Functions that should be used in client and server. */
bool parse_int(const char *input, int *i, const char *usage);
int get_string(char *buf, const size_t sz);

/**
 * Determines if the string input represent a valid integer.
 */
bool is_integer(const char *input) {
    int start = 0, len = strlen(input), i;

    if (len >= 1 && input[0] == '-') {
        if (len < 2) {
            return false;
        }
        start = 1;
    }
    for (i = start; i < len; i++) {
        if (!isdigit(input[i])) {
            return false;
        }
    }
    return len > 0;
}

/**
 * Attempts to convert the input string into the integer i.
 * Returns true if the conversion was successful, false otherwise.
 */
bool parse_int(const char *input, int *i, const char *usage) {
    long long long_long_i;

    if (strlen(input) == 0) {
        fprintf(stderr,
                "Error: Invalid input '' received for %s.\n", usage);
        return false;
    }
    if (is_integer(input) && sscanf(input, "%lld", &long_long_i) == 1) {
        *i = (int)long_long_i;
        if (long_long_i != *i) {
            fprintf(stderr, "Error: Integer overflow for %s.\n", usage);
            return false;
        }
    } else {
        fprintf(stderr, "Error: Invalid input '%s' received for %s.\n", input,
                usage);
        return false;
    }
    return true;
}

/**
 * Reads a string from STDIN up to maxlen characters. If more than maxlen
 * characters are supplied, the function consumes them.
 * Returns OK, NO_INPUT, END_OF_FILE, or TOO_LONG.
 */
int get_string(char *buf, const size_t maxlen) {
    int i = 0, ch;
    bool too_long = false;
    errno = 0;
    while ((ch = getc(stdin)) != EOF) {
        if (i > maxlen) {
            too_long = true;
        }
        if (ch == '\n') {
            break;
        }
        if (i < maxlen) {
            buf[i] = ch;
        }
        i++;
    }

    if (ch == EOF) {
        // If EOF signifies failure...
        if (errno != 0) {
            fprintf(stderr,
                "Warning: Failed to read from stdin. %s.\n",
                strerror(errno));
        }
        return END_OF_FILE;
    } else if (ch != '\n') {
        // Consume any extra characters on line.
        ch = getc(stdin);
        while (ch != EOF && ch != '\n') {
            ch = getc(stdin);
        }
    }
    if (too_long) {
        return TOO_LONG;
    }
    buf[i] = '\0';
    if (i == 0) {
        return NO_INPUT;
    }

    return OK;
}

#endif
