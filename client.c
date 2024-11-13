#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

char* createJson() {
    char* name = calloc(100, sizeof(char));
    char* pass = calloc(100, sizeof(char));
    JSON_Value *value = json_value_init_object();
	JSON_Object *object = json_value_get_object(value);

    printf("username=");
	scanf("%s", name);
    json_object_set_string(object, "username", name);

	printf("password=");
	scanf("%s", pass);
    json_object_set_string(object, "password", pass);

    return json_serialize_to_string(value);
}

char* errorMessage(char* string) {
    char* error = basic_extract_json_response(string);
    error += 10;
    error[strlen(error) - 2] = 0;

    return error;
}

int main(int argc, char *argv[])
{
    char *message = calloc(100, sizeof(char));
    char *response = calloc(100, sizeof(char));
    char* nume_comanda = calloc(100, sizeof(char));
    int sockfd;
    int acces = 0;
    char *token = calloc(100, sizeof(char));

    char **data = calloc(1, sizeof(char *));
    data[0] = calloc(LINELEN, sizeof(char));

    char **cookies = calloc(1, sizeof(char*));
    cookies[0] = calloc(LINELEN, sizeof(char));


    while (1) {
        fgets(nume_comanda, 100, stdin);
        sockfd = open_connection("34.241.4.235", 8080, AF_INET, SOCK_STREAM, 0);

        if (strncmp(nume_comanda, "exit", 4) == 0) {
            printf("You have disconnected from the server!\n");
            break;
        }

        else if (strncmp(nume_comanda, "register", 8) == 0) {
            memset(data[0], 0, LINELEN);
            data[0] = createJson();

            message = compute_post_request("34.241.4.235", "/api/v1/tema/auth/register", "application/json", data, 1, NULL, 0, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            
            if (strstr(response, "error") == NULL) {
                printf("Registered!\n"); 
            } else {
                printf("%s\n", errorMessage(response));
            }
        } 

        else if (strncmp(nume_comanda, "login", 5) == 0) {
            memset(data[0], 0, LINELEN);
            data[0] = createJson();

            message = compute_post_request("34.241.4.235", "/api/v1/tema/auth/login", "application/json", data, 1, NULL, 0, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
                       
            if (strstr(response, "Set-Cookie:") == NULL) {
                printf("%s\n", errorMessage(response));
            } else {
                char* cookie_repsone = strstr(response, "Set-Cookie");
                strtok(cookie_repsone, ";");
                cookie_repsone += 12;
                cookies[0] = cookie_repsone;

                printf("Logged in!\n");
            }
        }

        else if (strncmp(nume_comanda, "enter_library", 13) == 0) {
            acces = 1;

            message = compute_get_request("34.241.4.235", "/api/v1/tema/library/access", NULL, cookies, 1, token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);

            if (strstr(response, "token") == NULL) {
                acces = 0;
                printf("%s\n", errorMessage(response));
            } else {
                char* received_token = strstr(response, "token");

                strtok(received_token, "\":");
                char* saved_token = strtok(NULL, "\":");

                strcpy(token, saved_token);
                printf("You have access to the library now!\n");
            }
        }

        else if (strncmp(nume_comanda, "get_books", 9) == 0) {
            message = compute_get_request("34.241.4.235", "/api/v1/tema/library/books", NULL, cookies, 1, token);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);

            if (acces == 0) {
                printf("You need access to the library\n");
            } else {
                printf("Books: %s\n", strstr(response, "["));
            }
        }

        else if (strncmp(nume_comanda, "get_book", 8) == 0) {
            if (acces == 0) {
                printf("You need access to the library\n");
            } else {
                int id;
                printf("id=");
                scanf("%d", &id);

                char* path = calloc(LINELEN, sizeof(char));
                sprintf(path, "%s%d", "/api/v1/tema/library/books/", id);

                message = compute_get_request("34.241.4.235", path, NULL, cookies, 1, token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);

                if (strstr(response, "error") != NULL) {
                    printf("%s\n", errorMessage(response));
                } else {
                    printf("%s\n", strstr(response, "["));
                }

            }
        }

        else if (strncmp(nume_comanda, "add_book", 8) == 0) {
            if (acces == 0) {
                printf("You need access to the library\n");
            } else {
                JSON_Value *value = json_value_init_object();
                JSON_Object *object = json_value_get_object(value);
                char title[100];
                char author[100];
                char genre[100];
                char publisher[100];
                int page_count;

                printf("title="); 
                fgets(title, 100, stdin);
                title[strlen(title) - 1] = 0;
                json_object_set_string(object, "title", title);

                printf("author="); 
                fgets(author, 100, stdin);
                author[strlen(author) - 1] = 0;
                json_object_set_string(object, "author", author);

                printf("genre="); 
                fgets(genre, 100, stdin);
                genre[strlen(genre) - 1] = 0;
                json_object_set_string(object, "genre", genre);

                printf("publisher="); 
                fgets(publisher, 100, stdin);
                publisher[strlen(publisher) - 1] = 0;
                json_object_set_string(object, "publisher", publisher);

                printf("page_count="); 
                scanf("%d", &page_count);
                json_object_set_number(object, "page_count", page_count);

                char* book = NULL;
                book = json_serialize_to_string(value);

                message = compute_post_request("34.241.4.235", "/api/v1/tema/library/books", "application/json", &book, 1, NULL, 0, token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);

                if (strstr(response, "error") != NULL) {
                    printf("%s\n", errorMessage(response));
                } else {
                    printf("Book aded!\n");
                }
            }
          
        }

        else if (strncmp(nume_comanda, "delete_book", 11) == 0) {
            if (acces == 0) {
                printf("You need access to the library\n");
            } else {
                int id;
                printf("id=");
                scanf("%d", &id);

                char* path = calloc(LINELEN, sizeof(char));
                sprintf(path, "%s%d", "/api/v1/tema/library/books/", id);

                message = compute_delete_request("34.241.4.235", path, NULL, cookies, 1, token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);

                if (strstr(response, "error") != NULL) {
                    printf("%s\n", errorMessage(response));
                } else {
                    printf("Book was deleted!\n");
                }

            }
        } 

        else if (strncmp(nume_comanda, "logout", 6) == 0) {
                message = compute_get_request("34.241.4.235", "/api/v1/tema/auth/logout", NULL, cookies, 1, token);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                
                if (strstr(response, "error") != NULL) {
                    printf("%s\n", errorMessage(response));
                } else {
                    acces = 0;
                    printf("Logged out!\n");
                }
        }
        close(sockfd);
    }
        // free(message);
        // free(response);
        // free(data[0]);
        // free(data);
        // free(cookies[0]);
        // free(cookies);

    return 0;
}
