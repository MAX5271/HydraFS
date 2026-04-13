#include <iostream>
#include <fstream>
#include <string>

int main() {
    std::string base = "heavy_payload.bin";
    std::string output = "/mnt/vault/FINAL_FILE.bin";
    
    std::ofstream outfile(output, std::ios::binary);
    
    for (int i = 1; i <= 5; ++i) {
        std::string part = "/mnt/vault/" + base + ".part" + std::to_string(i);
        std::ifstream infile(part, std::ios::binary);
        
        if (!infile) {
            std::cerr << "Missing " << part << std::endl;
            return 1;
        }

        // Stream the part into the final file
        outfile << infile.rdbuf();
        std::cout << "Merged part " << i << "..." << std::endl;
    }

    outfile.close();
    std::cout << "Reassembly complete: " << output << std::endl;
    return 0;
}