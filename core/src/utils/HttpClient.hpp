#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include <string>
#include <optional>
#include <map>

class HttpClient {
public:
    // Метод для отправки HTTP-запроса
    static std::optional<std::string> sendRequest(
        const std::string& method,
        const std::string& url,
        const std::map<std::string, std::string>& headers = {},
        const std::string& body = ""
    );
    
    // Специальный метод для проверки токена через Auth-сервис
    static std::optional<std::string> validateTokenWithAuthService(
        const std::string& token
    );
    
private:
    static bool parseUrl(const std::string& url, 
                        std::string& host, 
                        std::string& path, 
                        int& port);
};

#endif // HTTP_CLIENT_HPP