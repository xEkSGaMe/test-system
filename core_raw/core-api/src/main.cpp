#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sstream>

std::string get_response(const std::string& method, const std::string& path) {
    if (method == "GET" && path == "/") {
        return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nCore API v1.0";
    }
    else if (method == "GET" && path == "/health") {
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"status\":\"healthy\",\"service\":\"core-api\"}";
    }
    else if (method == "GET" && path == "/tests") {
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n[{\"id\":1,\"title\":\"Test 1\"},{\"id\":2,\"title\":\"Test 2\"}]";
    }
    else if (method == "GET" && path.find("/tests/") == 0) {
        std::string id = path.substr(7);
        return "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"id\":" + id + ",\"title\":\"Test " + id + "\"}";
    }
    else if (method == "POST" && path == "/tests") {
        return "HTTP/1.1 201 Created\r\nContent-Type: application/json\r\n\r\n{\"status\":\"success\",\"message\":\"Test created\",\"id\":99}";
    }
    else {
        return "HTTP/1.1 404 Not Found\r\nContent-Type: application/json\r\n\r\n{\"error\":\"Not found\"}";
    }
}

int main() {
    std::cout << "=== CORE API SERVER ===" << std::endl;
    std::cout << "Starting on port 8080..." << std::endl;
    
    // Создаём сокет
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "ERROR: Cannot create socket" << std::endl;
        return 1;
    }
    
    // Разрешаем повторное использование порта
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "ERROR: setsockopt failed" << std::endl;
        return 1;
    }
    
    // Настраиваем адрес
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    
    // Привязываем
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "ERROR: Cannot bind to port 8080" << std::endl;
        close(server_fd);
        return 1;
    }
    
    // Начинаем слушать
    if (listen(server_fd, 10) < 0) {
        std::cerr << "ERROR: Cannot listen on port 8080" << std::endl;
        close(server_fd);
        return 1;
    }
    
    std::cout << "Server is listening on http://localhost:8080" << std::endl;
    std::cout << "Available endpoints:" << std::endl;
    std::cout << "  GET  /" << std::endl;
    std::cout << "  GET  /health" << std::endl;
    std::cout << "  GET  /tests" << std::endl;
    std::cout << "  GET  /tests/{id}" << std::endl;
    std::cout << "  POST /tests" << std::endl;
    std::cout << "======================" << std::endl;
    
    char buffer[4096] = {0};
    
    while (true) {
        std::cout << "Waiting for connection..." << std::endl;
        
        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "ERROR: Cannot accept connection" << std::endl;
            continue;
        }
        
        // Читаем запрос
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            
            // Парсим HTTP-запрос
            std::string request(buffer);
            std::string method = "GET";
            if (request.find("POST") == 0) method = "POST";
            
            std::string path = "/";
            size_t start = request.find(' ');
            if (start != std::string::npos) {
                size_t end = request.find(' ', start + 1);
                if (end != std::string::npos) {
                    path = request.substr(start + 1, end - start - 1);
                }
            }
            
            std::cout << method << " " << path << std::endl;
            
            // Получаем ответ
            std::string response = get_response(method, path);
            
            // Отправляем ответ
            send(client_fd, response.c_str(), response.length(), 0);
        }
        
        close(client_fd);
        std::cout << "Connection closed." << std::endl;
    }
    
    close(server_fd);
    return 0;
}
