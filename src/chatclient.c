#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "util.h"

int client_socket = -1, is_a_tty;
char username[MAX_NAME_LEN + 1];
char inbuf[BUFLEN + 1];
char outbuf[MAX_MSG_LEN + 1];

int handle_stdin() {
    int retval = get_string(outbuf, MAX_MSG_LEN);
    if (retval == TOO_LONG) {
        printf(
            "Sorry, limit your message to 1 line of at most %d characters.\n",
            MAX_MSG_LEN);
    } else if (retval == END_OF_FILE) {
        printf("\n");
        return 1;
    } else if (retval != NO_INPUT) {
        if (send(client_socket, outbuf, strlen(outbuf) + 1, 0) == -1) {
            fprintf(stderr,
                    "Warning: Failed to send message to server. %s.\n",
                   strerror(errno));
        }
        if (strcmp(outbuf, "bye") == 0) {
            printf("Goodbye.\n");
            return 1;
        }
    }
    return 0;
}

int handle_client_socket() {
    // Read the incoming message and use the number of bytes read to
    // check if the client disconnected.
    int i = 0,
        bytes_recvd = recv(client_socket, inbuf, 1, 0);
    if (bytes_recvd == 0) {
        fprintf(stderr, "\nConnection to server has been lost.\n");
        return 1;
    }
    while (inbuf[i] != '\0') {
        i++;
        if (i > BUFLEN) {
            break;
        }
        bytes_recvd = recv(client_socket, inbuf + i, 1, 0);
    }
    if (bytes_recvd == -1) {
        fprintf(stderr,
                "\nWarning: Failed to receive incoming message. %s.\n",
                strerror(errno));
    } else {
        if (strcmp(inbuf, "bye") == 0) {
            printf("\nServer initiated shutdown.\n");
            return 1;
        }
        printf("\n%s\n", inbuf);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server IP> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    is_a_tty = isatty(STDIN_FILENO);

    struct sockaddr_in server_addr;
    socklen_t addrlen = sizeof(struct sockaddr_in);

    memset(&server_addr, 0, addrlen); // Zero out structure
    int ip_conversion = inet_pton(AF_INET, argv[1], &server_addr.sin_addr);
    if (ip_conversion == 0) {
        fprintf(stderr, "Error: Invalid IP address '%s'.\n", argv[1]);
        return EXIT_FAILURE;
    } else if (ip_conversion < 0) {
        fprintf(stderr, "Error: Failed to convert IP address. %s.\n",
                strerror(errno));
        return EXIT_FAILURE;
    }
    server_addr.sin_family = AF_INET;   // Internet address family

    int port;
    if (!parse_int(argv[2], &port, "port number")) {
        return EXIT_FAILURE;
    }
    if (port < 1024 || port > 65535) {
        fprintf(stderr, "Error: Port must be in range [1024, 65535].\n");
        return EXIT_FAILURE;
    }
    server_addr.sin_port = htons(port); // Server port, 16 bits.

    int res;
    do {
        if (is_a_tty) {
            printf("Enter your username: ");
            fflush(stdout);
        }
        res = get_string(username, MAX_NAME_LEN);
        if (res == END_OF_FILE) {
            printf("\n");
            return EXIT_FAILURE;
        }
        if (res == TOO_LONG) {
            fprintf(stderr, "Sorry, limit your username to %d characters.\n",
                    MAX_NAME_LEN);
        }
    } while (res != OK);
    printf("Hello, %s. Let's try to connect to the server.\n", username);

    int retval = EXIT_SUCCESS;
    // Create a reliable, stream socket using TCP.
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: Failed to create socket. %s.\n",
                strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    // Establish the connection to the chat server.
    if (connect(client_socket, (struct sockaddr *)&server_addr, addrlen) < 0) {
        fprintf(stderr, "Error: Failed to connect to server. %s.\n",
                strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    // Receive and print welcome message.
    int bytes_recvd;
    if ((bytes_recvd = recv(client_socket, inbuf, BUFLEN + 1, 0)) < 0) {
        fprintf(stderr, "Error: Failed to receive message from server. %s.\n",
                strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    } else if (bytes_recvd == 0) {
        printf("All connections are busy. Try again later.\n");
        retval = EXIT_FAILURE;
        goto EXIT;
    }
    printf("\n%s\n\n", inbuf);

    if (send(client_socket, username, strlen(username) + 1, 0) == -1) {
        fprintf(stderr,
                "Error: Failed to send username to server. %s.\n",
                strerror(errno));
        retval = EXIT_FAILURE;
        goto EXIT;
    }

    fd_set sockset;
    while (true) {
        if (is_a_tty) {
            printf("[%s]: ", username);
            fflush(stdout);
        }
        // Zero out and set socket descriptors for server sockets.
        // This must be reset every time select() is called.
        FD_ZERO(&sockset);
        FD_SET(STDIN_FILENO, &sockset);
        FD_SET(client_socket, &sockset);

        // Wait for activity on one of the sockets.
        // Timeout is NULL, so wait indefinitely.
        if (select(client_socket + 1, &sockset, NULL, NULL, NULL) < 0
                && errno != EINTR) {
            fprintf(stderr, "Error: select() failed. %s.\n", strerror(errno));
            retval = EXIT_FAILURE;
            goto EXIT;
        }

        if (FD_ISSET(STDIN_FILENO, &sockset)) {
            if (handle_stdin()) {
                break;
            }
        }
        if (FD_ISSET(client_socket, &sockset)) {
            if (handle_client_socket()) {
                break;
            }
        }
    }
EXIT:
    if (fcntl(client_socket, F_GETFD) != -1) {
        close(client_socket);
    }
    return retval;
}
