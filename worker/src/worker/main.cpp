#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <memory>
#include "hydra.pb.h"

using boost::asio::ip::tcp;

// ---------------------------------------------------------
// FUNCTION: The Switchboard (Identify and Process Packets)
// ---------------------------------------------------------
void handle_connection(std::shared_ptr<tcp::socket> socket) {
    auto received_data = std::make_shared<std::string>();

    // Use async_read to capture the entire transmission until the client sends EOF
    boost::asio::async_read(*socket,
        boost::asio::dynamic_buffer(*received_data),
        [socket, received_data](const boost::system::error_code& error, std::size_t bytes_transferred) {
            
            if (error == boost::asio::error::eof) {
                try {
                    // --- STEP 1: SHARD DETECTION (STRICT) ---
                    hydrafs::FileShard shard;
                    // We check if it parses AND if the filename field is populated.
                    // This prevents "Shadowing" where a shard is misidentified as a heartbeat.
                    if (shard.ParseFromString(*received_data) && !shard.filename().empty()) {
                        std::cout << "[RECEIVE] Shard " << shard.shard_id() << " of " << shard.filename() 
                                  << " (" << shard.data().size() << " bytes)" << std::endl;
                        
                        std::string save_path = "/mnt/vault/" + shard.filename() + ".part" + std::to_string(shard.shard_id());
                        std::ofstream outfile(save_path, std::ios::out | std::ios::binary);
                        
                        if (outfile.is_open()) {
                            outfile.write(shard.data().c_str(), shard.data().size());
                            outfile.close();
                            std::cout << "[DISK] Saved piece to: " << save_path << std::endl;
                        } else {
                            std::cerr << "[DISK ERROR] Check /mnt/vault/ permissions!" << std::endl;
                        }
                        return;
                    }

                    // --- STEP 2: HEARTBEAT DETECTION ---
                    hydrafs::Heartbeat hb;
                    if (hb.ParseFromString(*received_data)) {
                        std::cout << "[HEARTBEAT] Ping from " << socket->remote_endpoint().address() << " ❤️" << std::endl;
                        
                        hb.set_status(hydrafs::Heartbeat_Status_PONG);
                        std::string response;
                        hb.SerializeToString(&response);
                        
                        // 🟢 FIX: Use error_code to handle "Broken Pipe" gracefully
                        boost::system::error_code ec;
                        boost::asio::write(*socket, boost::asio::buffer(response), ec);
                        
                        if (ec) {
                            std::cerr << "[NETWORK] Could not send PONG: " << ec.message() << " (Client hung up early)" << std::endl;
                        }
                        return; 
                    }

                    std::cerr << "[ERROR] Received " << bytes_transferred << " bytes but couldn't identify the packet type." << std::endl;

                } catch (const std::exception& e) {
                    std::cerr << "[CRITICAL] Logic Error during processing: " << e.what() << std::endl;
                }
            } else if (error && error != boost::asio::error::eof) {
                std::cerr << "[NETWORK] Connection error: " << error.message() << std::endl;
            }
        });
}

// ---------------------------------------------------------
// FUNCTION: Asynchronous Accept Loop
// ---------------------------------------------------------
void start_accept(tcp::acceptor& acceptor, boost::asio::io_context& io) {
    auto socket = std::make_shared<tcp::socket>(io);
    acceptor.async_accept(*socket, [&acceptor, &io, socket](const boost::system::error_code& error) {
        if (!error) {
            handle_connection(socket);
        }
        start_accept(acceptor, io); // Recurse to handle the next client
    });
}

// ---------------------------------------------------------
// MAIN ENGINE
// ---------------------------------------------------------
int main() {
    try {
        boost::asio::io_context io;
        tcp::acceptor acceptor(io);
        tcp::endpoint endpoint(tcp::v4(), 8080);

        acceptor.open(endpoint.protocol());
        acceptor.set_option(tcp::acceptor::reuse_address(true)); 
        acceptor.bind(endpoint);
        acceptor.listen();

        std::cout << "HydraFS Vault Online. Listening on port 8080..." << std::endl;
        
        start_accept(acceptor, io);
        io.run();

    } catch (std::exception& e) {
        std::cerr << "Fatal System Error: " << e.what() << std::endl;
    }
    return 0;
}