# CWebCrawler

Two versions:
1. multithreaded separates downloading from cURL to multiple threads
2. asynchronous handles multiple downloads using asynchronous I/O (poll) 

## Usage:

1. multithreaded: findpng2 [OPTION]... [ROOT_URL]
   - options: 
     - -t=NUM - program will create NUM threads to crawl the web (default: 1)
     - -m=NUM - program will find up to NUM unique PNG urls (default: 50)
     - -v=LOGFILE - if specified, program will log the unique urls visited in a file named LOGFILE 
   - output:
     - on terminal, `findpng2 execution time: S seconds`
     - program will create a `png_urls.txt` file containing all the valid PNG urls found
2. asynchronous: findpng3 [OPTION]... [ROOT_URL]
   - options: 
     - -t=NUM - program will keep a maximum of NUM concurrent connections to crawl the web (default: 1)
     - -m=NUM - program will find up to NUM unique PNG urls (default: 50)
     - -v=LOGFILE - if specified, program will log the unique urls visited in a file named LOGFILE 
   - output:
     - on terminal, `findpng3 execution time: S seconds`
     - program will create a `png_urls.txt` file containing all the valid PNG urls found

