#include <gtest/gtest.h> // googletest header file


#include <arpa/inet.h> 
#include <errno.h> 
#include <netinet/in.h> 
#include <signal.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <strings.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <unistd.h> 


#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <mutex> 

using namespace std;

namespace SoketTest {

vector<string> zrequest_queue;
mutex zrequest_queue_mtx;

// threads shared variables
bool z_server_ready = false;

const int MAXLINE = 1024;

ostream& cout_thread(thread::id l_id) {
    return cout << "T" << l_id << ": ";
}

// Handle single event
void server_handler(const int l_conn_fd)
{
    char buffer[MAXLINE] = {0};
    thread::id id = this_thread::get_id();
    string message("HTTP/1.1 200 OK\r\n\r\nServer Response: Hello Client\r\n\r\n"); 

    if (read(l_conn_fd, buffer, sizeof(buffer)) > 0) {
        lock_guard<mutex> lck (zrequest_queue_mtx);
        zrequest_queue.push_back(string(buffer, sizeof(buffer)));
        cout_thread(id) << "message from client - " << endl << buffer << endl;
        write(l_conn_fd, message.c_str(), sizeof(buffer));
    } 
    close(l_conn_fd); 
}


void server(const int l_port)
{
    int listenfd, nready; 
    fd_set rset; 
    struct sockaddr_in servaddr; 
    void sig_chld(int); 
    struct timeval tv;

    vector<thread> lhandlers;
    vector<thread>::iterator lhandler_iter;

    vector<int> lconns;
    vector<int>::iterator lconns_iter;

    thread::id id = this_thread::get_id();

    /* create listening TCP socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0); 
    bzero(&servaddr, sizeof(servaddr)); 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(l_port); 
  
    // binding server addr structure to listenfd 
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
    listen(listenfd, 10); 
    
    z_server_ready = true;
    
    // clear the descriptor set 
    FD_ZERO(&rset); 
  
    while (z_server_ready) { 
  
        // set listenfd and udpfd in readset 
        FD_SET(listenfd, &rset); 
  
        /* Wait up to five seconds. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        // select the ready descriptor 
        nready = select(listenfd+1, &rset, NULL, NULL, &tv); 
        if (nready <= 0) {
            cout_thread(id) << "didn't find any active event, errno " << errno << endl;
            continue;
        }
        
        // if tcp socket is readable then handle 
        // it by accepting the connection 
        if (FD_ISSET(listenfd, &rset)) {
            struct sockaddr_in cliaddr;
            socklen_t len = sizeof(cliaddr);
            int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len); 
            if (connfd <= 0) {
                cout_thread(id) << "failed to estabilsh connection." << endl;
                return;
            }

            lconns.push_back(connfd);
            thread lhandler(server_handler, connfd);
            lhandlers.push_back(std::move(lhandler));
        } 
    }

    lhandler_iter = lhandlers.begin();
    while (lhandler_iter != lhandlers.end()) {
        if (lhandler_iter->joinable()) {
            lhandler_iter->join();
        }
        lhandler_iter++;
    }

    lconns_iter = lconns.begin();
    while (lconns_iter != lconns.end()) {
        close(*lconns_iter);
        lconns_iter++;
    }

    close(listenfd);
}

void client(const int l_server_port, const string& l_message, const string& l_output_file)
{
    thread::id id = this_thread::get_id();

    stringstream lcmd;
    lcmd << "curl -d \"" << l_message << "\" http://localhost:" << l_server_port << " --out " << l_output_file << " 1>&2 2>/dev/null";
    cout_thread(id) << "exectue command " << lcmd.str() << endl;;
    system(lcmd.str().c_str());
    cout_thread(id) << ifstream(l_output_file.c_str()).rdbuf() << endl;
}


class SoketTest : public ::testing::Test {
 protected:
  void SetUp() override {
    system("rm -rf client*.out*");
  }

  // void TearDown() override {}

};

TEST_F(SoketTest, Sanity) {
    const int lport = 34345;

    thread lserver(server, lport);

    while (!z_server_ready) {
        this_thread::sleep_for(chrono::seconds(1));
    }

    thread lclient1(client, lport, string("Client Request: Hello Server from client1"), string("client1.out"));
    thread lclient2(client, lport, string("Client Request: Hello Server from client2"), string("client2.out"));
    thread lclient3(client, lport, string("Client Request: Hello Server from client3"), string("client3.out"));
    lclient1.join();
    lclient2.join();
    lclient3.join();

    z_server_ready = false;
    lserver.join();

    cout << "======== dump request queue ========" << endl;
    vector<string>::const_iterator lrequest_iter = zrequest_queue.begin();
    while (lrequest_iter != zrequest_queue.end()) {
        cout << *lrequest_iter << endl;
        lrequest_iter++;
    }
}

}