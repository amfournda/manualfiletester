#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>

static void die (const char * format, ...) {
        va_list vargs;
        va_start(vargs, format);
        vfprintf(stderr, format, vargs);
        fprintf(stderr, ".\n");
        va_end(vargs);
        exit(1);
}

char *randstring(size_t length) {
    static char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *randomString = NULL;

    if (length) {
        randomString = malloc(sizeof(char) * (length +1));
        if (randomString) {
            for (int n = 0;n < length;n++) {
                int key = rand() % (int)(sizeof(charset) -1);
                randomString[n] = charset[key];
            }
            randomString[length] = '\0';
        }
    }
    return randomString;
}

static const char FILETOCHECK[] = "/tmp/elastic/manualtest";

void handle_session(int session_fd) {
        struct timeval tval_start, tval_filewrite, tval_end;
        gettimeofday(&tval_start, NULL);
        srand(time(NULL));
        char buffer[1024];
        FILE *fp = fopen(FILETOCHECK, "w+");
        char *r = randstring(16);
        fputs(r,fp);
        fclose(fp);
        gettimeofday(&tval_filewrite, NULL);
        long seconds = (tval_filewrite.tv_sec - tval_start.tv_sec);
        long micros = ((seconds * 1000000) + tval_filewrite.tv_usec) - (tval_start.tv_usec);
        printf("Took %d seconds and %d microseconds to write to manualtest: %s\n",seconds,micros,r);
        size_t index=0;
        while(index<16) {
                ssize_t count=write(session_fd,r+index,16-index);
                if(count<0) {
                        if(errno==EINTR) continue;
                        die("failed to write to socket (errno=%d)",errno);
                } else {
                        index+=count;
                }
        }
        gettimeofday(&tval_end, NULL);
        seconds = (tval_end.tv_sec - tval_filewrite.tv_sec);
        micros = ((seconds * 1000000) + tval_end.tv_usec) - (tval_filewrite.tv_usec);
        printf("Took %d seconds and %d microseconds to sent string to client: %s\n",seconds,micros,r);
        fflush(stdout);
        free(r);
}

int main(int argc, char *argv[]) {
        const char* hostname=0; /* wildcard */
        const char* portname="4978";
        struct addrinfo hints;
        memset(&hints,0,sizeof(hints));
        hints.ai_family=AF_UNSPEC;
        hints.ai_socktype=SOCK_STREAM;
        hints.ai_protocol=0;
        hints.ai_flags=AI_PASSIVE|AI_ADDRCONFIG;
        struct addrinfo* res=0;

        int err=getaddrinfo(hostname,portname,&hints,&res);
        if(err!=0) {
                die("failed to resolve local socet address (err=%d)",err);
        }

        int server_fd=socket(res->ai_family,res->ai_socktype,res->ai_protocol);
        if(server_fd==-1) {
                die("%s",strerror(errno));
        }

        int reuseaddr=1;
        if(setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR,&reuseaddr,sizeof(reuseaddr))==-1) {
                die("%s",strerror(errno));
        }

        if(bind(server_fd,res->ai_addr,res->ai_addrlen)==-1) {
                die("$s",strerror(errno));
        }

        if(listen(server_fd,SOMAXCONN)) {
                die("failed to listen for connections (errno=%d)",errno);
        }

        for(;;) {
                int session_fd=accept(server_fd,0,0);
                if(session_fd==-1) {
                        if (errno==EINTR) continue;
                        die("failed to accept connection (errno=%d)",errno);
                }
                pid_t pid=fork();
                if(pid==-1) {
                        die("failed to create child process (errno=%d)",errno);
                } else if (pid==0) {
                        close(server_fd);
                        handle_session(session_fd);
                        close(session_fd);
                        _exit(0);
                } else {
                        close(session_fd);
                }
        }
}
