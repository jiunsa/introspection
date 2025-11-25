#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <regex.h>
#include <time.h>

#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define BOLD "\033[1m"
#define BLACK_TEXT "\033[30m"
#define YELLOW_BACKGROUND "\033[43m"
#define GREEN_BACKGROUND "\033[42m"
#define RESET "\033[0m"

#define MAX_THREADS 50
#define MAX_URL_LENGTH 2048
#define MAX_WORD_LENGTH 256
#define MAX_WORDLIST_SIZE 10000
#define MAX_USER_AGENTS 32

int score_200 = 0;
int score_301 = 0;
int score_403 = 0;
int total_words = 0;
int processed_words = 0;
pthread_mutex_t print_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t progress_lock = PTHREAD_MUTEX_INITIALIZER;

const char *key_words[] = {
    "admin", "login", "private", "administration", "secure",
    "wp", "wp-admin", "wp-login", "prive", "robot",
    ".htaccess", ".htpassword", "passwd", ".ht", NULL};

const char *user_agents[] = {
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.1234.56 Safari/537.36",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.1234.56 Safari/537.36",
    "Mozilla/5.0 (Linux; Android 10; Pixel 3) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/100.0.1234.56 Mobile Safari/537.36",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:100.0) Gecko/20100101 Firefox/100.0",
    "Mozilla/5.0 (X11; Linux x86_64; rv:109.0) Gecko/20100101 Firefox/116.0",
    "Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:100.0) Gecko/20100101 Firefox/100.0",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36 Edg/115.0.1901.188",
    "Mozilla/5.0 (iPhone; CPU iPhone OS 15_0 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.0 Mobile/15E148 Safari/604.1",
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/116.0.0.0 Safari/537.36",
    NULL};

typedef struct
{
    char **wordlist;
    int start;
    int end;
    char *base_url;
} ThreadData;

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    (void)contents;
    (void)userp;
    return size * nmemb;
}

int contains_keyword(const char *word)
{
    for (int i = 0; key_words[i] != NULL; i++)
    {
        if (strstr(word, key_words[i]) != NULL)
        {
            return 1;
        }
    }
    return 0;
}

const char *get_random_user_agent()
{
    int count = 0;
    while (user_agents[count] != NULL)
        count++;
    return user_agents[rand() % count];
}

void update_progress()
{
    pthread_mutex_lock(&progress_lock);
    processed_words++;
    int percent = (processed_words * 100) / total_words;
    printf("\rProgression: %d%% [%d/%d]", percent, processed_words, total_words);
    fflush(stdout);
    pthread_mutex_unlock(&progress_lock);
}

void *scan_url(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    CURL *curl;
    CURLcode res;
    long response_code;

    curl = curl_easy_init();
    if (!curl)
    {
        return NULL;
    }

    for (int i = data->start; i < data->end; i++)
    {
        char url[MAX_URL_LENGTH];
        char word_display[MAX_WORD_LENGTH * 2];

        snprintf(url, sizeof(url), "%s%s", data->base_url, data->wordlist[i]);

        if (contains_keyword(data->wordlist[i]))
        {
            snprintf(word_display, sizeof(word_display), "%s%s%s%s",
                     YELLOW_BACKGROUND, BLACK_TEXT, data->wordlist[i], RESET);
        }
        else
        {
            snprintf(word_display, sizeof(word_display), "%s", data->wordlist[i]);
        }

        struct curl_slist *headers = NULL;
        char user_agent_header[512];
        snprintf(user_agent_header, sizeof(user_agent_header),
                 "User-Agent: %s", get_random_user_agent());
        headers = curl_slist_append(headers, user_agent_header);
        headers = curl_slist_append(headers, "Accept: */*");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);

        if (res == CURLE_OK)
        {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

            pthread_mutex_lock(&print_lock);
            if (response_code == 200)
            {
                printf("\r\033[K%s200 OK : %s%s%s\n", GREEN, data->base_url, word_display, RESET);
                score_200++;
            }
            else if (response_code == 301 || response_code == 302)
            {
                printf("\r\033[K%s%ld REDIRECT : %s%s%s\n", BLUE, response_code, data->base_url, word_display, RESET);
                score_301++;
            }
            else if (response_code == 403)
            {
                printf("\r\033[K%s403 RESTRICTED : %s%s%s\n", RED, data->base_url, word_display, RESET);
                score_403++;
            }
            pthread_mutex_unlock(&print_lock);
        }

        curl_slist_free_all(headers);
        update_progress();
    }

    curl_easy_cleanup(curl);
    return NULL;
}

