#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include "hydra.pb.h"

using boost::asio::ip::tcp;

// ---------------------------------------------------------
// FUNCTION: Handle incoming binary data & Save to Disk
// ---------------------------------------------------------
void handle_connection(std::shared_ptr<tcp::socket> socket) {
    
    // 1. Use a dynamic string that grows automatically. No more hardcoded 1MB limits!
    auto received_data = std::make_shared<std::string>();

    // 2. Use async_read (not some). This loops automatically under the hood.
    boost::asio::async_read(*socket,
        boost::asio::dynamic_buffer(*received_data),
        [socket, received_data](const boost::system::error_code& error, std::size_t bytes_transferred) {
            
            // 3. EOF means the client finished sending the chunk and closed the socket. This is SUCCESS.
            if (error == boost::asio::error::eof) {
                std::cout << "\n[DATA RECEIVED] " << received_data->size() << " total bytes captured." << std::endl;

                hydrafs::FileShard shard;
                
                // 4. Parse from the string we accumulated
                if (shard.ParseFromString(*received_data)) {
                    std::cout << "--- 📦 Shard Manifest Decoded ---" << std::endl;
                    std::cout << "Target File : " << shard.filename() << std::endl;
                    std::cout << "Piece       : " << shard.shard_id() << " of " << shard.total_shards() << std::endl;
                    
                    std::string save_path = "/mnt/vault/" + shard.filename() + ".part" + std::to_string(shard.shard_id());
                    std::ofstream outfile(save_path, std::ios::out | std::ios::binary);
                    
                    if (outfile.is_open()) {
                        outfile.write(shard.data().c_str(), shard.data().size());
                        outfile.close();
                        std::cout << "[DISK SUCCESS] Shard securely written to: " << save_path << std::endl;
                    } else {
                        std::cerr << "[DISK ERROR] Could not open " << save_path << ". Check folder permissions!" << std::endl;
                    }
                    std::cout << "---------------------------------\n" << std::endl;

                } else {
                    std::cerr << "[ERROR] Failed to parse the binary data." << std::endl;
                }
            } else if (error) {
                std::cerr << "Network read error: " << error.message() << std::endl;
            }
        });
}

// ---------------------------------------------------------
// FUNCTION: The Receptionist (Wait for knocks)
// ---------------------------------------------------------
void start_accept(tcp::acceptor& acceptor, boost::asio::io_context& io) {
    auto socket = std::make_shared<tcp::socket>(io);

    acceptor.async_accept(*socket, [&acceptor, &io, socket](const boost::system::error_code& error) {
        if (!error) {
            std::cout << "\n[NEW CONNECTION] Client IP: " << socket->remote_endpoint().address().to_string() << std::endl;
            handle_connection(socket);
        }
        start_accept(acceptor, io); // Go back to waiting
    });
}

// ---------------------------------------------------------
// MAIN ENGINE
// ---------------------------------------------------------
int main() {
    try {
        boost::asio::io_context io;
        tcp::acceptor acceptor(io, tcp::endpoint(tcp::v4(), 8080));
        
        std::cout << "HydraFS Storage Engine listening on port 8080..." << std::endl;
        
        start_accept(acceptor, io);
        io.run();

    } catch (std::exception& e) {
        std::cerr << "Network Error: " << e.what() << std::endl;
    }
    return 0;
}