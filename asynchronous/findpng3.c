#include "findpng3.h"

// -- Global Variables --
STACK *frontier;
STACK *pngs;
HMAP *visited;
uint8_t done;
size_t num_waiting_on_png;
size_t num_running;
int m;
/* ----------------- */

int main(int argc, char **argv)
{
    /* -- command line inputs -- */
    char *seed_url;
    char *logfile = NULL;
    size_t t = 1;
    m = 50;

    if (argc == 1)
    {
        printf("Usage: ./findpng3 OPTION[-t=<NUM> -m=<NUM> -v=<LOGFILE>] SEED_URL\n");
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
    /* ----------------- */

    // -- CURL global init --
    curl_global_init(CURL_GLOBAL_ALL);
    /* ----------------- */

    // -- Initialize XML Parser
    xmlInitParser();
    /* ----------------- */

    double times[2];
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0)
    {
        perror("gettimeofday");
        exit(1);
    }
    times[0] = (tv.tv_sec) + tv.tv_usec / 1000000.;

    // -- Process the URL --

    // -- CURL --
    CURLM *cm = NULL;
    CURL **ehs = malloc(sizeof(CURL *) * t);
    memset(ehs, 0, sizeof(CURL *) * t);
    for (int i = 0; i < t; ++i)
    {
        ehs[i] = curl_easy_init();
        if (ehs[i] == NULL)
        {
            fprintf(stderr, "curl_easy_init: returned NULL\n");
            return -1;
        }
    }

    RECV_BUF **bufs = malloc(sizeof(RECV_BUF *) * t);
    memset(bufs, 0, sizeof(RECV_BUF *) * t);
    for (int i = 0; i < t; ++i)
    {
        bufs[i] = NULL;
    }

    int *indices = malloc(sizeof(int) * t);
    memset(indices, 0, sizeof(int) * t);
    for (int i = 0; i < t; ++i)
    {
        indices[i] = i;
    }

    CURLMsg *msg = NULL;
    CURLMcode resm = 0;
    int still_running = 0;
    int msgs_left = 0;

    cm = curl_multi_init();
    /* ----------------- */

    // -- findpng setup --
    push_s(frontier, seed_url);

    char **urls_to_crawl = malloc(sizeof(char *) * t);
    memset(urls_to_crawl, 0, sizeof(char *) * t);
    for (int i = 0; i < t; ++i)
    {
        urls_to_crawl[i] = NULL;
    }
    /* ----------------- */

    // -- main while loop --
    while ((!is_empty_s(frontier) || (still_running > 0)) && (pngs->pos + 1 < m))
    {
        // -- add to mutlihandle --
        while ((t > still_running) && !is_empty_s(frontier))
        {
            char *url_to_crawl = NULL;
            pop_s(frontier, &url_to_crawl);
            // printf("pop: %s\n", url_to_crawl);
            if (search_h(visited, url_to_crawl) != 1)
            {
                add_h(visited, url_to_crawl);
                for (int i = 0; i < t; ++i)
                {
                    if (bufs[i] == NULL)
                    {
                        curl_add(cm, ehs[i], url_to_crawl, &bufs[i], &indices[i]);
                        urls_to_crawl[i] = url_to_crawl;
                        still_running += 1;
                        break;
                    }
                }
            }
            else
            {
                free(url_to_crawl);
            }
        }
        if (still_running == 0)
        {
            break;
        }
        /* ----------------- */

        // multi_perform and wait until at least one handle finishes
        int curl_still_running = 0;
        curl_multi_perform(cm, &curl_still_running);
        do
        {
            int numfds = 0;
            resm = curl_multi_wait(cm, NULL, 0, MAX_WAIT_MSECS, &numfds);
            if (resm != CURLM_OK)
            {
                fprintf(stderr, "error: curl_multi_wait() returned %d\n", resm);
                return -1;
            }
            curl_multi_perform(cm, &curl_still_running);
        } while (curl_still_running == still_running);

        still_running = curl_still_running;

        // -- process results of each easy handle --
        STACK *urls_found = malloc(sizeof(STACK));
        memset(urls_found, 0, sizeof(STACK));
        init_stack(urls_found, 1);

        msgs_left = 0;
        msg = NULL;
        while ((msg = curl_multi_info_read(cm, &msgs_left)))
        {
            int done = 0;

            if (msg->msg == CURLMSG_DONE)
            {
                CURL *eh = msg->easy_handle;
                CURLcode res = msg->data.result;
                int *index_p = 0;
                curl_easy_getinfo(eh, CURLINFO_PRIVATE, &index_p);
                int index = *index_p;

                if (res != CURLE_OK)
                {
                    // fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
                }
                else
                {
                    int data_type = -1;
                    long response_code = 0;
                    process_data(eh, bufs[index], &data_type, urls_found, &response_code);
                    // -- Either check the urls in the HTML or process PNG --
                    if ((response_code >= 200 && response_code <= 299) || (response_code >= 300 && response_code <= 399))
                    {
                        if (data_type == HTML)
                        {
                            char *url_in_html = NULL;
                            while (pop_s(urls_found, &url_in_html) == 0)
                            {
                                push_s(frontier, url_in_html);
                                // printf("push: %s\n", url_in_html);
                                free(url_in_html);
                                url_in_html = NULL;
                            }
                        }
                        else if (data_type == VALID_PNG)
                        {
                            push_s(pngs, urls_to_crawl[index]);
                            if ((pngs->pos + 1) >= m)
                            {
                                done = 1;
                            }
                        }
                    }
                    /* ----------------- */
                }

                recv_buf_cleanup(bufs[index]);
                free(bufs[index]);
                bufs[index] = NULL;

                curl_multi_remove_handle(cm, eh);

                free(urls_to_crawl[index]);
                urls_to_crawl[index] = NULL;

                if (done)
                {
                    break;
                }
            }
            else
            {
                // fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n", msg->msg);
            }
        }

        cleanup_s(urls_found);
        free(urls_found);
        /* ----------------- */
    }

    // -- cleanup of used structures in url processing --
    curl_multi_cleanup(cm);
    free(indices);

    for (int i = 0; i < t; ++i)
    {
        if (bufs[i] != NULL)
        {
            recv_buf_cleanup(bufs[i]);
            free(bufs[i]);
        }
    }
    free(bufs);

    for (int i = 0; i < t; ++i)
    {
        if (urls_to_crawl[i] != NULL)
        {
            free(urls_to_crawl[i]);
        }
    }
    free(urls_to_crawl);

    for (int i = 0; i < t; ++i)
    {
        curl_easy_cleanup(ehs[i]);
    }
    free(ehs);
    /* ----------------- */

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
    printf("findpng3 execution time: %.6lf seconds\n", times[1] - times[0]);

    return 0;
}