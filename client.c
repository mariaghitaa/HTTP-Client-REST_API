#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include <stdbool.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

#define HOST "63.32.125.183"
#define PORT 8081
#define PAYLOAD_TYPE "application/json"
#define REGISTER_LOG "/api/v1/tema/admin/login"
#define ADD_USER "/api/v1/tema/admin/users"
#define LOGOUT_ADMIN "/api/v1/tema/admin/logout"
#define USER_LOGIN "/api/v1/tema/user/login"
#define GET_USERS "/api/v1/tema/admin/users"
#define GET_ACCESS "/api/v1/tema/library/access"
#define GET_MOVIES "/api/v1/tema/library/movies"
#define LOGOUT_USER "/api/v1/tema/user/logout"
#define GET_COLLECTIONS "/api/v1/tema/library/collections"

// variabilele globale pentru a stoca cookie-ul de sesiune
// token-ul de acces si niste flaguri pentru a verifica tipul de logare(admin sau user)
char *cookie = NULL;
char *token = NULL;
bool is_admin = false;
bool is_user = false;

// functie care extrage si actualizeaza cookie-ul de sesiune pentrun cererile
// urmatoare catre server in cadrul functiilor
void update_cookie(char *response);

void login_admin()
{
    if (cookie != NULL)
    {
        printf("ERROR: Sunteti deja logat ca admin!\n");
        return;
    }
    char *message = NULL;
    char *response = NULL;
    int sockfd;
    char username[1000], password[1000];
    // citesc datele de inregistrare
    printf("username=");
    scanf("%s", username);
    printf("password=");
    scanf("%s", password);

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    json_object_set_string(obj, "username", username);
    json_object_set_string(obj, "password", password);

    // creez un obiect de tip JSON cu campurile necesare
    char *s = json_serialize_to_string(val);
    // creez mesajul si deschid conexiunea
    message = compute_post_request(HOST, REGISTER_LOG, PAYLOAD_TYPE, &s, 1, NULL, 0, NULL);
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    // trimit mesajul, primesc raspuns inchid conexiunea
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);
    // in functie de tipul raspunsului de la server informez user ul
    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Adminul nu s-a putut loga!\n");
    }
    else
    {
        printf("SUCCESS: Admin logat cu succes!\n");
        is_admin = true;
        update_cookie(response);
    }
    json_free_serialized_string(s);
    json_value_free(val);
    // eliberez memoria alocata
    free(message);
    free(response);
}

void add_user()
{
    // verific daca sunt logat ca admin
    if (cookie == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    // citesc datele de inregistrare
    char *message = NULL;
    char *response = NULL;
    int sockfd;
    char username[1000], password[1000];

    printf("username=");
    scanf("%s", username);
    printf("password=");
    scanf("%s", password);

    JSON_Value *user_val = json_value_init_object();
    JSON_Object *user_obj = json_value_get_object(user_val);
    json_object_set_string(user_obj, "username", username);
    json_object_set_string(user_obj, "password", password);

    char *s = json_serialize_to_string(user_val);
    message = compute_post_request(HOST, ADD_USER, PAYLOAD_TYPE, &s, 1, &cookie, 1, NULL);
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut adauga utilizatorul!\n");
    }
    else
    {
        printf("SUCCESS: Utilizator adaugat cu succes!\n");
    }

    json_free_serialized_string(s);
    json_value_free(user_val);
    free(message);
    free(response);
}

void get_users()
{
    if (cookie == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    char *message = compute_get_request(HOST, GET_USERS, NULL, &cookie, 1, token);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    // actualizez cookie-ul  de sesiune
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-au putut obtine utilizatorii!\n");
    }
    else
    {
        printf("SUCCESS: Utilizatori obtinuti cu succes!\n");
        char *json_str = basic_extract_json_response(response);
        if (json_str != NULL)
        {
            // extrag datele din JSON si le afisez pentru fiecare user din lista
            JSON_Value *val = json_parse_string(json_str);
            JSON_Object *obj = json_value_get_object(val);
            JSON_Array *users = json_object_get_array(obj, "users");
            int k = json_array_get_count(users);
            // afisez datele pentru fiecare user
            for (int i = 0; i < k; i++)
            {
                // extrag datele pentru fiecare user
                JSON_Object *user_obj = json_array_get_object(users, i);
                const char *username = json_object_get_string(user_obj, "username");
                const char *password = json_object_get_string(user_obj, "password");
                if (username && password)
                    printf("#%d %s:%s\n", i + 1, username, password);
            }
            json_value_free(val);
        }
        else
        {
            printf("ERROR: Nu exista JSON valid!\n");
        }
    }

    free(message);
    free(response);
}

