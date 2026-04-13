#include <iostream>
#include <boost/asio.hpp>
#include <memory>

using boost::asio::ip::tcp;

// ---------------------------------------------------------
// FUNCTION: Handle the connection once someone knocks
// ---------------------------------------------------------
void handle_connection(std::shared_ptr<tcp::socket> socket) {
    // 1. Create a message to send back to the laptop
    std::string message = "HydraFS Worker Node: Connection Established! 🚀\n";

    // 2. Asynchronously send the message
    boost::asio::async_write(*socket, boost::asio::buffer(message),
        [socket](const boost::system::error_code& error, std::size_t /*bytes*/) {
            if (!error) {
                std::cout << "Message sent to client successfully." << std::endl;
            }
        });
}

// ---------------------------------------------------------
// FUNCTION: The Receptionist (Wait for knocks)
// ---------------------------------------------------------
void start_accept(tcp::acceptor& acceptor, boost::asio::io_context& io) {
    // 1. Create a new, blank socket (a new telephone line)
    auto socket = std::make_shared<tcp::socket>(io);

    // 2. Wait for someone to connect to this socket asynchronously
    acceptor.async_accept(*socket, [&acceptor, &io, socket](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "\n[NEW CONNECTION] Client IP: " << socket->remote_endpoint().address().to_string() << std::endl;
            
            // Pass the connected socket to our handler function
            handle_connection(socket);
        }

        // 3. IMPORTANT: The receptionist must immediately go back to waiting for the NEXT knock!
        start_accept(acceptor, io);
    });
}

// ---------------------------------------------------------
// MAIN ENGINE
// ---------------------------------------------------------
int main() {
    try {
        boost::asio::io_context io;

        // Start the Receptionist on Port 8080
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
        
        std::cout << "HydraFS Worker listening on port 8080... (Press Ctrl+C to stop)" << std::endl;

        // Tell the receptionist to start waiting
        start_accept(acceptor, io);

        // Start the engine! This blocks forever, handling network events.
        io.run();

    } catch (std::exception& e) {
        std::cerr << "Network Error: " << e.what() << std::endl;
    }

    return 0;
}