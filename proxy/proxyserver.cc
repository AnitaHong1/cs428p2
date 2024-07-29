#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <mutex>
#include <algorithm>
#include <signal.h>

//cleaning up, anita added this
#include <chrono>
#include <iomanip>
using namespace std;

mutex pool_mutex;
vector<thread> THREAD_POOL;

void signal_handler(int sig_num) {
    lock_guard<mutex> guard(pool_mutex);
    while (!THREAD_POOL.empty()) {
        THREAD_POOL.back().join();
        THREAD_POOL.pop_back();
    }
    cout << endl;
    exit(sig_num);
}

ssize_t send_all(int sockfd, const char *buffer, size_t len) {
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(sockfd, buffer + total_sent, len - total_sent, 0);
        if (sent == -1) {
            cerr << "send() failed: " << strerror(errno) << endl;
            return -1;
        }
        total_sent += sent;
    }
    return total_sent;
}

string forward_request(const string &request, const string &serverAddress, int serverPort) {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        cerr << "Failed to create socket to forward request" << endl;
        return "";
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverAddress.c_str(), &serverAddr.sin_addr);

    if (connect(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        cerr << "Failed to connect to web server" << endl;
        close(serverSocket);
        return "";
    }

    write(serverSocket, request.c_str(), request.size());
    //cout << "Forwarded request to server: " << request << endl; // Log forwarded request
    thread::id id = this_thread::get_id();
    auto now = chrono::system_clock::now();
    auto now_c = chrono::system_clock::to_time_t(now);
    cout << "proxy-forward,server," << id << "," << put_time(localtime(&now_c), "%Y-%m-%d %X") << endl; //print message for request to web server
    
    // Read response from web server
    stringstream responseStream;
    char buffer[8192];
    int bytesRead;
    while ((bytesRead = read(serverSocket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the buffer
        responseStream.write(buffer, bytesRead);
    }

    close(serverSocket);
    return responseStream.str();
}

void handle_client(int clientSocket, int portNum) {
    char buffer[8192];
    memset(buffer, 0, sizeof(buffer));
    int bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);

    if (bytesRead < 0) {
        cerr << "Failed to read from client" << endl;
        close(clientSocket);
        return;
    }

    string request(buffer);
    string serverResponse = forward_request(request, "127.0.0.1", portNum);

    send_all(clientSocket, serverResponse.c_str(), serverResponse.size());
    thread::id id = this_thread::get_id();
    auto now = chrono::system_clock::now();
    auto now_c = chrono::system_clock::to_time_t(now);
    cout << "proxy-forward,client," << id << "," << put_time(localtime(&now_c), "%Y-%m-%d %X") << endl; //print message for response forwarded to client

    close(clientSocket);
}

int main(int argc, char *argv[]) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);

    if (argc != 2) {
        cerr << "Usage: ./proxyserver {port}" << endl;
        exit(0);
    }
    int portNum = atoi(argv[1]);

    int mainServer = socket(AF_INET, SOCK_STREAM, 0);
    if (mainServer < 0) {
        cerr << "Failed to create main server socket" << endl;
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portNum);
    inet_aton("127.0.0.2", &addr.sin_addr);

    if (bind(mainServer, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        cerr << "Failed to bind main server socket" << endl;
        close(mainServer);
        return 1;
    }

    if (listen(mainServer, 10) < 0) {
        cerr << "Failed to listen on main server socket" << endl;
        close(mainServer);
        return 1;
    }

    cout << "Proxy server is listening on port " << portNum << endl;

    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int client;

    while ((client = accept(mainServer, (struct sockaddr *)&clientAddr, &clientAddrSize)) > 0) {
        cout << "Connection established." << endl;
        thread newThread(handle_client, client, portNum);

        {
            lock_guard<mutex> guard(pool_mutex);
            THREAD_POOL.push_back(move(newThread));

            /* THREAD_POOL.erase(
                remove_if(THREAD_POOL.begin(), THREAD_POOL.end(), [](thread &t) {
                    if (t.joinable()) {
                        t.join();
                        return true;
                    }
                    return false;
                }),
                THREAD_POOL.end()
            ); */
        }
    }

    close(mainServer);
    return 0;
}