void delete_user()
{
    if (cookie == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }

    char username[1000];
    printf("username=");
    scanf("%s", username);

    char url[1100];
    snprintf(url, sizeof(url), "/api/v1/tema/admin/users/%s", username);

    char *message = NULL;
    char *response = NULL;
    int sockfd;

    message = compute_delete_request(HOST, url, NULL, &cookie, 1, token);
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut sterge utilizatorul!\n");
    }
    else
    {
        printf("SUCCESS: Utilizator sters cu succes!\n");
    }

    free(message);
    free(response);
}

void logout_admin()
{
    if (cookie == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }

    int sockfd;
    char *message = NULL;
    char *response = NULL;

    // cerere GET pentru logout admin
    message = compute_get_request(HOST, LOGOUT_ADMIN, NULL, &cookie, 1, NULL);
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Adminul nu s-a putut deconecta\n");
    }
    else
    {
        printf("SUCCESS: Admin deconectat cu succes!\n");
        // eliberez cookie-ul de sesiune
        free(cookie);
        cookie = NULL;
        // eliberez si token-ul de acces
        free(token);
        token = NULL;
        // resetez flagurile de logare pentru a fi folosite la urmatoarea logare
        is_admin = is_user = false;
    }
    free(message);
    free(response);
}

void login(void)
{
    char admin_username[1000], username[1000], password[1000];
    printf("admin_username=");
    scanf("%s", admin_username);
    printf("username=");
    scanf("%s", username);
    printf("password=");
    scanf("%s", password);

    JSON_Value *user_val = json_value_init_object();
    JSON_Object *user_obj = json_value_get_object(user_val);
    json_object_set_string(user_obj, "admin_username", admin_username);
    json_object_set_string(user_obj, "username", username);
    json_object_set_string(user_obj, "password", password);

    char *s = json_serialize_to_string(user_val);
    char *message = compute_post_request(HOST, USER_LOGIN, PAYLOAD_TYPE, &s, 1, NULL, 0, NULL);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Utiliziatorul nu s-a putut conecta!\n");
    }
    else
    {
        printf("SUCCESS: Utilizator autentificat!\n");
        is_user = true;
        // salvez cookie‑ul de user si actualizez
        update_cookie(response);
    }
    json_free_serialized_string(s);
    json_value_free(user_val);
    free(message);
    free(response);
}

void get_access()
{
    if (is_user == false)
    {
        printf("ERROR: Trebuie sa va conectati ca user mai intai!\n");
        return;
    }
    // {
    //     printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
    //     return;
    // }
    char *message = NULL;
    char *response = NULL;
    int sockfd;
    // cerere GET pentru a obtine accesul la biblioteca
    message = compute_get_request(HOST, GET_ACCESS, NULL, &cookie, 1, NULL);
    // deschid conexiunea, triit cererea si primesc raspunsul
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);
    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut obtine accesul!\n");
    }
    else
    {
        // extrag JSON-ul din raspuns
        char *json_str = basic_extract_json_response(response);
        if (json_str == NULL)
        {
            printf("ERROR: NU exista un JSON valid!\n");
        }
        else
        {
            // parsez JSON-ul si extrag token-ul JWT
            JSON_Value *json_val = json_parse_string(json_str);
            if (json_val == NULL)
            {
                printf("ERROR: Nu s-a putut parsa JSON pentru token!\n");
            }
            else
            {
                JSON_Object *json_obj = json_value_get_object(json_val);
                if (json_obj == NULL)
                {
                    printf("ERROR: Structura JSON este invalida pentru token!\n");
                }
                else
                {
                    // extrag token-ul JWT din JSON
                    const char *jwt_token = json_object_get_string(json_obj, "token");
                    if (jwt_token == NULL)
                    {
                        printf("ERROR: Tokenul JWT nu a fost gasit!\n");
                    }
                    else
                    {
                        // salvez token-ul din variabila globala si eliberez vechiul daca exista
                        if (token != NULL)
                            free(token);
                        token = strdup(jwt_token); // salveaza token ul
                        printf("SUCCESS: Token JWT primit!\n");
                    }
                }
                json_value_free(json_val);
            }
        }
    }
    free(message);
    free(response);
}

