#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <mutex>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>

using namespace std;

// ==========================================
// CONFIGURATION & GLOBAL VARIABLES
// ==========================================

// Access Control: List of domains that the proxy will block.
vector<string> blacklist = {"blocked.com", "example.com"};

// Cache Structure: Holds the actual web page data AND the exact time it was saved.
struct CacheEntry {
    string response_data;
    chrono::time_point<chrono::steady_clock> timestamp;
};

// The in-memory cache mapping a request string to its CacheEntry
unordered_map<string, CacheEntry> cache;

// Mutex (Lock): Prevents the program from crashing if two concurrent 
// threads try to read/write to the cache at the exact same millisecond.
mutex cache_lock; 

// ==========================================
// HELPER FUNCTIONS
// ==========================================

// Parses the raw HTTP request to find the destination website (e.g., "www.google.com")
string get_host(string request) {
    size_t pos = request.find("Host: ");
    if (pos == string::npos) return ""; // Malformed request check
    size_t end = request.find("\r\n", pos);
    return request.substr(pos + 6, end - pos - 6);
}

// Configures a socket to timeout after 3 seconds of inactivity.
void set_socket_timeout(int socket_fd) {
    struct timeval timeout;
    timeout.tv_sec = 3;  // 3 seconds
    timeout.tv_usec = 0;
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
}

// ==========================================
// CORE CLIENT HANDLING THREAD
// ==========================================

// This function runs independently for every single user that connects.
void handle_client(int client_sock) {
    // Start measuring performance overhead (Latency)
    auto start_time = chrono::high_resolution_clock::now(); 

    // Apply timeout rules to the client connection
    set_socket_timeout(client_sock);

    // 1. Read the HTTP request from the browser
    char buffer[8192] = {0};
    int bytes_read = read(client_sock, buffer, 8191);
    
    if (bytes_read <= 0) {
        close(client_sock); // Gracefully close if nothing is sent
        return; 
    }

    string request(buffer);
    string host = get_host(request);

    // Handle Malformed Requests Gracefully
    if (host.empty()) { 
        close(client_sock); 
        return; 
    }

    // 2. Access Control (Domain Filtering)
    for (const string& bad_domain : blacklist) {
        if (host.find(bad_domain) != string::npos) {
            cout << "[BLOCKED] Access to " << host << " denied by admin rules.\n";
            string msg = "HTTP/1.1 403 Forbidden\r\n\r\n<h1>Access Denied by Proxy Firewall</h1>";
            send(client_sock, msg.c_str(), msg.length(), 0);
            close(client_sock);
            return;
        }
    }

    // 3. Sensible Response Caching (with 60-second expiration policy)
    cache_lock.lock();
    if (cache.count(request)) {
        auto now = chrono::steady_clock::now();
        auto age = chrono::duration_cast<chrono::seconds>(now - cache[request].timestamp).count();
        
        if (age < 60) { // If the cache is less than 60 seconds old, use it
            cout << "[CACHE HIT] Serving " << host << " from memory (Age: " << age << "s)\n";
            send(client_sock, cache[request].response_data.c_str(), cache[request].response_data.length(), 0);
            cache_lock.unlock();
            close(client_sock);
            return;
        } else {
            cout << "[CACHE EXPIRED] " << host << " data is stale. Re-fetching...\n";
            cache.erase(request); // Remove old data
        }
    }
    cache_lock.unlock();

    // 4. HTTP Request Forwarding (Relay to actual server)
    cout << "[FETCHING] Requesting fresh data from " << host << "\n";
    struct hostent* server = gethostbyname(host.c_str());
    
    // Handle Unreachable Hosts Gracefully
    if (!server) { 
        cout << "[ERROR] Host unreachable: " << host << "\n";
        close(client_sock); 
        return; 
    }

    // Create a new socket to talk to the actual website
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    set_socket_timeout(server_sock); // Protect against slow servers
    
    struct sockaddr_in server_addr = { .sin_family = AF_INET, .sin_port = htons(80) };
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // Connect to the actual website
    if (connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cout << "[ERROR] Connection to " << host << " timed out.\n";
        close(client_sock); close(server_sock); return;
    }

    // Send the client's original HTTP request to the real server
    send(server_sock, request.c_str(), request.length(), 0);

    // Read the server's response and stream it back to the browser chunk-by-chunk
    string response_data;
    memset(buffer, 0, sizeof(buffer));
    
    while ((bytes_read = read(server_sock, buffer, 8191)) > 0) {
        send(client_sock, buffer, bytes_read, 0); // Relay to browser
        response_data.append(buffer, bytes_read); // Save for our cache
        memset(buffer, 0, sizeof(buffer));
    }

    // 5. Save the new response to cache with the current timestamp
    cache_lock.lock();
    cache[request] = {response_data, chrono::steady_clock::now()};
    cache_lock.unlock();

    // 6. Performance Metrics Measurement
    auto end_time = chrono::high_resolution_clock::now();
    chrono::duration<double, std::milli> ms = end_time - start_time;
    cout << "[METRICS] Request to " << host << " handled in " << ms.count() << " ms\n\n";

    // Clean up connections
    close(server_sock);
    close(client_sock);
}

// ==========================================
// MAIN SERVER LISTENER
// ==========================================
int main() {
    // Setup the main proxy server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    
    // SO_REUSEADDR prevents the "Address already in use" error if you restart the proxy quickly
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); 
    
    // Bind to port 8080 on the local machine
    struct sockaddr_in address = { .sin_family = AF_INET, .sin_port = htons(8080), .sin_addr = { INADDR_ANY } };
    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    
    // Start listening for incoming browser connections (queue up to 10 at a time)
    listen(server_fd, 10);
    
    cout << "=================================================\n";
    cout << " Group 31 Proxy Server Running on Port 8080\n";
    cout << "=================================================\n";

    // Infinite loop: Keep the server running forever
    while (true) {
        // Wait for a browser to connect...
        int client_sock = accept(server_fd, nullptr, nullptr);
        
        if (client_sock < 0) continue; // Skip bad connections
        
        // CONCURRENT CONNECTION HANDLING: Spawn a new detached thread for every incoming connection.
        thread(handle_client, client_sock).detach(); 
    }
    
    return 0;
}