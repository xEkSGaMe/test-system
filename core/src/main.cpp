#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

void handle_get(http_request request) {
    json::value response;
    response[U("message")] = json::value::string(U("Core Service (C++) - Under Construction"));
    
    request.reply(status_codes::OK, response);
}

void handle_health(http_request request) {
    json::value response;
    response[U("status")] = json::value::string(U("OK"));
    
    request.reply(status_codes::OK, response);
}

int main() {
    const char* port = std::getenv("PORT");
    if (!port) port = "8082";
    
    std::string address = "http://0.0.0.0:";
    address += port;
    
    http_listener listener(address);
    
    listener.support(methods::GET, handle_get);
    listener.support(methods::GET, [](http_request request) {
        if (request.relative_uri().path() == U("/health")) {
            handle_health(request);
        } else {
            handle_get(request);
        }
    });
    
    try {
        listener.open().wait();
        std::cout << "Core service starting on " << address << std::endl;
        
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        
        listener.close().wait();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}