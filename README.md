# HydraFS - Storage Engine (The Vault) 🛡️

The **Storage Engine** is the backbone of the HydraFS distributed system. Deployed on a Raspberry Pi, it acts as a "Vault" that listens for incoming data shards, manages health-check heartbeats, and persists binary data to a high-capacity external storage array.

## 🚀 Features

* **Asynchronous Processing**: Built with `Boost.Asio` to handle concurrent network requests without blocking the main execution thread.
* **Indestructible Switchboard**: Smart packet identification that distinguishes between `Heartbeat` PINGs and `FileShard` data packets.
* **Fault-Tolerant Networking**: Implements error-code capturing to survive `Broken Pipe` and `Connection Reset` scenarios common in wireless environments.
* **Vault Persistence**: Configured to write data directly to an external 1TB mount point with binary-safe I/O.
* **Reassembly Utility**: Includes a specialized `stitch` tool to merge sharded pieces back into a single verified file.

## 🛠️ Tech Stack

* **Language**: C++17
* **Networking**: Boost.Asio
* **Serialization**: Google Protocol Buffers (Protobuf)
* **Hardware**: Raspberry Pi (ARM) + 1TB External HDD
* **Storage Path**: `/mnt/vault/`

## 📦 Prerequisites

Install the necessary development libraries on your Raspberry Pi:

```bash
sudo apt update
sudo apt install cmake g++ libprotobuf-dev protobuf-compiler libboost-all-dev
```

### Storage Setup
Ensure your external drive is mounted correctly at `/mnt/vault`:

```bash
# Example mount command
sudo mount /dev/sda1 /mnt/vault
sudo chown -R $USER:$USER /mnt/vault
```

## 🏗️ Building the Worker

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/MAX5271/HydraFS.git
    cd HydraFS
    ```

2.  **Generate Protobuf classes**:
    ```bash
    protoc -I=src/proto --cpp_out=src/proto src/proto/hydra.proto
    ```

3.  **Compile the Worker and Stitcher**:
    ```bash
    mkdir build && cd build
    cmake ..
    make
    ```

## 🚦 Running the Vault

Start the storage engine to begin listening for shards:

```bash
./hydra_worker
```

## 🧵 Manual Reassembly

Once all shards (e.g., `.part1`, `.part2`, etc.) have been received in `/mnt/vault/`, use the `stitch` utility to reconstruct the original file:

```bash
# Navigate to project root
g++ src/worker/stitch.cpp -o stitch
./stitch
```

## 📊 Integrity Verification

HydraFS ensures zero-bit corruption. Compare the MD5 hash of the reassembled file against the source file on the client:

```bash
md5sum /mnt/vault/FINAL_FILE.bin
```