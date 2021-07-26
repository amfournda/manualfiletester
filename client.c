#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <stdarg.h>
#include <arpa/inet.h>

static const char FILETOCHECK[] = "/tmp/elastic/manualtest";

static void die (const char * format, ...) {
        va_list vargs;
        va_start(vargs, format);
        vfprintf(stderr, format, vargs);
        fprintf(stderr, ".\n");
        va_end(vargs);
        exit(1);
}

int main(int argc, char *argv[]) {
        int sock = 0, valread;
        struct sockaddr_in serv_addr;
        char buffer[1024] = {0};

        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                die("failed to create socket (errno=%d)",errno);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(4978);
        inet_pton(AF_INET, "10.40.8.52", &serv_addr.sin_addr);

        if(connect(sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
                die("failed to connect to server (errno=%d)",errno);
        }

        valread = read(sock,buffer,1024);
        char *buffer2 = buffer+valread+1;
        FILE *fp = fopen(FILETOCHECK, "r");
        fgets(buffer2,(1023-valread),fp);
        fclose(fp);
        printf("Remote server sent %s and file contains %s\n",buffer, buffer2);
        exit(0);
}
