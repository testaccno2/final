#include <iostream>
#include <fstream>
#include <exception>
#include <unistd.h>
#include <sys/types.h>
#include <cstddef>/*
#include <sys/socket.h>
#include <sys/ioctl.h>
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

void daemonize() {
    chdir("/");

    setsid();

    fclose(stderr);
    fclose(stdout);
    fclose(stdin);
}

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

    int listen_socket;
    if ((listen_socket = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        throw runtime_error("socket_init_err");
        return -1;
    }

    if (bind(listen_socket, (struct sockaddr*) &host_addr,
            sizeof(host_addr)) == -1) {
        throw runtime_error("bind_err");
        return -1;
    }

    if (set_nonblock(listen_socket) == -1) {
        throw runtime_error("unblock_err");
        return -1;
    }

    if (listen(listen_socket, SOMAXCONN) == -1) {
        throw runtime_error("listen_activation_err");
        return -1;
    }

    daemonize();

    while (true) {
        int new_slave_socket;
        if ((new_slave_socket = accept(listen_socket,NULL,NULL)) == -1) {
            cerr << "Error when accepting new connection." << endl;
            continue;
        }
        if (fork() =! 0) close(new_slave_socket);
        else {
            close(listen_socket);
            //TODO io recv

            fstream file("TODO", ios::in | ios::binary);
            if(!file.is_open())
                cerr << "TODO" << " — 404" << endl;
            std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            if (send(new_slave_socket, contents.c_str(), contents.length(), 0 ) == -1)
                cerr << "send err" << endl;
        }
    }

}
