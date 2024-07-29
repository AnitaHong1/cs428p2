# Binghamton University, Summer 2024

## CS428/528 Project-2: Web Proxy Server

### SUMMARY

[Provide a short description of your program's functionality, no more than a couple sentences]: Program functions very similarly to project 1.
webserver.cc acts as a multi-threaded server, which works in tandem with the proxy server, operating on set IP address 127.0.0.1 and a customizable
port number. proxyserver.cc also acts a multi-threaded server, working in tandem with the web server, forwarding requests and responses between
the client and web server, operating on set IP address 127.0.0.2 and a customizable port number.

### NOTES, KNOWN BUGS, AND/OR INCOMPLETE PARTS

[Add any notes you have here and/or any parts of the project you were not able to complete]: We managed to minimize the amount of bugs, and could
not find any we know of when testing with a bash script. We tested with valgrind as well, and found no leaks or crashes during our testing.

### REFERENCES

[List any outside resources used]: The outside resources we used were Google, namely StackOverflow and the C++ reference website.

### INSTRUCTIONS

[Provide clear and complete step-by-step instructions on how to run and test your project]: We did not provide a makefile, but the compilation is
very simple as is. For compiling the web server, simply do g++ webserver.cc -o webserver -pthread. For compiling the proxy server, do g++ proxyserver.cc -o proxyserver -pthread. To start up the web server, use the following format: ./webserver {port number}. To start up the proxy number, use the exact same formatting, but with ./proxyserver. To shut either down safely, initialize a SIGINT by doing Ctrl+C.

### SUBMISSION

I have done this assignment completely on my own. I have not copied it, nor have I given my solution to anyone else. I understand that if I am involved in plagiarism or cheating I will have to sign an official form that I have cheated and that this form will be stored in my official university record. I also understand that I will receive a grade of "0" for the involved assignment and my grade will be reduced by one level (e.g., from "A" to "A-" or from "B+" to "B") for my first offense, and that I will receive a grade of "F" for the course for any additional offense of any kind.

By signing my name below and submitting the project, I confirm the above statement is true and that I have followed the course guidelines and policies.

Submission date: July 28, 2024

Team member 1 name: Christopher Potthast

Team member 2 name (N/A, if not applicable): Anita Hong

