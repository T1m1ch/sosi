#pragma once
#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <fstream>
#include <format>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>


std::string constructResponse(std::string mime_type, size_t data_size, bool is_file = false);
std::string constructError(size_t status_code = 400);

class Server {
private:
	boost::asio::io_context io_context;
	boost::asio::ip::tcp::endpoint endpoint;
	boost::asio::ip::tcp::acceptor acceptor;
	std::map <std::string, std::function<void(boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket>, std::map<std::string, std::string> query_params)>> http_handlers;
	std::string chooseMimeType(std::string resource_path);
	std::vector<char> readFile(std::string resource_path);
public:
	void httpServe(std::string resource_path, std::shared_ptr<boost::asio::ip::tcp::socket> socket);
	void handleRequest(std::shared_ptr<boost::asio::ip::tcp::socket> socket, boost::beast::http::request<boost::beast::http::string_body> request);
	void readRequst(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
	void startAccept();
	Server();
	void handleFunc(std::string request_name, std::function<void(boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket>, std::map<std::string, std::string> query_params)> http_handler);
	void run();
};