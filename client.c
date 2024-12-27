#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_PARAMS 10
#define REQUEST_BUFFER 1024
#define RESPONSE_BUFFER 4096


// Function to print usage instructions
void print_usage() {
    printf("Usage: client [-r n <pr1=value1 pr2=value2 …>] <URL>\n");
    exit(EXIT_FAILURE);
}

// Validate port number
int validate_port(const char *port_str) {
    for (int i = 0; port_str[i]; i++) {
        if (!isdigit(port_str[i])) return 0;
    }
    int port = atoi(port_str);
    return (port > 0 && port < 65536);
}

// Parse the URL
void parse_url(const char *url, char *host, char *path, int *port) {
    if (strncmp(url, "http://", 7) != 0) {
        printf("Error: URL must start with 'http://'\n");
        print_usage();
    }

    const char *url_without_http = url + 7;
    const char *colon_ptr = strchr(url_without_http, ':');
    const char *slash_ptr = strchr(url_without_http, '/');

    if (colon_ptr && (!slash_ptr || colon_ptr < slash_ptr)) {
        strncpy(host, url_without_http, colon_ptr - url_without_http);
        host[colon_ptr - url_without_http] = '\0';

        const char *port_str = colon_ptr + 1;
        char port_buffer[6] = {0};
        int i = 0;
        while (*port_str && *port_str != '/' && i < 5) {
            port_buffer[i++] = *port_str++;
        }
        port_buffer[i] = '\0';

        if (!validate_port(port_buffer)) {
            printf("Error: Invalid port number\n");
            print_usage();
        }
        *port = atoi(port_buffer);

        if (*port_str == '/') {
            strcpy(path, port_str);
        } else {
            strcpy(path, "/");
        }
    } else {
        if (slash_ptr) {
            strncpy(host, url_without_http, slash_ptr - url_without_http);
            host[slash_ptr - url_without_http] = '\0';
            strcpy(path, slash_ptr);
        } else {
            strcpy(host, url_without_http);
            strcpy(path, "/");
        }
        *port = 80; // Default HTTP port
    }
}

// Parse Command-Line Arguments in Any Order
void parse_arguments(int argc, char *argv[], char **url, char *params[], int *param_count) {
    int i = 1;
    *param_count = 0;

    while (i < argc) {
        if (strcmp(argv[i], "-r") == 0) {
            i++;
            if (i >= argc || !isdigit(argv[i][0])) {
                print_usage();
            }

            *param_count = atoi(argv[i++]);
            for (int j = 0; j < *param_count; j++) {
                if (i >= argc || strchr(argv[i], '=') == NULL) {
                    print_usage();
                }
                params[j] = argv[i++];
            }
        } else if (strncmp(argv[i], "http://", 7) == 0) {
            *url = argv[i++];
        } else {
            print_usage();
        }
    }

    if (!*url) {
        print_usage();
    }
}

// Build the HTTP request string
void build_http_request(char *request, const char *host, const char *path, char *params[], int param_count) {
    char query_string[256] = {0};

    // Add parameters to the query string if they exist
    if (param_count > 0) {
        strcat(query_string, "?");
        for (int i = 0; i < param_count; i++) {
            strcat(query_string, params[i]);
            if (i < param_count - 1) {
                strcat(query_string, "&");
            }
        }
    }

    // Build the HTTP request
    snprintf(request, REQUEST_BUFFER,
             "GET %s%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             path, query_string, host);
}

// Connect to the server
int connect_to_server(const char *host, int port) {
    struct sockaddr_in server_addr;
    struct hostent *server;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server = gethostbyname(host);
    if (!server) {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }


    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = ((struct in_addr*)(server->h_addr_list[0]))->s_addr;

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Send and receive HTTP response
int send_and_receive(int sockfd, const char *request, char *location, const char *host, int port) {
    char buffer[RESPONSE_BUFFER];
    int bytes_received;
    location[0] = '\0'; // Reset Location header

    // Send the HTTP request
    if (write(sockfd, request, strlen(request)) < 0) {
        perror("write");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Read the HTTP response
    while ((bytes_received = read(sockfd, buffer, RESPONSE_BUFFER - 1)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        printf("\n Total received response bytes: %d\n",bytes_received);

        // Look for a 3XX status code (Redirection)
        if (strncmp(buffer, "HTTP/1.1 3", 10) == 0) {

            // Check for Location header
            char *location_start = strstr(buffer, "Location: ");
            if (location_start) {
                sscanf(location_start + 10, "%s", location);

                // Handle relative URL
                if (location[0] == '/') {
                    char absolute_url[512];
                    snprintf(absolute_url, sizeof(absolute_url), "http://%s:%d%s", host, port, location);
                    strcpy(location, absolute_url);
                }

                return 1; // Redirect found, return to follow the Location
            }
        }
    }

    if (bytes_received < 0) {
        perror("read");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    close(sockfd);
    return 0; // No redirect, or error
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage();
    }

    char *url = NULL;
    char *params[MAX_PARAMS];
    int param_count = 0;

    parse_arguments(argc, argv, &url, params, &param_count);

    char host[256] = {0};
    char path[256] = {0};
    int port = 80;

    parse_url(url, host, path, &port);

    char http_request[REQUEST_BUFFER];
    build_http_request(http_request, host, path, params, param_count);

    printf("HTTP request =\n%s\nLEN = %lu\n", http_request, strlen(http_request));

    int sockfd = connect_to_server(host, port);
    char location[RESPONSE_BUFFER] = {0};  // Buffer to store the Location header

    while (1) {
        int redirect = send_and_receive(sockfd, http_request, location,host,port);

        if (redirect) {
            printf("\nRedirecting to: %s\n", location);
            // Parse the URL from Location and make the new request
            parse_url(location, host, path, &port);
            build_http_request(http_request, host, path, params, param_count);
            close(sockfd); // Close previous connection
            sockfd = connect_to_server(host, port); // Reconnect to the new host
        } else {
            break;  // No redirect, exit the loop
        }
    }
    return 0;
}