void get_movies()
{
    if (cookie == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    if (token == NULL)
    {
        printf("ERROR: Nu ai acces la biblioteca!\n");
        return;
    }
    char *message = NULL;
    char *response = NULL;
    int sockfd;
    // comunic cu serverul
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(HOST, GET_MOVIES, NULL, &cookie, 1, token);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);
    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-au putut obtine filmele!\n");
    }
    else
    {
        printf("SUCCESS: Lista filmelor:\n");
        char *str = basic_extract_json_response(response);
        JSON_Value *val = json_parse_string(str);
        JSON_Object *obj = json_value_get_object(val);
        JSON_Array *movies = json_object_get_array(obj, "movies");
        int k = json_array_get_count(movies);
        // afisez filmele
        for (int i = 0; i < k; i++)
        {
            JSON_Object *movie = json_array_get_object(movies, i);
            const char *title = json_object_get_string(movie, "title");
            int id = (int)json_object_get_number(movie, "id");
            printf("#%d %s\n", id, title);
        }
        json_value_free(val);
    }
    free(message);
    free(response);
}

void delete_movie()
{
    // verific daca utilizatorul este logat ca admin si are token de acces la biblioteca
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    // id-ul filmului
    char id_str[100];
    printf("id=");
    scanf("%s", id_str);
    // construiesc url-ul(calea resursei de pe server) pentru stergerea filmului
    char url[2000];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%s", id_str);
    int sockfd;
    char *message = NULL;
    char *response = NULL;
    // cererea HTTP-DELETE pentru stergerea filmului
    message = compute_delete_request(HOST, url, PAYLOAD_TYPE, &cookie, 1, token);
    // deschid conexiunea cu serverul
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    // trimit cererea catre server
    send_to_server(sockfd, message);
    // primesc raspunsul de la server
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    // actualizez cookie-ul de sesiune daca este cazul
    update_cookie(response);
    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut sterge filmul\n");
    }
    else
    {
        printf("SUCCESS: Film sters cu succes!\n");
    }
    free(message);
    free(response);
}

void get_movie()
{
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    char id_str[100];
    printf("id=");

    scanf("%s", id_str);
    char url[2000];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%s", id_str);
    int sockfd;
    char *message = NULL;
    char *response = NULL;
    message = compute_get_request(HOST, url, NULL, &cookie, 1, token);
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);
    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut obtine filmul!\n");
    }
    else
    {
        printf("SUCCESS: Film:\n");
        // extrag JSON-ul din raspuns
        char *json_str = basic_extract_json_response(response);
        JSON_Value *json_val = json_parse_string(json_str);
        if (json_val)
        {
            JSON_Object *movie = json_value_get_object(json_val);
            if (movie != NULL)
            {
                // extrag datele filmului si le afisez
                const char *title = json_object_get_string(movie, "title");
                int year = (int)json_object_get_number(movie, "year");
                const char *description = json_object_get_string(movie, "description");
                const char *rating = json_object_get_string(movie, "rating");
                printf("title: %s\n", title);
                printf("year: %d\n", year);
                printf("description: %s\n", description);
                printf("rating: %s\n", rating);
            }
            json_value_free(json_val);
        }
    }
    free(message);
    free(response);
}

void add_movie()
{
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    char title[1000], year_str[50], description[2000], rating_str[50];
    // am folosit o asemenea citire pentru a putea citi si spatiile din titlu si descriere
    printf("title=");
    scanf(" %[^\n]", title);
    printf("year=");
    scanf("%s", year_str);
    printf("description=");
    scanf(" %[^\n]", description);
    printf("rating=");
    scanf("%s", rating_str);

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    json_object_set_string(obj, "title", title);
    json_object_set_number(obj, "year", atoi(year_str));
    json_object_set_string(obj, "description", description);
    json_object_set_number(obj, "rating", atof(rating_str));
    char *s = json_serialize_to_string(val);
    char *req = compute_post_request(HOST, GET_MOVIES, PAYLOAD_TYPE, &s, 1, &cookie, 1, token);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, req);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Filmul nu s-a putut adauga!\n");
    }
    else
    {
        printf("SUCCESS: Filmul a fost adaugat!\n");
    }
    json_free_serialized_string(s);
    json_value_free(val);
    free(req);
    free(response);
}