int load_wordlist(const char *filename, char ***wordlist)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "%sError: Cannot open wordlist file%s\n", RED, RESET);
        return 0;
    }

    *wordlist = malloc(MAX_WORDLIST_SIZE * sizeof(char *));
    if (!*wordlist)
    {
        fclose(file);
        return 0;
    }

    char buffer[MAX_WORD_LENGTH];
    int count = 0;

    while (fgets(buffer, sizeof(buffer), file) && count < MAX_WORDLIST_SIZE)
    {

        buffer[strcspn(buffer, "\n")] = 0;
        buffer[strcspn(buffer, "\r")] = 0;

        if (strlen(buffer) > 0)
        {
            (*wordlist)[count] = strdup(buffer);
            count++;
        }
    }

    fclose(file);
    return count;
}

void free_wordlist(char **wordlist, int count)
{
    for (int i = 0; i < count; i++)
    {
        free(wordlist[i]);
    }
    free(wordlist);
}

int is_valid_url(const char *url)
{
    regex_t regex;
    int ret;

    const char *pattern = "^(http|https|ftp)://[a-zA-Z0-9\\-\\.]+\\.[a-zA-Z]{2,}(/.*)?$";
    ret = regcomp(&regex, pattern, REG_EXTENDED | REG_ICASE);
    if (ret)
    {
        return 0;
    }

    ret = regexec(&regex, url, 0, NULL, 0);
    regfree(&regex);

    return (ret == 0);
}

