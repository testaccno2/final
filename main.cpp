#include <iostream>
#include <sstream>
#include <fstream>
#include <exception>
#include <unistd.h>
#include <sys/types.h>
#include <cstddef>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define RECVBUFF_SIZE 2048

using namespace std;

const string HEADER200 = "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\n"; //HTML
const string HEADER404 = "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n";

void daemonize(const string& dir) {
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
    if (recv(listen_socket,&buf,RECVBUFF_SIZE,0) == -1)
        throw runtime_error("get req err: recv cmd");

    string buf_str(buf);
    istringstream received_data(buf_str);
    struct Request_Line req;
    received_data >> req.Method >> req.Request_URI >> req.HTTP_Version;

    return req;
}

void HTTP10_GET(string dir, string Request_URI, int socket) {
    if (socket == -1) throw runtime_error("get send err: invalid socket");

    if ( (!dir.empty()) && dir.back() == '/') dir.pop_back();

    fstream file(dir + Request_URI, ios::in | ios::binary);
    if(file.is_open()) {
        std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        contents += "\r\n\r\n";
        string full_HEADER200 = HEADER200 + "Content-Length: " + to_string(contents.length()) + "\r\n\r\n";

        if (send(socket, full_HEADER200.c_str(), full_HEADER200.length(), 0 ) == -1)
            cerr << "send 200 err" << endl;
        if (send(socket, contents.c_str(), contents.length(), 0 ) == -1)
            cerr << "send body err" << endl;

        file.close();
    }
    else {
        if (send(socket, HEADER404.c_str(), HEADER404.length(), 0 ) == -1)
            cerr << "send 404 err" << endl;
    }

    close(socket);


}

int main(int argc, char* argv[]) {

    struct sockaddr_in host_addr;
    host_addr.sin_addr.s_addr = INADDR_ANY;
    host_addr.sin_port = htons(8080);
    host_addr.sin_family = AF_INET;

    char opchar = 0;

    char current_dir[RECVBUFF_SIZE];
    getwd(current_dir);
    string dir (current_dir);

    while (-1 != (opchar = getopt(argc,argv, "h:p:d:"))) {
        switch (opchar) {
            case 'h':
                host_addr.sin_addr.s_addr = inet_addr(optarg);
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

//    if (set_nonblock(listen_socket) == -1) {
//        throw runtime_error("unblock_err");
//        return -1;
//    }

    if (listen(listen_socket, SOMAXCONN) == -1) {
        throw runtime_error("listen_activation_err");
        return -1;
    }

  //  daemonize(dir);

    while (true) {
        int new_slave_socket;
        if ((new_slave_socket = accept(listen_socket,NULL,NULL)) == -1) {
            perror("Error when accepting new connection");
            continue;
        }
        if (fork() == 0)  {
            close(listen_socket);
            Request_Line new_request = get_req(new_slave_socket);

            HTTP10_GET(dir,new_request.Request_URI,new_slave_socket);

        }
        else close(new_slave_socket);
    }

}
