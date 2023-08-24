#include "findpng2.h"

// -- Global Variables --
STACK *frontier;
STACK *pngs;
HMAP *visited;
uint8_t done;
size_t num_waiting_on_png;
size_t num_running;
int m;
/* ----------------- */

// -- Synchronization --
pthread_cond_t frontier_empty;
pthread_mutex_t frontier_mutex;
pthread_mutex_t visited_mutex;
pthread_mutex_t pngs_mutex;
/* ----------------- */

void *runner(void *args)
{
    // -- Initialize CURL --
    CURL *curl_handle = curl_easy_init();
    if (curl_handle == NULL)
    {
        fprintf(stderr, "curl_easy_init: returned NULL\n");
        exit(1);
    }
    /* ----------------- */

    uint8_t to_break = 0;
    char *url_to_crawl = NULL;
    int data_type = -1;
    STACK *urls_found = NULL;
    int cleaned_urls_found = 0;
    long response_code;

    while (1)
    {
        // -- cleanup from last iteration --
        if (urls_found != NULL)
        {
            if (cleaned_urls_found == 0)
            {
                cleanup_s(urls_found);
                cleaned_urls_found = 1;
            }
            free(urls_found);
        }
        urls_found = malloc(sizeof(STACK));
        memset(urls_found, 0, sizeof(STACK));
        init_stack(urls_found, 1);
        cleaned_urls_found = 0;

        if (url_to_crawl != NULL)
        {
            free(url_to_crawl);
            url_to_crawl = NULL;
        }
        /* ----------------- */

        // -- Checking if there are any urls in the frontier to crawl --
        pthread_mutex_lock(&frontier_mutex);
        {

            if (is_empty_s(frontier) && num_running == 0)
            {
                done = 1;
                if (num_waiting_on_png > 0)
                {
                    pthread_cond_broadcast(&frontier_empty);
                }
            }

            if (done)
            {
                pthread_mutex_unlock(&frontier_mutex);
                break;
            }
            while (is_empty_s(frontier))
            {
                ++num_waiting_on_png;
                pthread_cond_wait(&frontier_empty, &frontier_mutex);
                --num_waiting_on_png;
                if (done)
                {
                    pthread_mutex_unlock(&frontier_mutex);
                    to_break = 1;
                    break;
                }
            }
            if (to_break)
            {
                break;
            }
            pop_s(frontier, &url_to_crawl);
            pthread_mutex_lock(&visited_mutex);
            {
                if (search_h(visited, url_to_crawl) == 1)
                {
                    pthread_mutex_unlock(&visited_mutex);
                    pthread_mutex_unlock(&frontier_mutex);
                    continue;
                }
                else
                {
                    add_h(visited, url_to_crawl);
                }
            }
            pthread_mutex_unlock(&visited_mutex);
            ++num_running;
        }
        pthread_mutex_unlock(&frontier_mutex);
        /* ----------------- */

        // -- Crawl the url --
        process_url(curl_handle, url_to_crawl, &data_type, urls_found, &response_code);
        /* ----------------- */

        // -- Either check the urls in the HTML or process PNG --
        if ((response_code >= 200 && response_code <= 299) || (response_code >= 300 && response_code <= 399))
        {
            if (data_type == HTML)
            {
                char *url_in_html = NULL;
                while (pop_s(urls_found, &url_in_html) == 0)
                {
                    pthread_mutex_lock(&frontier_mutex);
                    {
                        push_s(frontier, url_in_html);
                        if (num_waiting_on_png > 0)
                        {
                            pthread_cond_broadcast(&frontier_empty);
                        }
                    }
                    pthread_mutex_unlock(&frontier_mutex);
                    free(url_in_html);
                    url_in_html = NULL;
                }
            }
            else if (data_type == VALID_PNG)
            {
                pthread_mutex_lock(&pngs_mutex);
                {
                    if ((pngs->pos + 1) >= m)
                    {
                        pthread_mutex_lock(&frontier_mutex);
                        {
                            done = 1;
                            pthread_cond_broadcast(&frontier_empty);
                        }
                        pthread_mutex_unlock(&frontier_mutex);
                    }
                    else
                    {
                        push_s(pngs, url_to_crawl);
                    }
                }
                pthread_mutex_unlock(&pngs_mutex);
            }
        }
        /* ----------------- */
        // -- Signal that the thread is done running --
        pthread_mutex_lock(&frontier_mutex);
        {
            --num_running;
        }
        pthread_mutex_unlock(&frontier_mutex);
        /* ----------------- */
    }

    // -- Clean up --
    // variables
    if (urls_found != NULL)
    {
        if (cleaned_urls_found == 0)
        {
            cleanup_s(urls_found);
        }
        free(urls_found);
    }

    if (url_to_crawl != NULL)
    {
        free(url_to_crawl);
        url_to_crawl = NULL;
    }
    // clean up CURL
    curl_easy_cleanup(curl_handle);
    /* ----------------- */

    return NULL;
}

