#include <iostream>
#include <unistd.h>/*
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>*/
#include "winsock.h"

using namespace std;

char[PATH_MAX] current_dir;
getwd(&current_dir);

struct sockaddr_in host_addr;
host_addr.sin_addr = inet_addr();
host_addr.sin_port = htons("8080");
host_addr.sin_family = AF_INET;

struct option opts[] = {

        {"ip",1 ,0,'h'},

        {"port",1 ,0,'p'},

        {"directory",1 ,0,'d'},

        {0,0 ,0,0},
};


int main(int argc, char* argv[]) {
    char opchar = 0;
    int opindex = 0;

    string dir (current_dir);

    while (-1 != (opchar = getopt(argc,argv, "h:p:d:", opts, &opindex))) {

        switch (opchar) {
            case 'h':
                host_addr.sin_addr = inet_addr(optarg);
                break;
            case 'p':
                host_addr.sin_port = htons(atoi(optarg));
                break;
            case 'd':
                dir = string(optarg);
                break;
            default:
                break;
        }
    }

}
