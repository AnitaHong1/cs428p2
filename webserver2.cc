#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <csignal>

using namespace std;

vector<thread> THREAD_POOL;

void signal_handler(int sig_num)
{
    while (!THREAD_POOL.empty())
    {
        THREAD_POOL.back().join();
        THREAD_POOL.pop_back();
    }
    cout << endl;
    exit(sig_num);
}

void foo(int &client)
{
    char buffer[5000] = { 0 };
    if (recv(client, (char*)&buffer, sizeof(buffer), 0) == 0)
    {
        cout << "Could not receive message." << endl;
        return;
    }
    string cutBuff = ((string)buffer).substr(((string)buffer).find("/"));
    string filename = cutBuff.substr(0, cutBuff.find(" ")).substr(1);
    fstream file;
    file.open(filename, ios::in | ios::binary);
    if (!file.is_open())
    {
        cout << "Could not find requested file!" << endl;
        string error = "HTTP/1.0 404 Not Found\r\n\r\n404 Not Found";
        send(client, error.c_str(), error.length(), 0);
        close(client);
    }
    else if (filename.substr(filename.find_last_of(".") + 1) == "html")
    {
        string fileContents((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        string msg = "HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=utf-8\r\n\r\n" + fileContents;
        send(client, msg.c_str(), msg.length(), 0);
        close(client);
    }
    else if (filename.substr(filename.find_last_of(".") + 1) == "pdf")
    {
        string fileContents((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        string msg = "HTTP/1.0 200 OK\r\nContent-Type: application/pdf\r\n\r\n" + fileContents;
        send(client, msg.c_str(), msg.length(), 0);
        close(client);
    }
    else
    {
        cout << "File format not supported, must be of HTML or PDF." << endl;
        string error = "HTTP/1.0 404 Not Found\r\n\r\n404 Not Found";
        send(client, error.c_str(), error.length(), 0);
        close(client);
    }
}

int main(int argc, char* argv[])
{
    signal(SIGINT, signal_handler);
    if (argc != 2)
    {
        cerr << "Usage: ./webserver2 {port}" << endl;
        exit(0);
    }
    int portNum = atoi(argv[1]);

    int mainServer = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portNum);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(mainServer, (struct sockaddr*)&addr, sizeof(addr));

    cout << "Exit the web server using a typical SIGINT (Ctrl+C)" << endl;

    listen(mainServer, 1);
    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int client;

    while((client = accept(mainServer, (sockaddr*)&clientAddr, &clientAddrSize)) > 0)
    {
        cout << "Connection established." << endl;
        thread newThread(foo, ref(client));
        THREAD_POOL.push_back(move(newThread));
    }
    close(mainServer);
    return 0;
}