void update_movie()
{
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }

    char id_str[50], title[1000], year_str[50], description[2000], rating_str[50];
    printf("id=");
    scanf("%s", id_str);
    printf("title=");
    scanf(" %[^\n]", title);
    printf("year=");
    scanf("%s", year_str);
    printf("description=");
    scanf(" %[^\n]", description);
    printf("rating=");
    scanf("%s", rating_str);

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    json_object_set_string(obj, "title", title);
    json_object_set_number(obj, "year", atoi(year_str));
    json_object_set_string(obj, "description", description);
    json_object_set_number(obj, "rating", atof(rating_str));
    char *s = json_serialize_to_string(val);
    char url[128];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%s", id_str);
    // folosesc cererea PUT pentru a actualiza filmul(singurul loc in care am folosit-o)
    // in rest am folosit DELETE pentru stergere si POST pentru adaugare
    char *req = compute_put_request(HOST, url, PAYLOAD_TYPE, &s, 1, &cookie, 1, token);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, req);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);
    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut actualiza filmul\n");
    }
    else
    {
        printf("SUCCESS: Film actualizat\n");
    }

    json_free_serialized_string(s);
    json_value_free(val);
    free(req);
    free(response);
}

void get_collections()
{
    if (cookie == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    if (token == NULL)
    {
        printf("ERROR: Nu aveti acces la biblioteca!\n");
        return;
    }
    int sockfd;
    char *message = NULL;
    char *response = NULL;
    message = compute_get_request(HOST, GET_COLLECTIONS, NULL, &cookie, 1, token);
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-au putut obtine colectiile!\n");
    }
    else
    {
        printf("SUCCESS: Lista colectiilor:\n");

        // extrag JSON ul
        char *json_str = basic_extract_json_response(response);
        JSON_Value *val = json_parse_string(json_str);
        JSON_Object *obj = json_value_get_object(val);
        JSON_Array *arr = json_object_get_array(obj, "collections");
        int k = json_array_get_count(arr);
        for (int i = 0; i < k; i++)
        {
            JSON_Object *obj = json_array_get_object(arr, i);
            int id = (int)json_object_get_number(obj, "id");
            const char *title = json_object_get_string(obj, "title");
            printf("#%d: %s\n", id, title);
        }

        json_value_free(val);
    }

    free(message);
    free(response);
}

void get_collection()
{
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    char id_str[100];
    printf("id=");

    scanf("%s", id_str);
    char url[2000];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%s", id_str);
    int sockfd;
    char *message = NULL;
    char *response = NULL;
    message = compute_get_request(HOST, url, NULL, &cookie, 1, token);
    sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut obtine colectia!\n");
    }
    else
    {
        printf("SUCCESS: Detalii colectie\n");
        char *json_str = basic_extract_json_response(response);
        JSON_Value *val = json_parse_string(json_str);
        JSON_Object *obj = json_value_get_object(val);
        printf("title: %s\n", json_object_get_string(obj, "title"));
        printf("owner: %s\n", json_object_get_string(obj, "owner"));
        JSON_Array *movies = json_object_get_array(obj, "movies");
        int k = json_array_get_count(movies);
        for (int i = 0; i < k; i++)
        {
            JSON_Object *movie = json_array_get_object(movies, i);
            int id = (int)json_object_get_number(movie, "id");
            const char *title = json_object_get_string(movie, "title");
            printf("#%d: %s\n", id, title);
        }
        json_value_free(val);
    }

    free(message);
    free(response);
}

// functie cu ajutorul careia verific daca filmul exista sau nu in baza de date
// a serverului, returneaza 1 daca exista si 0 daca nu
// am folosit-o pentru a verifica daca filmele dintr-o colectie sunt valide
int exist_film(int id)
{
    char url[2000];
    snprintf(url, sizeof(url), "/api/v1/tema/library/movies/%d", id);
    char *req = compute_get_request(HOST, url, NULL, &cookie, 1, token);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, req);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    int exist = 0;
    if (strstr(response, "error") == NULL)
        exist = 1;
    free(req);
    free(response);
    return exist;
}

