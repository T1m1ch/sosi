#include "server.hpp"
#include <map>

std::string Server::chooseMimeType(std::string resource_path) {
    std::string file_extension = resource_path.substr(resource_path.rfind(".") + 1);

    if (file_extension == "html") {
        return "text/html";
    }
    if (file_extension == "jpg" || file_extension == "jpeg") {
        return "image/jpeg";
    }
    if (file_extension == "png") {
        return "image/png";
    }
    if (file_extension == "css") {
        return "text/css";
    }
    if (file_extension == "svg") {
        return "image/svg+xml";
    }

    if (file_extension == "webp") {
        return "image/webp";
    }

    return "";
}

std::vector<char> Server::readFile(std::string resource_path) {
    std::ifstream file(resource_path, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        file.seekg(0, file.end);
        std::vector <char> buffer(file.tellg());
        file.seekg(0, file.beg);
        file.read(buffer.data(), buffer.size());
        file.close();
        return buffer;
    } 
    file.close();
    return *(new std::vector<char>(0));
}

std::string constructResponse(std::string mime_type, size_t data_size, bool is_file) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << mime_type << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    if (is_file) {
        //response << "Transfer-Encoding: chunked\r\n";
    }
    response << "Content-Length: " << data_size << "\r\n\r\n";
    return response.str();
}

std::string constructError(size_t status_code) {
    std::string message;
    switch (status_code) {
    case 400:
        message = "Bad Request";
        break;
    case 404:
        message = "Not Found";
        break;
    }

    std::ostringstream response;
    response << "HTTP/1.1 " << status_code << " " << message << "\r\n";
    response << "Content-Type: " << "text/html" << "\r\n";
    response << "Access-Control-Allow-Origin: *\r\n";
    response << "Content-Length: " << 0 << "\r\n\r\n";
    return response.str();
}


void Server::httpServe(std::string resource_path, std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    resource_path = resource_path.substr(1);		
    std::vector<char> buffer = this->readFile(resource_path);

    std::string response = constructResponse(this->chooseMimeType(resource_path), buffer.size(), true);

    boost::asio::async_write(*socket, boost::asio::buffer(response.data(), response.size()), [socket, buffer](const boost::system::error_code& ec, size_t) {
        
        if (ec) {
            std::cerr << ec.message() << std::endl;
        } else {
            socket->write_some(boost::asio::buffer(buffer.data(), buffer.size()));
        }
   
    });
}


/*
            bool flag = false;
            size_t pos = 0;
            size_t chunck = 4096;
            size_t elements = 4096;
            while (!flag) {
                if ((buffer.size() - pos * chunck) < chunck) {
                    flag = true;
                    elements = buffer.size() - pos * chunck;
                }
                std::vector <char> slice(buffer.begin() + pos * chunck, buffer.begin() + pos * chunck + elements);

                std::ostringstream chunk;
                chunk << std::hex << elements << "\r\n";
                chunk.write(slice.data(), elements);
                chunk << "\r\n";

                boost::asio::async_write(*socket, boost::asio::buffer(chunk.str()), [](const boost::system::error_code& ec, size_t) {
                    if (ec) {
                        std::cerr << ec.message() << std::endl;
                    }
                });
                pos++;
            }
            */


void Server::handleRequest(std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::beast::http::request<boost::beast::http::string_body> request) {
    std::string url, _params_str;
    std::map<std::string, std::string> query_params;
    
    if (request.target().find("?") != std::string::npos) {
        url = request.target().substr(0, request.target().find("?"));
        _params_str = request.target().substr(request.target().find("?") + 1);
        std::pair <std::string, std::string> pair;
        bool first = true;
        for (char elem : _params_str) {
           if (first) {
               if (elem == '=') {
                   first = false;
                   continue;
               }
               pair.first += elem;
           } else {
               if (elem == '&') {
                   first = true;
                   query_params.insert(pair);
                   pair.first = "";
                   pair.second = "";
                   continue;
               }
               pair.second += elem;
           }
        }
        query_params.insert(pair);
    } else {
        url = request.target();
    }
    
        std::cout << request.target() << std::endl;

    if (this->http_handlers.find(url) != this->http_handlers.end()) {
        this->http_handlers[url](request, socket, query_params);
    } else if (request.target().find(".") != std::string::npos) {
        this->httpServe(request.target(), socket);
    } else {
        boost::beast::http::response<boost::beast::http::string_body> response{boost::beast::http::status::not_found, 11};
        boost::beast::http::async_write(*socket, response, [](const boost::system::error_code &ec, size_t) {
            if (ec) {
                std::cerr << ec.message() << std::endl;
            }
        });
    }
}

void Server::readRequst(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    auto streambuf = std::make_shared<boost::asio::streambuf>();

    boost::asio::async_read_until(*socket, *streambuf, "\r\n\r\n", [this, streambuf, socket](const boost::system::error_code &ec, size_t size) {
        if (ec && ec != boost::asio::error::eof) {
            std::cerr << ec.message() << std::endl;
        } else {
            boost::beast::http::request<boost::beast::http::string_body> request;
            boost::beast::error_code request_ec;
            boost::beast::http::read(*socket, *streambuf, request, request_ec);
            this->handleRequest(socket, request);
        }
    });
}

void Server::startAccept() {
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(this->io_context);
    this->acceptor.async_accept(*socket, [this, socket](const boost::system::error_code &ec) {
        if (ec) {	
            std::cerr << ec.message();
        } else {
            this->readRequst(socket);
            this->startAccept();
        }
    });
}


Server::Server() : endpoint(boost::asio::ip::tcp::v4(), 80), acceptor(io_context, endpoint) {
    this->startAccept();
}
void Server::handleFunc(std::string request_name, std::function<void(boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket>, std::map<std::string, std::string>)> http_handler) {
    this->http_handlers.insert({request_name, http_handler});
}
void Server::run() {
    this->io_context.run();
}
