#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <exception>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstddef>
#include <getopt.h>
#include<sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <csignal>

#define RECVBUFF_SIZE 2048

using namespace std;

void SIGCHLD_hundler(int signum) {
    int status;
    wait(&status);
    if (status != 0) cerr << "problems with child" << endl;
}

const string HEADER200 = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n";
const string HEADER404 = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";

void daemonize(const string &dir) {
    signal(SIGCHLD, SIGCHLD_hundler);
    signal(SIGHUP,SIG_IGN);

    chdir(dir.c_str());

    setsid();

    fclose(stderr);
    fclose(stdout);
    fclose(stdin);
}

struct Request_Line {
    string Method;
    string Request_URI;
    string HTTP_Version;
};

Request_Line get_req(int listen_socket) {
    if (listen_socket == -1) throw runtime_error("get req err: invalid socket");

    char buf[RECVBUFF_SIZE];
    if (recv(listen_socket, &buf, RECVBUFF_SIZE, 0) == -1)
        throw runtime_error("get req err: recv cmd");

    string buf_str(buf);
    istringstream received_data(buf_str);
    struct Request_Line req;
    received_data >> req.Method >> req.Request_URI >> req.HTTP_Version;

    return req;
}

void HTTP10_GET(string dir, string Request_URI, int socket) {
    if (socket == -1) throw runtime_error("get send err: invalid socket");

    if ((!dir.empty()) && dir.back() == '/') dir.pop_back();
    replace(Request_URI.begin(), Request_URI.end(),'?','\0');
    dir += Request_URI;

    struct stat s;
    fstream file(dir, ios::in | ios::binary);
    if (stat(dir.c_str(),&s) == 0 && s.st_mode & S_IFREG && file.is_open()) {
        std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        string full_HEADER200 = HEADER200 + "Content-Length: " + to_string(contents.length()) + "\r\n\r\n";

        if (send(socket, full_HEADER200.c_str(), full_HEADER200.length(), 0) == -1)
            cerr << "send 200 err" << endl;
        if (send(socket, contents.c_str(), contents.length(), 0) == -1)
            cerr << "send body err" << endl;

        file.close();
    } else {
        if (dir.find(".html") == string::npos) HTTP10_GET(dir+".html","",socket);
        else if (send(socket, HEADER404.c_str(), HEADER404.length(), 0) == -1)
            cerr << "send 404 err" << endl;
    }

    close(socket);


}

int main(int argc, char *argv[]) {

    struct sockaddr_in host_addr;
    host_addr.sin_addr.s_addr = INADDR_ANY;
    host_addr.sin_port = htons(8080);
    host_addr.sin_family = AF_INET;

    char opchar = 0;

    char current_dir[RECVBUFF_SIZE];
    getwd(current_dir);
    string dir(current_dir);

    while (-1 != (opchar = getopt(argc, argv, "h:p:d:"))) {
        switch (opchar) {
            case 'h':
                host_addr.sin_addr.s_addr = inet_addr(optarg);
                break;
            case 'p':
                host_addr.sin_port = htons(atoi(optarg));
                break;
            case 'd':
                if (optarg[0] == '/') dir = string(optarg);
                else dir += '/' + string (optarg);
                break;
            default:
                break;
        }
    }
    if (fork() == 0) {
        daemonize(dir);

        int listen_socket;
        if ((listen_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            throw runtime_error("socket_init_err");
        }

        if (bind(listen_socket, (struct sockaddr *) &host_addr,
                 sizeof(host_addr)) == -1) {
            throw runtime_error("bind_err");
        }

        if (listen(listen_socket, SOMAXCONN) == -1) {
            throw runtime_error("listen_activation_err");
        }

        while (true) {
            int new_slave_socket;
            if ((new_slave_socket = accept(listen_socket, NULL, NULL)) == -1) {
                if (errno != EINTR) perror("Error when accepting new connection");
                continue;
            }
            if (fork() == 0) {
                close(listen_socket);
                Request_Line new_request = get_req(new_slave_socket);

                HTTP10_GET(dir, new_request.Request_URI, new_slave_socket);

                exit(EXIT_SUCCESS);
            } else close(new_slave_socket);
        }
    } else return 0;


}
