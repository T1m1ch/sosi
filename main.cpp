#include <iostream>
#include <vector>
#include <sstream>
#include <map>
#include <fstream>
#include <format>

#include <mongocxx/v_noabi/mongocxx/client.hpp>
#include <bsoncxx/v_noabi/bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/v_noabi/bsoncxx/json.hpp>
#include <mongocxx/v_noabi/mongocxx/instance.hpp>

#include "server.hpp"

int main() {
	mongocxx::instance instance{};
	Server server;
	server.handleFunc("/", [&](boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::map<std::string, std::string> query_params) {
		server.httpServe("/index.html", socket);
	});
	server.handleFunc("/get-event-list", [&](boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::map<std::string, std::string> query_params) {
		mongocxx::client conection(mongocxx::uri{});
		mongocxx::collection collection = conection["mydb"]["events"];
		mongocxx::cursor cursor = collection.find({});
        
		std::string event_id_list = "[";
		
		for (auto&& doc : cursor) {
			std::string _document_json_str = bsoncxx::to_json(doc);
			for (const auto& element : doc) {
				if (element.key() == "_id") {
					event_id_list += element.get_value().get_oid().value.to_string();
					event_id_list += ", ";
				}
			}
		}
		event_id_list.erase(event_id_list.size()-2);
		event_id_list += "]";
		
		std::string response = constructResponse("text/plain", event_id_list.size());
		
		boost::asio::async_write(*socket, boost::asio::buffer(response.data(), response.size()), [socket, event_id_list](const boost::system::error_code& ec, size_t) {
			if (ec) {
				std::cerr << ec.message() << std::endl;
			}
			else {
				boost::asio::async_write(*socket, boost::asio::buffer(event_id_list.data(), event_id_list.size()), [socket](const boost::system::error_code& ec, size_t) {
					if (ec) {
						std::cerr << ec.message() << std::endl;
					}
				});
			}
		});
	});
	server.handleFunc("/get-event-info", [&](boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::map<std::string, std::string> query_params) {
		mongocxx::client connection(mongocxx::uri{});
		mongocxx::collection collection = connection["mydb"]["events"];
		
		bsoncxx::builder::stream::document filter_builder;
		filter_builder << "_id" << query_params["id"];
		auto filter = filter_builder.view();
		mongocxx::options::find options;
		mongocxx::cursor cursor = collection.find(filter, options);
		std::string document;
		std::string response;
		for (auto && elem : cursor) {
			document = bsoncxx::to_json(elem);
			response = constructResponse("application/json", bsoncxx::to_json(elem).size());
			boost::asio::async_write(*socket, boost::asio::buffer(response.data(), response.size()), [socket, elem, document](const boost::system::error_code& ec, size_t) {
				if (ec) {
					std::cerr << ec.message() << std::endl;
				} else {
					boost::asio::async_write(*socket, boost::asio::buffer(document), [](const boost::system::error_code& ec, size_t) {
						if (ec) {
							std::cerr << ec.message() << std::endl;
						}
					});
				}
			});
			return;
		}
		response = constructError(400);
		boost::asio::async_write(*socket, boost::asio::buffer(response.data(), response.size()), [](const boost::system::error_code &ec, size_t) {
			if (ec) {
				std::cerr << ec.message() << std::endl;
			}
		});
	});
	server.handleFunc("/upload-event", [&](boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::map<std::string, std::string> query_params) {
		boost::json::object _json_object = boost::json::parse(request.body()).as_object();
		mongocxx::client connection(mongocxx::uri{});
		mongocxx::collection collection = connection["mydb"]["events"];
		bsoncxx::builder::stream::document document;
		document 
			<< "_id" << bsoncxx::oid().to_string()
			<< "name" << static_cast<std::string>(_json_object["name"].as_string().c_str())
			<< "descriprion" << static_cast<std::string>(_json_object["description"].as_string().c_str())
			<< "date" << static_cast<std::string>(_json_object["date"].as_string().c_str())
			<< "price" << static_cast<int32_t>(_json_object["price"].as_int64())
			<< "images" << static_cast<int32_t>(_json_object["images"].as_int64());
		auto result = collection.insert_one(document.view());
		std::string response = result
			
		?constructResponse("text/plain", 0, false)
		:constructError(400);

		boost::asio::async_write(*socket, boost::asio::buffer(response.data(), response.size()), [](const boost::system::error_code &ec, size_t) {
			if (ec) {
				std::cerr << ec.message() << std::endl;
			}
		});
	});
	server.handleFunc("/delete-event", [&](boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::map<std::string, std::string> query_params) {
		mongocxx::client conection(mongocxx::uri{});
		mongocxx::collection collection = conection["mydb"]["events"];
		bsoncxx::builder::stream::document filter_builder;
		filter_builder << "_id" << query_params["id"];
		auto filter = filter_builder.view();
		mongocxx::options::delete_options options;
		auto result = collection.delete_one(filter, options);
		
		std::string response = result->deleted_count() > 0
		?constructResponse("text/plain", 0)
		:constructError(404);
		boost::asio::async_write(*socket, boost::asio::buffer(response.data(), response.size()), [](const boost::system::error_code &ec, size_t) {
			if (ec) {
				std::cerr << ec.message() << std::endl;
			}
		});
	});
	server.handleFunc("/buy-ticket", [&](boost::beast::http::request<boost::beast::http::string_body> request, std::shared_ptr<boost::asio::ip::tcp::socket> socket, std::map<std::string, std::string> query_params) {
		boost::json::object json_object = boost::json::parse(request.body()).as_object();

	});
	server.run();

	//std::cout << strstream.str();
	return 0;
}