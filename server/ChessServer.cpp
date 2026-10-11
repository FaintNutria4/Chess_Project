#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>
#include <string>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using asio::ip::tcp;

class ChessSession : public std::enable_shared_from_this<ChessSession> {
public:
    explicit ChessSession(websocket::stream<beast::tcp_stream> ws)
        : ws_(std::move(ws)) {}

    void start(const http::request<http::string_body>& req) {
        ws_.async_accept(req,
            [self = shared_from_this()](beast::error_code ec) {
                if (ec) {
                    std::cout << "Error en el handshake: " << ec.message() << "\n";
                    return;
                }
                self->read_message();
            });
    }

private:
    void read_message() {
        ws_.async_read(buffer_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec == websocket::error::closed) {
                    std::cout << "Jugador desconectado\n";
                    return;
                }
                if (ec) {
                    std::cout << "Error al leer: " << ec.message() << "\n";
                    return;
                }
                std::string payload = beast::buffers_to_string(self->buffer_.data());
                self->buffer_.consume(self->buffer_.size());
                std::cout << "Movimiento recibido: " << payload << "\n";
                self->write_message("ACK: " + payload);
            });
    }

    void write_message(std::string message) {
        out_ = std::move(message);
        ws_.async_write(asio::buffer(out_),
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cout << "Error al escribir: " << ec.message() << "\n";
                    return;
                }
                self->read_message();
            });
    }

    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string out_;
};

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    explicit HttpSession(tcp::socket socket)
        : stream_(std::move(socket)) {}

    void run() {
        read_request();
    }

private:
    void read_request() {
        http::async_read(stream_, buffer_, request_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cout << "Error al leer petición: " << ec.message() << "\n";
                    return;
                }
                if (websocket::is_upgrade(self->request_)) {
                    websocket::stream<beast::tcp_stream> ws(std::move(self->stream_));
                    ws.binary(false);
                    std::make_shared<ChessSession>(std::move(ws))->start(self->request_);
                    return;
                }
                self->write_response();
            });
    }

    void write_response() {
        response_ = std::make_shared<http::response<http::string_body>>(
            http::status::ok, request_.version(), "chess_server\n");
        response_->set(http::field::server, "chess_server");
        response_->set(http::field::content_type, "text/plain");
        response_->keep_alive(false);
        response_->prepare_payload();
        http::async_write(stream_, *response_,
            [self = shared_from_this()](beast::error_code ec, std::size_t) {
                if (ec) {
                    std::cout << "Error al escribir respuesta: " << ec.message() << "\n";
                    return;
                }
                beast::error_code shutdown_ec;
                self->stream_.socket().shutdown(tcp::socket::shutdown_both, shutdown_ec);
            });
    }

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    http::request<http::string_body> request_;
    std::shared_ptr<http::response<http::string_body>> response_;
};

class ChessServer {
public:
    ChessServer(asio::io_context& io_context, tcp::endpoint endpoint)
        : acceptor_(io_context, endpoint) {
        start_accept();
    }

private:
    void start_accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if (ec == asio::error::operation_aborted) {
                    return;
                }
                if (!ec) {
                    std::cout << "¡Nuevo jugador conectado desde: "
                              << socket.remote_endpoint() << "!\n";
                    std::make_shared<HttpSession>(std::move(socket))->run();
                }
                start_accept();
            });
    }

    tcp::acceptor acceptor_;
};

int main() {
    try {
        asio::io_context io_context{1};

        std::cout << "Iniciando servidor de ajedrez en el puerto 8080...\n";
        ChessServer server(io_context, tcp::endpoint(tcp::v4(), 8080));

        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Excepción en el servidor: " << e.what() << "\n";
    }
    return 0;
}