int url_exists(const char *url)
{
    CURL *curl;
    CURLcode res;
    long response_code;
    int exists = 0;

    curl = curl_easy_init();
    if (!curl)
    {
        return 0;
    }

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: */*");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);
    res = curl_easy_perform(curl);

    if (res == CURLE_OK || res == CURLE_PEER_FAILED_VERIFICATION || res == CURLE_SSL_CONNECT_ERROR)
    {
        if (res == CURLE_OK)
        {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            printf("%ld\n", response_code);
            if (response_code >= 200 && response_code < 500)
            {
                exists = 1;
            }
        }
        else
        {
            // Pour les erreurs SSL, on considère que l'URL existe quand même
            printf("SSL Error (site exists but SSL issue)\n");
            exists = 1;
        }
    }
    else
    {
        fprintf(stderr, "CURL Error: %s\n", curl_easy_strerror(res));
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return exists;
}

void extract_domain(const char *url, char *domain, size_t domain_size)
{
    const char *start = strstr(url, "://");
    if (start)
    {
        start += 3;
    }
    else
    {
        start = url;
    }

    const char *end = strchr(start, '/');
    if (end)
    {
        size_t len = end - start;
        if (len >= domain_size)
            len = domain_size - 1;
        strncpy(domain, start, len);
        domain[len] = '\0';
    }
    else
    {
        strncpy(domain, start, domain_size - 1);
        domain[domain_size - 1] = '\0';
    }
}

void extract_path(const char *url, char *path, size_t path_size)
{
    const char *start = strstr(url, "://");
    if (start)
    {
        start = strchr(start + 3, '/');
        if (start)
        {
            strncpy(path, start, path_size - 1);
            path[path_size - 1] = '\0';
            return;
        }
    }
    strcpy(path, "/");
}

void get_ip_address(const char *domain, char *ip, size_t ip_size)
{
    struct hostent *host = gethostbyname(domain);
    if (host && host->h_addr_list[0])
    {
        struct in_addr addr;
        memcpy(&addr, host->h_addr_list[0], sizeof(struct in_addr));
        strncpy(ip, inet_ntoa(addr), ip_size - 1);
        ip[ip_size - 1] = '\0';
    }
    else
    {
        strcpy(ip, "Unknown");
    }
}

void scanner(char *url)
{
    char **wordlist;
    total_words = load_wordlist("wordlist.txt", &wordlist);

    if (total_words == 0)
    {
        fprintf(stderr, "%sError: Failed to load wordlist%s\n", RED, RESET);
        return;
    }

    printf("Loaded %d words from wordlist\n", total_words);

    pthread_t threads[MAX_THREADS];
    ThreadData thread_data[MAX_THREADS];
    int words_per_thread = total_words / MAX_THREADS;
    int remaining = total_words % MAX_THREADS;

    int current_start = 0;
    for (int i = 0; i < MAX_THREADS; i++)
    {
        thread_data[i].wordlist = wordlist;
        thread_data[i].base_url = url;
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + words_per_thread + (i < remaining ? 1 : 0);
        current_start = thread_data[i].end;

        pthread_create(&threads[i], NULL, scan_url, &thread_data[i]);
    }

    for (int i = 0; i < MAX_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    free_wordlist(wordlist, total_words);
}

void show(char *url)
{
    int ret = system("clear");
    (void)ret;

    size_t len = strlen(url);
    if (len > 0 && url[len - 1] != '/')
    {
        strcat(url, "/");
    }

    printf("%s%s", BOLD, RED);
    printf("  _____       _                                 _   _             \n");
    printf(" |_   _|     | |                               | | (_)            \n");
    printf("   | |  _ __ | |_ _ __ ___  ___ _ __   ___  ___| |_ _  ___  _ __  \n");
    printf("   | | | '_ \\| __| '__/ _ \\/ __| '_ \\ / _ \\/ __| __| |/ _ \\| '_ \\ \n");
    printf("  _| |_| | | | |_| | | (_) \\__ | |_) |  __| (__| |_| | (_) | | | |\n");
    printf(" |_____|_| |_|\\__|_|  \\___/|___| .__/ \\___|\\___|\\___|_|\\___/|_| |_|\n");
    printf("                               | |                                \n");
    printf("                               |_|                                \n");
    printf("%s\n", RESET);

    printf("%s%s----------\n", YELLOW, BOLD);
    printf("|%s by OdG %s%s|\n", RESET, YELLOW, BOLD);
    printf("----------%s\n\n", RESET);

    printf("%sTarget URL --------> %s%s%s\n\n", BOLD, RED, url, RESET);

    char domain[256], path[256], ip[64];
    extract_domain(url, domain, sizeof(domain));
    extract_path(url, path, sizeof(path));
    get_ip_address(domain, ip, sizeof(ip));

    printf("Domain : %s\n", domain);
    printf("Path : %s\n", path);
    printf("IP : %s\n", ip);
    printf("Scanner started...\n");

    scanner(url);

    printf("\n\n%s%sScan completed%s\n\n", GREEN_BACKGROUND, BLACK_TEXT, RESET);
    printf("%sScores :    200 OK : %d founded\n", BOLD, score_200);
    printf("            301 REDIRECT : %d founded\n", score_301);
    printf("            403 RESTRICTED : %d founded%s\n", score_403, RESET);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    curl_global_init(CURL_GLOBAL_ALL);

    if (argc < 2)
    {
        printf("Usage: %s <TargetURL>\n", argv[0]);
        curl_global_cleanup();
        return 1;
    }

    char url[MAX_URL_LENGTH];
    strncpy(url, argv[1], sizeof(url) - 2);
    url[sizeof(url) - 2] = '\0';

    if (!is_valid_url(url))
    {
        printf("Usage: %s <TargetURL>\n", argv[0]);
        printf("%s-------- Invalid URL --------%s\n", RED, RESET);
        curl_global_cleanup();
        return 1;
    }

    if (!url_exists(url))
    {
        printf("%s-------- URL does not exist --------%s\n", RED, RESET);
        curl_global_cleanup();
        return 1;
    }

    show(url);

    curl_global_cleanup();
    return 0;
}
