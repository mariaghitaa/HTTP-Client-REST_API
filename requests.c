#include <stdlib.h> /* exit, atoi, malloc, free */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

char *compute_get_request(char *host, char *url, char *query_params,
                          char **cookies, int cookies_count, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL, request params (if any) and protocol type
    if (query_params != NULL)
    {
        sprintf(line, "GET %s?%s HTTP/1.1", url, query_params);
    }
    else
    {
        sprintf(line, "GET %s HTTP/1.1", url);
    }

    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (token)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", token); // Adauga prefixul corect pentru header
        compute_message(message, line);
    }
    // Step 3 (optional): add headers and/or cookies, according to the protocol format
    memset(line, 0, LINELEN);
    if (cookies != NULL)
    {
        sprintf(line, "Cookie: ");

        for (int i = 0; i < cookies_count; i++)
        {
            sprintf(line + strlen(line), "%s; ", cookies[i]);
        }

        if (cookies_count > 0)
        {
            line[strlen(line) - 1] = '\0';
            line[strlen(line) - 1] = '\0';
        }

        // printf("LINE = %s\n", line);

        compute_message(message, line);
    }
    // Step 4: add final new line
    compute_message(message, "");
    free(line);
    return message;
}

int find_message_length(int fields, char **body)
{
    int tot_len = 0;
    for (int i = 0; i < fields; i++)
        tot_len = tot_len + strlen(body[i]);
    if (fields > 0)
        tot_len = tot_len + (fields - 1);
    return tot_len;
}

char *compute_post_request(char *host, char *url, char *content_type, char **body_data,
                           int body_data_fields_count, char **cookies, int cookies_count, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "POST %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (token != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }
    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %d", find_message_length(body_data_fields_count, body_data));
    compute_message(message, line);

    // Step 4 (optional): add cookies
    memset(line, 0, LINELEN);
    if (cookies != NULL)
    {
        sprintf(line, "Cookie: ");

        for (int i = 0; i < cookies_count; i++)
        {
            sprintf(line + strlen(line), "%s; ", cookies[i]);
        }

        if (cookies_count > 0)
        {
            line[strlen(line) - 1] = '\0';
            line[strlen(line) - 1] = '\0';
        }

        compute_message(message, line);
    }
    // Step 5: add new line at end of header
    compute_message(message, "");

    // Step 6: add the actual payload data
    for (int i = 0; i < body_data_fields_count; i++)
    {
        strcat(body_data_buffer, body_data[i]);
        strcat(body_data_buffer, "&");
    }

    if (body_data_fields_count > 0)
    {
        body_data_buffer[strlen(body_data_buffer) - 1] = '\0';
    }

    compute_message(message, body_data_buffer);

    free(body_data_buffer);
    free(line);
    return message;
}

// functie extra pe care o folosesc pentru a actualiza sursa existenta, iar in cazul meu
// la update_movie pentru a actualiza un film
char *compute_put_request(char *host, char *url, char *content_type,
                          char **body_data, int body_data_fields_count,
                          char **cookies, int cookies_count, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));
    char *body_data_buffer = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "PUT %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    if (token)
    {
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }
    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    strcpy(body_data_buffer, body_data[0]);

    for (int i = 1; i < body_data_fields_count; i++)
    {
        strcat(body_data_buffer, "&");
        strcat(body_data_buffer, body_data[i]);
    }

    int content_length = strlen(body_data_buffer);
    sprintf(line, "Content-Length: %d", content_length);
    compute_message(message, line);
    // Step 4 (optional): add cookies
    memset(line, 0, LINELEN);
    if (cookies != NULL)
    {
        sprintf(line, "Cookie: ");

        for (int i = 0; i < cookies_count; i++)
        {
            sprintf(line + strlen(line), "%s; ", cookies[i]);
        }

        if (cookies_count > 0)
        {
            line[strlen(line) - 1] = '\0';
            line[strlen(line) - 1] = '\0';
        }

        compute_message(message, line);
    }
    // Step 5: add new line at end of header
    compute_message(message, "");

    // Step 6: add the actual payload data
    compute_message(message, body_data_buffer);
    free(body_data_buffer);
    free(line);
    return message;
}

char *compute_delete_request(char *host, char *url, char *content_type, char **cookies, int cookies_count, char *token)
{
    char *message = calloc(BUFLEN, sizeof(char));
    char *line = calloc(LINELEN, sizeof(char));

    // Step 1: write the method name, URL and protocol type
    sprintf(line, "DELETE %s HTTP/1.1", url);
    compute_message(message, line);

    // Step 2: add the host
    memset(line, 0, LINELEN);
    sprintf(line, "Host: %s", host);
    compute_message(message, line);

    /* Step 3: add necessary headers (Content-Type and Content-Length are mandatory)
            in order to write Content-Length you must first compute the message size
    */
    memset(line, 0, LINELEN);
    sprintf(line, "Content-Type: %s", content_type);
    compute_message(message, line);

    memset(line, 0, LINELEN);
    sprintf(line, "Content-Length: %d", 0);
    compute_message(message, line);
    // if we have bearer token, add it too
    if (token != NULL)
    {
        memset(line, 0, LINELEN);
        sprintf(line, "Authorization: Bearer %s", token);
        compute_message(message, line);
    }
    // Step 4 (optional): add cookies
    memset(line, 0, LINELEN);
    if (cookies != NULL)
    {
        strcat(line, "Cookie: ");
        for (int i = 0; i < cookies_count; i++)
        {
            strcat(line, cookies[i]);
            if (i != cookies_count - 1)
            {
                strcat(line, ";");
            }
        }
        compute_message(message, line);
    }
    // Step 5: add new line at end of header
    compute_message(message, "");
    // Step 6: add the actual payload data
    memset(line, 0, LINELEN);

    free(line);
    return message;
}