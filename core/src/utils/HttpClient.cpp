#include "HttpClient.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include "Logger.hpp"

std::optional<std::string> HttpClient::sendRequest(
    const std::string& method,
    const std::string& url,
    const std::map<std::string, std::string>& headers,
    const std::string& body) {
    
    std::string host, path;
    int port = 80;
    
    if (!parseUrl(url, host, path, port)) {
        Logger::error("Failed to parse URL", {{"url", url}});
        return std::nullopt;
    }
    
    // Создаем сокет
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        Logger::error("Socket creation failed", {});
        return std::nullopt;
    }
    
    // Получаем IP адрес хоста
    struct hostent* server = gethostbyname(host.c_str());
    if (server == NULL) {
        Logger::error("No such host", {{"host", host}});
        close(sock);
        return std::nullopt;
    }
    
    // Настраиваем адрес сервера
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(port);
    
    // Устанавливаем таймаут на подключение
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    
    // Подключаемся к серверу
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        Logger::error("Connection failed", {{"host", host}, {"port", std::to_string(port)}});
        close(sock);
        return std::nullopt;
    }
    
    // Формируем HTTP-запрос
    std::stringstream request;
    request << method << " " << path << " HTTP/1.1\r\n";
    request << "Host: " << host << "\r\n";
    
    // Добавляем заголовки
    for (const auto& [key, value] : headers) {
        request << key << ": " << value << "\r\n";
    }
    
    // Если есть тело, добавляем Content-Length
    if (!body.empty()) {
        request << "Content-Length: " << body.length() << "\r\n";
    }
    
    request << "Connection: close\r\n";
    request << "\r\n";
    
    if (!body.empty()) {
        request << body;
    }
    
    std::string request_str = request.str();
    
    // Отправляем запрос
    if (send(sock, request_str.c_str(), request_str.length(), 0) < 0) {
        Logger::error("Send failed", {});
        close(sock);
        return std::nullopt;
    }
    
    // Читаем ответ
    char buffer[4096];
    std::string response;
    ssize_t bytes_read;
    
    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        response.append(buffer, bytes_read);
    }
    
    close(sock);
    
    // Парсим ответ, извлекаем тело
    size_t header_end = response.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return std::nullopt;
    }
    
    return response.substr(header_end + 4);
}

std::optional<std::string> HttpClient::validateTokenWithAuthService(
    const std::string& token) {
    
    std::map<std::string, std::string> headers = {
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer " + token}
    };
    
    // Отправляем запрос на auth-service
    return sendRequest(
        "POST",
        "http://auth-service:8080/auth/validate",
        headers,
        "{}"  // Пустое тело, так как токен в заголовке
    );
}

bool HttpClient::parseUrl(const std::string& url, 
                         std::string& host, 
                         std::string& path, 
                         int& port) {
    
    if (url.find("http://") == 0) {
        std::string rest = url.substr(7);  // Убираем "http://"
        
        size_t slash_pos = rest.find('/');
        if (slash_pos != std::string::npos) {
            host = rest.substr(0, slash_pos);
            path = rest.substr(slash_pos);
        } else {
            host = rest;
            path = "/";
        }
        
        // Проверяем есть ли порт в хосте
        size_t colon_pos = host.find(':');
        if (colon_pos != std::string::npos) {
            std::string port_str = host.substr(colon_pos + 1);
            port = std::stoi(port_str);
            host = host.substr(0, colon_pos);
        } else {
            port = 80;
        }
        
        return true;
    }
    
    return false;
}