void add_collection()
{
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }
    char title[100];
    printf("title=");
    scanf(" %[^\n]", title);
    // eroare speciala tratata pentru a nu permite titluri goale
    if (strlen(title) == 0)
    {
        printf("ERROR: Titlul nu poate fi gol!\n");
        return;
    }
    // variabile pentru a citi filmele din colectie
    // am folosit un vector pentru a citi id-urile filmelor
    // si un contor pentru a numara cate filme sunt in colectie
    int num_movies;
    printf("num_movies=");
    scanf("%d", &num_movies);
    int movie_ids[100];
    for (int i = 0; i < num_movies; i++)
    {
        printf("movie_id[%d]=", i);
        scanf("%d", &movie_ids[i]);
    }

    int invalid_ids[100];
    int invalid_cnt = 0;
    int valid_cnt = 0;
    // verific daca filmele sunt valide si le numar pe cele valide
    for (int i = 0; i < num_movies; i++)
    {
        if (exist_film(movie_ids[i]))
            valid_cnt++;
        else
            invalid_ids[invalid_cnt++] = movie_ids[i];
    }
    // nu am gasit filme valide
    if (valid_cnt == 0)
    {
        printf("ERROR: Nu exista filme valide!\n");
        return;
    }
    // afisez id-urile filmelor invalide
    if (invalid_cnt > 0)
    {
        printf("ERROR: Filmele cu id-uri invalide: ");
        for (int i = 0; i < invalid_cnt; i++)
        {
            printf("%d ", invalid_ids[i]);
            if (i < invalid_cnt - 1)
                printf(", ");
        }
        printf("\n");
    }
    // creez un obiect JSON cu titlul colectiei
    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    json_object_set_string(obj, "title", title);
    char *s = json_serialize_to_string(val);
    char *req = compute_post_request(HOST, GET_COLLECTIONS, PAYLOAD_TYPE, &s, 1, &cookie, 1, token);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, req);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut adauga colectia!\n");
    }
    else
    {
        printf("SUCCESS: Colectie adaugata cu succes!\n");
        // extrag id-ul colectiei nou create
        char *c_str = basic_extract_json_response(response);
        JSON_Value *cval = json_parse_string(c_str);
        JSON_Object *cobj = json_value_get_object(cval);
        int collection_id = (int)json_object_get_number(cobj, "id");
        json_value_free(cval);
        // pentru fiecare film din colectie trimit cerere POST pentru a-l adauga in colectie
        for (int i = 0; i < num_movies; i++)
        {
            JSON_Value *mval = json_value_init_object();
            JSON_Object *mobj = json_value_get_object(mval);
            json_object_set_number(mobj, "id", movie_ids[i]);
            char *str = json_serialize_to_string(mval);

            char url[128];
            snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%d/movies", collection_id);

            char *mreq = compute_post_request(HOST, url, PAYLOAD_TYPE, &str, 1, &cookie, 1, token);
            sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, mreq);
            char *mresp = receive_from_server(sockfd);
            close_connection(sockfd);

            free(mreq);
            free(mresp);
            json_free_serialized_string(str);
            json_value_free(mval);
        }
        json_value_free(val);
    }
    free(req);
    free(response);
    json_free_serialized_string(s);
}

void delete_collection()
{
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }

    char id_str[100];
    printf("id=");
    scanf("%s", id_str);

    char url[2000];
    snprintf(url, sizeof(url), "/api/v1/tema/library/collections/%s", id_str);

    char *message = compute_delete_request(HOST, url, PAYLOAD_TYPE, &cookie, 1, token);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut sterge colectia!\n");
    }
    else
    {
        printf("SUCCESS: Colectie stearsa cu succes!\n");
    }
    free(message);
    free(response);
}

void add_movie_to_collection()
{
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }

    char collection_id[32], movie_id[32];
    printf("collection_id=");
    scanf("%s", collection_id);
    printf("movie_id=");
    scanf("%s", movie_id);

    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);
    json_object_set_number(obj, "id", atoi(movie_id));
    char *s = json_serialize_to_string(val);
    char url[2000];
    snprintf(url, 2000, "/api/v1/tema/library/collections/%s/movies", collection_id);

    // trimite cerere POST
    char *message = compute_post_request(HOST, url, PAYLOAD_TYPE, &s, 1, &cookie, 1, token);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut adauga filmul in colectie\n!");
    }
    else
    {
        printf("SUCCESS: Film adaugat in colectie!\n");
    }
    free(message);
    free(response);
    json_free_serialized_string(s);
    json_value_free(val);
}

