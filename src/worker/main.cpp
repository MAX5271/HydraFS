#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <memory>
#include "hydra.pb.h"

using boost::asio::ip::tcp;

// ---------------------------------------------------------
// FUNCTION: Handle incoming binary data & Save to Disk
// ---------------------------------------------------------
void handle_connection(std::shared_ptr<tcp::socket> socket) {
    auto received_data = std::make_shared<std::string>();

    boost::asio::async_read(*socket,
        boost::asio::dynamic_buffer(*received_data),
        [socket, received_data](const boost::system::error_code& error, std::size_t bytes_transferred) {
            
            if (error == boost::asio::error::eof) {
                try {
                    // --- SWITCHBOARD LOGIC ---

                    // 1. Try Shard FIRST
                    // We check if the filename is not empty to verify it's a real FileShard
                    hydrafs::FileShard shard;
                    if (shard.ParseFromString(*received_data) && !shard.filename().empty()) {
                        std::cout << "[RECEIVE] Shard " << shard.shard_id() << " of " << shard.filename() << std::endl;
                        
                        std::string save_path = "/mnt/vault/" + shard.filename() + ".part" + std::to_string(shard.shard_id());
                        std::ofstream outfile(save_path, std::ios::out | std::ios::binary);
                        
                        if (outfile.is_open()) {
                            outfile.write(shard.data().c_str(), shard.data().size());
                            outfile.close();
                            std::cout << "[DISK] Saved piece to: " << save_path << std::endl;
                        } else {
                            std::cerr << "[DISK ERROR] Check /mnt/vault/ permissions!" << std::endl;
                        }
                        return; // Successfully processed a shard, exit the lambda
                    }

                    // 2. Fallback to Heartbeat
                    hydrafs::Heartbeat hb;
                    if (hb.ParseFromString(*received_data) && hb.status() == hydrafs::Heartbeat_Status_PING) {
                        std::cout << "[HEARTBEAT] Ping from " << socket->remote_endpoint().address() << " ❤️" << std::endl;
                        
                        hb.set_status(hydrafs::Heartbeat_Status_PONG);
                        std::string response;
                        hb.SerializeToString(&response);
                        
                        boost::system::error_code ec;
                        boost::asio::write(*socket, boost::asio::buffer(response), ec);
                        
                        if (ec) {
                            std::cerr << "[NETWORK] Could not send PONG: " << ec.message() << std::endl;
                        }
                        return; 
                    }

                } catch (const std::exception& e) {
                    std::cerr << "[CRITICAL] Logic Error: " << e.what() << std::endl;
                }
            } else if (error && error != boost::asio::error::eof) {
                std::cerr << "[NETWORK] Connection dropped: " << error.message() << std::endl;
            }
        });
}

// ---------------------------------------------------------
// FUNCTION: The Receptionist
// ---------------------------------------------------------
void start_accept(tcp::acceptor& acceptor, boost::asio::io_context& io) {
    auto socket = std::make_shared<tcp::socket>(io);
    acceptor.async_accept(*socket, [&acceptor, &io, socket](const boost::system::error_code& error) {
        if (!error) handle_connection(socket);
        start_accept(acceptor, io); 
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

        std::cout << "HydraFS Vault Online. Listening on 8080..." << std::endl;
        
        start_accept(acceptor, io);
        io.run();

    } catch (std::exception& e) {
        std::cerr << "Fatal System Error: " << e.what() << std::endl;
    }
    return 0;
}