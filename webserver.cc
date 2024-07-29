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
#include <mutex>
#include <algorithm>
#include <sstream>

//anita adding chrono 
#include <chrono>
#include <iomanip>


using namespace std;

mutex pool_mutex;
vector<thread> THREAD_POOL;

void signal_handler(int sig_num)
{
    lock_guard<mutex> guard(pool_mutex);  // Lock the mutex before accessing THREAD_POOL
    while (!THREAD_POOL.empty())
    {
        THREAD_POOL.back().join();
        THREAD_POOL.pop_back();
    }
    cout << endl;
    exit(sig_num);
}

ssize_t send_all(int sockfd, const char *buffer, size_t len)
{
    size_t total_sent = 0;
    while (total_sent < len) {
        ssize_t sent = send(sockfd, buffer + total_sent, len - total_sent, 0);
        if (sent == -1) {
            cerr << "send() failed: " << strerror(errno) << endl; // Log the specific error
            return -1; // Return -1 if send fails
        }
        total_sent += sent;
    }
    return total_sent;
}


//helper function
string get_timestamp() {
    auto now = chrono::system_clock::now();
    auto now_time_t = chrono::system_clock::to_time_t(now);
    auto now_ms = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()) % 1000;
    stringstream timestamp;
    timestamp << put_time(localtime(&now_time_t), "%F %T")
              << '.' << setfill('0') << setw(3) << now_ms.count();
    return timestamp.str();
}


void foo(int client)
{
    char buffer[10000] = {0};  // Increased buffer size

    int received = recv(client, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        cerr << "Error receiving data." << endl;
        close(client);
        return;
    }

    string request(buffer);
    size_t start = request.find("GET /") + 5;
    size_t end = request.find(" ", start);
    string filename = request.substr(start, end - start);

    if (filename.empty() || filename == "/") {
        filename = "index.html";
    }

    ifstream file(filename, ios::binary);
    if (!file) {
        cerr << "Could not find requested file: " << filename << endl;
        string error = "HTTP/1.0 404 Not Found\r\n\r\n404 Not Found";
        if (send_all(client, error.c_str(), error.size()) == -1) {
            cerr << "Error sending 404 response." << endl;
        }
        //added print statement to work with helper function:
        cout << "server-response,404," << this_thread::get_id() << "," << get_timestamp() << endl;
        close(client);
        return;
    }

    ostringstream fileContents;
    fileContents << file.rdbuf();
    string contents = fileContents.str();
    file.close();

    string contentType;
    if (filename.substr(filename.find_last_of(".") + 1) == "html") {
        contentType = "text/html";
    } else if (filename.substr(filename.find_last_of(".") + 1) == "pdf") {
        contentType = "application/pdf";
    } else {
        contentType = "application/octet-stream";
    }

    string response = "HTTP/1.0 200 OK\r\n";
    response += "Content-Type: " + contentType + "\r\n";
    response += "Content-Length: " + to_string(contents.size()) + "\r\n\r\n";

    //added print statement to work with helper function:
    cout << "server-response,200," << this_thread::get_id() << "," << get_timestamp() << endl;
    //

    if (send_all(client, response.c_str(), response.size()) == -1) {
        cerr << "Error sending response headers." << endl;
        close(client);
        return;
    }

    if (send_all(client, contents.c_str(), contents.size()) == -1) {
        cerr << "Error sending file contents." << endl;
    }

    close(client);
}

int main(int argc, char* argv[])
{
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signal_handler);

    if (argc != 2)
    {
        cerr << "Usage: ./webserver {port}" << endl;
        exit(0);
    }
    int portNum = atoi(argv[1]);

    int mainServer = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(portNum);
    inet_aton("127.0.0.1", &addr.sin_addr);
    bind(mainServer, (struct sockaddr*)&addr, sizeof(addr));

    cout << "Exit the web server using a typical SIGINT (Ctrl+C)" << endl;

    listen(mainServer, 10);
    sockaddr_in clientAddr;
    socklen_t clientAddrSize = sizeof(clientAddr);
    int client;

    while((client = accept(mainServer, (sockaddr*)&clientAddr, &clientAddrSize)) > 0)
    {
        cout << "Connection established." << endl;
        thread newThread(foo, client);  // Pass client by value

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