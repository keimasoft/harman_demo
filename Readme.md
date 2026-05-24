# HARMAN Technical Assignment - Real-Time UDP Video Pipeline

A distributed client-server video processing application written in C using GStreamer. The client captures video from a webcam, applies transformations (scaling to 640x480, vertical flipping, and color inversion) using native plugins, encodes it to H264, and streams it via RTP over UDP. The server reads its pipeline layout dynamically from an `.ini` file and displays the stream.

## Prerequisites (Ubuntu)

Install the required development build tools and GStreamer framework plugins:

```bash
sudo apt update
sudo apt install -y build-essential pkg-config libgstreamer1.0-dev \
libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-good \
gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
gstreamer1.0-libav gstreamer1.0-tools v4l-utils graphviz frei0r-plugins
```

Verify your webcam device node path before running the code (defaults to `/dev/video0`):
```bash
v4l2-ctl --list-devices
```

## How to Build

Compile both targets using the provided Makefile:
```bash
make
```

To clear the binaries and build artifacts:
```bash
make clean
```

## How to Run

1. **Start the Server first** (reads configuration dynamically from `cfg/server.ini`):
   ```bash
   ./build/server
   ```

2. **Start the Client** in a separate terminal (provide server IP and port as CLI arguments):
   ```bash
   ./build/client 127.0.0.1 5000
   ```

---

## Troubleshooting & Debugging

When debugging a distributed streaming application, issues must be isolated between the GStreamer pipeline states and the network transport layer.

### 1. GStreamer Log Level Inspecting
If the pipeline fails to transition to the `PLAYING` state or throws internal errors, use the `GST_DEBUG` environment variable to inspect runtime warnings:
```bash
# Run server with warning logs enabled (Level 3 captures errors and negotiation issues)
GST_DEBUG=3 ./build/server

# Target specific elements for deeper log analysis (e.g., inspecting udpsrc at debug level 5)
GST_DEBUG=udpsrc:5,x264enc:4 ./server
```

### 2. Pipeline Architecture Graph Dumps (.dot files)
To visually inspect pad capabilities (Caps Negotiation) and link behaviors inside memory, you can dump pipeline architecture diagrams:
```bash
# Execute the binary while triggering dot file generation on state changes
GST_DEBUG_DUMP_DOT_DIR=/tmp ./client 127.0.0.1 5000

# Convert the generated dot file to a viewable PNG format using graphviz
dot -Tpng /tmp/0.00.00.*-client-pipeline.dot -o client_pipeline.png
```

### 3. Network Transport Validation (Layer 4 / Layer 7)
If the server window opens but remains black, verify whether network packets are actually being routed and arriving at the target port:
```bash
# Sniff incoming UDP packets on port 5000 (use 'lo' for localhost, or eth0/wlan0 for remote hosts)
sudo tcpdump -i lo udp port 5000 -X
```
*Alternatively, capture the traffic into a `.pcap` file and use Wireshark's **Telephony -> RTP -> RTP Streams** analyzer to evaluate network jitter, packet loss, or out-of-order execution sequence numbers.*

---

## Architectural & Automotive Networking Roadmap (AVB)

To ensure high portability and seamless testing on standard host OS/laptop development setups, this implementation utilizes **RTP over UDP (Layer 4/Transport)**. This choice guarantees that the application runs out-of-the-box in generic IP network environments.

However, in modern production Automotive Ethernet architectures (such as HARMAN Infotainment or ADAS platforms), utilizing the full TCP/IP stack introduces unwanted kernel overhead, jitter, and non-deterministic latencies. 

### Future Transition to AVB/TSN (Layer 2)
The application design is deliberately modular to support a direct transition to **AVB (Audio Video Bridging - IEEE 1722)**:
- **No IP Stack**: AVB operates directly at **Layer 2 (Data Link)** using MAC addresses, completely bypassing the OS routing table and socket layers to reduce CPU usage and latency near zero.
- **GStreamer Modular Swap**: Because the processing steps are decoupled from the transport mechanisms, `rtph264pay` and `udpsink` on the client can be swapped instantly with AVB native plugins (`avtph264pay` and `avtpsink` via `libavtp`), without refactoring any of the underlying video handling logic.
- **Deterministic Delivery**: In a true automotive target environment (e.g., NXP i.MX8 or Qualcomm platforms), AVB guarantees bounded, deterministic latency for the safety-critical video feed.