int main(int argc, char **argv)
{
    /* -- command line inputs -- */
    char *seed_url;
    char *logfile = NULL;
    size_t t = 1;
    m = 50;

    if (argc == 1)
    {
        printf("Usage: ./findpng2 OPTION[-t=<NUM> -m=<NUM> -v=<LOGFILE>] SEED_URL\n");
        return -1;
    }

    seed_url = argv[argc - 1];
    // printf("SEED_URL is %s\n", seed_url);

    int c;
    char *str = "option requires an argument";
    while ((c = getopt(argc, argv, "t:m:v:")) != -1)
    {
        switch (c)
        {
        case 't':
            if (optarg == NULL)
            {
                t = 1;
                break;
            }
            t = strtoul(optarg, NULL, 10);
            if (t <= 0)
            {
                fprintf(stderr, "%s: %s > 0 -- 't'\n", argv[0], str);
                return -1;
            }
            break;
        case 'm':
            if (optarg == NULL)
            {
                m = 50;
                break;
            }
            m = atoi(optarg);
            if (m < 0)
            {
                fprintf(stderr, "%s: %s >= 0 -- 'm'\n", argv[0], str);
                return -1;
            }
            break;
        case 'v':
            if (optarg == NULL)
            {
                logfile = NULL;
                break;
            }
            logfile = malloc(sizeof(char) * URL_SIZE);
            memset(logfile, 0, sizeof(char) * URL_SIZE);
            strcpy(logfile, optarg);
            break;
        }
    }
    // printf("option -t specifies a value of %ld\n", t);
    // printf("option -m specifies a value of %ld\n", m);
    // printf("option -v specifies a value of %s\n", logfile);
    /* ----------------- */

    // -- initialize global variables and synchronization variables --
    frontier = malloc(sizeof(STACK));
    memset(frontier, 0, sizeof(STACK));
    init_stack(frontier, STACK_SIZE);

    visited = malloc(sizeof(HMAP));
    memset(visited, 0, sizeof(HMAP));
    init_hmap(visited, HMAP_SIZE);

    pngs = malloc(sizeof(STACK));
    memset(pngs, 0, sizeof(STACK));
    init_stack(pngs, STACK_SIZE);

    done = 0;
    num_waiting_on_png = 0;
    num_running = 0;

    pthread_cond_init(&frontier_empty, NULL);
    pthread_mutex_init(&frontier_mutex, NULL);
    pthread_mutex_init(&visited_mutex, NULL);
    pthread_mutex_init(&pngs_mutex, NULL);
    /* ----------------- */

    // -- CURL global init --
    curl_global_init(CURL_GLOBAL_DEFAULT);
    /* ----------------- */

    // -- Initialize XML Parser
    xmlInitParser();
    /* ----------------- */

    // -- Put the seed URL in the frontier --
    push_s(frontier, seed_url);
    /* ----------------- */

    double times[2];
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        perror("gettimeofday");
        exit(1);
    }
    times[0] = (tv.tv_sec) + tv.tv_usec / 1000000.;

    // -- Create threads --
    pthread_t *runners = malloc(t * sizeof(pthread_t));
    memset(runners, 0, sizeof(pthread_t) * t);
    if (runners == NULL)
    {
        perror("malloc\n");
        exit(-1);
    }
    for (int i = 0; i < t; ++i)
    {
        pthread_create(&runners[i], NULL, runner, NULL);
    }
    /* ----------------- */

    // -- Wait for threads to be done --
    for (int i = 0; i < t; ++i)
    {
        pthread_join(runners[i], NULL);
    }
    /* ----------------- */

    // -- Write to files --
    FILE *fpngs = fopen("./png_urls.txt", "w+");
    if (fpngs == NULL)
    {
        fprintf(stderr, "Opening png file for write failed\n");
        exit(1);
    }
    char *temp = NULL;
    while (pop_s(pngs, &temp) == 0)
    {
        fprintf(fpngs, "%s\n", temp);
        free(temp);
    }
    fclose(fpngs);

    if (logfile != NULL)
    {
        char *logfile_name = malloc(sizeof(char) * URL_SIZE);
        memset(logfile_name, 0, sizeof(char) * URL_SIZE);
        sprintf(logfile_name, "./%s", logfile);
        FILE *flogs = fopen(logfile_name, "w+");
        free(logfile_name);
        if (flogs == NULL)
        {
            fprintf(stderr, "Opening log file for write failed\n");
            exit(1);
        }
        temp = NULL;
        for (size_t i = 0; i < visited->cur_size; ++i)
        {
            fprintf(flogs, "%s\n", visited->elements[i]);
            free(temp);
        }
        fclose(flogs);
    }

    free(logfile);
    /* ----------------- */

    // -- Cleanup global variables and sync variables
    cleanup_s(frontier);
    free(frontier);
    frontier = NULL;

    cleanup_h(visited);
    free(visited);
    visited = NULL;

    cleanup_s(pngs);
    free(pngs);
    pngs = NULL;

    pthread_cond_destroy(&frontier_empty);
    pthread_mutex_destroy(&frontier_mutex);
    pthread_mutex_destroy(&visited_mutex);
    pthread_mutex_destroy(&pngs_mutex);

    free(runners);
    /* ----------------- */

    // -- library cleanups --
    curl_global_cleanup();
    xmlCleanupParser();
    /* ----------------- */

    if (gettimeofday(&tv, NULL) != 0)
    {
        perror("gettimeofday");
        exit(1);
    }
    times[1] = (tv.tv_sec) + tv.tv_usec / 1000000.;
    // printf("Parent pid = %d: total execution time is %.6lf seconds\n", getpid(),  times[1] - times[0]);
    printf("findpng2 execution time: %.6lf seconds\n", times[1] - times[0]);

    return 0;
}