void delete_movie_from_collection()
{
    if (cookie == NULL || token == NULL)
    {
        printf("ERROR: Trebuie sa va conectati ca admin mai intai!\n");
        return;
    }

    char collection_id[100], movie_id[100];

    printf("collection_id=");
    scanf("%s", collection_id);
    printf("movie_id=");
    scanf("%s", movie_id);

    char url[2000];
    snprintf(url, 2000, "/api/v1/tema/library/collections/%s/movies/%s", collection_id, movie_id);

    char *message = compute_delete_request(HOST, url, PAYLOAD_TYPE, &cookie, 1, token);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Nu s-a putut sterge filmul din colectie!\n");
    }
    else
    {
        printf("SUCCESS: Film sters din colectie!!\n");
    }
    free(message);
    free(response);
}

void logout()
{
    if (cookie == NULL)
    {
        printf("ERROR: Nu esti logat ca utilizator!\n");
        return;
    }
    char *message = compute_get_request(HOST, LOGOUT_USER, NULL, &cookie, 1, NULL);
    int sockfd = open_connection(HOST, PORT, AF_INET, SOCK_STREAM, 0);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    close_connection(sockfd);
    update_cookie(response);

    if (strstr(response, "error") != NULL)
    {
        printf("ERROR: Utilizatorul nu s-a putut deconecta!\n");
    }
    else
    {
        printf("SUCCESS: Utiizatorul s-a deconectat cu succes!\n");
        is_user = false;
        free(cookie);
        cookie = NULL;
        if (token != NULL)
        {
            free(token);
            token = NULL;
        }
    }
}

// functie suplimentara pentru a extrage si actualiza cookie-ul din raspunsul HTTP
void update_cookie(char *response)
{
    // cauta in raspunsul HTTP pentru Set-Cookie
    char *aux = strstr(response, "Set-Cookie: ");
    if (aux != NULL)
    {
        // extrage valoarea cookie-ului si actualizeaza variabila globala cu noua valoare
        aux = aux + strlen("Set-Cookie: ");
        char *end = strchr(aux, ';');
        size_t len;
        if (end)
            len = (size_t)(end - aux);
        else
            len = strlen(aux);
        char *new_cookie = (char *)calloc(len + 1, 1);
        if (new_cookie != NULL)
        {
            memcpy(new_cookie, aux, len);
            new_cookie[len] = '\0';
            if (cookie)
                free(cookie);
            cookie = new_cookie;
        }
    }
}

int main(int argc, char *argv[])
{
    char command[1000];
    while (1)
    {
        if (scanf("%s", command) != 1)
        {
            break;
        }

        if (strcmp(command, "login_admin") == 0)
        {
            login_admin();
        }
        else if (strcmp(command, "logout_admin") == 0)
        {
            logout_admin();
        }
        else if (strcmp(command, "add_user") == 0)
        {
            add_user();
        }
        else if (strcmp(command, "get_users") == 0)
        {
            get_users();
        }
        else if (strcmp(command, "delete_user") == 0)
        {
            delete_user();
        }
        else if (strcmp(command, "login") == 0)
        {
            login();
        }
        else if (strcmp(command, "logout") == 0)
        {
            logout();
        }
        else if (strcmp(command, "get_access") == 0)
        {
            get_access();
        }
        else if (strcmp(command, "get_movies") == 0)
        {
            get_movies();
        }
        else if (strcmp(command, "get_movie") == 0)
        {
            get_movie();
        }
        else if (strcmp(command, "add_movie") == 0)
        {
            add_movie();
        }
        else if (strcmp(command, "delete_movie") == 0)
        {
            delete_movie();
        }
        else if (strcmp(command, "update_movie") == 0)
        {
            update_movie();
        }
        else if (strcmp(command, "get_collections") == 0)
        {
            get_collections();
        }
        else if (strcmp(command, "get_collection") == 0)
        {
            get_collection();
        }
        else if (strcmp(command, "add_collection") == 0)
        {
            add_collection();
        }
        else if (strcmp(command, "delete_collection") == 0)
        {
            delete_collection();
        }
        else if (strcmp(command, "add_movie_to_collection") == 0)
        {
            add_movie_to_collection();
        }
        else if (strcmp(command, "delete_movie_from_collection") == 0)
        {
            delete_movie_from_collection();
        }
        else if (strcmp(command, "exit") == 0)
        {
            free(cookie);
            free(token);
            break;
        }
        else
        {
            printf("Unknown command\n");
        }
    }

    return 0;
}