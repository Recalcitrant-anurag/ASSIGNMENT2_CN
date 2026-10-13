# NetworkingProject01

**COMPUTER NETWORK project of fifth semester engineering.**
**Project Chosen:** Project 1 (Designing a Proxy Server)

## SECTION - CSE 31

**Group Members:**

1. ADARSH MALVIYA - Roll No: 2405703
2. ASHISH KUMAR MANDAL - Roll No: 24158048
3. ANURAG ANAND - Roll No: 2405417

## WHY WE CHOSE THIS PARTICULAR PROJECT?

We selected the "Designing a Proxy Server" project for three specific reasons aligned with our educational goals:

* **Direct Application-Layer Visibility:** Unlike other options that deal with complex bit-level chunking or hardware, a proxy server handles HTTP requests. Because HTTP is human-readable plaintext, we can easily print, inspect, and analyze network packets directly in our console, which accelerates our understanding of how network protocols function.
* **Core Networking Fundamentals:** This project forces us to deal with essential real-world networking concepts such as socket programming, TCP handshakes, multi-threaded connection handling, data caching and network security (access control lists) within a clean scope.
* **Incremental Development:** The structure of a proxy server allows for modular development. We can start with a basic single-client request forwarder, and progressively layer on multi-threading, logging, filtering and performance benchmarking day-by-day.

## FEATURES OF THE FINAL PROJECT

Our completed application implements the following features to meet the core criteria of the assignment description:

* **Concurrent Connection Handling:** The server utilizes the C++ `<thread>` standard library to accept connections from multiple simultaneous clients without letting any single client's request block others.
* **HTTP Request Parsing & Forwarding:** The proxy correctly intercepts, parses and dynamically forwards standard HTTP protocol requests, returning valid server responses cleanly back to the client application.
* **Response Caching Mechanism:** We implemented an in-memory cache to store static web content, applying a 60-second Time-To-Live (TTL) validation policy to serve cached data quickly instead of fetching it from the origin server every time.
* **Access Control & Domain Filtering:** An administrative rule layer is integrated to block access to specific blacklisted domains/URLs, demonstrating basic network firewall controls.
* **Comprehensive Network Logging:** The proxy actively outputs formatted logs tracking requested URLs, response statuses, cache hits, and timestamps for administrative auditing.
* **Robust Error Handling:** The application handles malformed requests and unreachable host exceptions gracefully (using socket timeouts) without crashing the core listening loop.
* **Quantitative Performance Metrics:** Built-in timing mechanisms measure and report network latency/throughput in milliseconds.

## DECLARATION OF AI USAGE & LIMITATIONS

To maintain complete academic transparency and ensure deep conceptual readiness for our final demonstration and viva evaluation, we declare our intent to use AI tools under these strict pre-defined boundaries:

* **Code Template Generation:** AI was used to generate initial boilerplate templates for low-level socket connections and multi-threading loops.
* **Architectural & Concepts Guide:** AI was used as a digital textbook to explain networking mechanics line-by-line.
* **Real-Time Error Interpretation:** Used as a debugger to isolate failure points for cryptic runtime errors common to socket interfaces.

*Every single line of code added to the final project was manually reviewed, tested, modified, and documented with explanatory comments written by our group members to ensure absolute readiness for any live viva or demonstration testing.*

**1. Start the Server (Window 1)**

Run this to compile the multi-threaded server and start listening on Port 8080:

```bash
g++ proxy.cpp -o proxy -pthread
./proxy
```

**2. Prove Concurrency & Internet Relay (Window 2 & 3)**
Type these into two separate terminals and press Enter on both at the exact same time. Window 1 will log `[FETCHING]` for both simultaneously, proving the main loop doesn't block.
Window 2:

```bash
curl --proxy 127.0.0.1:8080 http://neverssl.com
```

Window 3:

```bash
curl --proxy 127.0.0.1:8080 http://httpbin.org/get

```

**3. Prove Caching & Performance Metrics (Window 2)**
Within 60 seconds of running the previous step, press the Up arrow in Window 2 and hit Enter again. Window 1 will instantly log `[CACHE HIT]` and print a drastically lower response time (e.g., 2 ms vs 4000 ms).

```bash
curl --proxy 127.0.0.1:8080 http://neverssl.com
```

**4. Prove Access Control / Blacklist (Window 3)**
Run this to test the firewall logic. Window 1 will log `[BLOCKED]` and drop the connection, returning your custom `<h1>Access Denied</h1>` HTML. *(Note: Replace badsite.com with the exact domain in your proxy.cpp blacklist array).*

```bash
curl --proxy 127.0.0.1:8080 http://blocked.com
```
