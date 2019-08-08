# UDP reliable transport protocol

## Project introduction:

Based on the connection-oriented, reliable and byte throttle-based UDP transmission layer communication protocol, it abandons the redundant function of TCP when rapidly transmitting short data packets, and achieves better results than TCP communication, and is superior to TCP in terms of real-time data, transmission efficiency and bandwidth utilization.

## Function realization:

Adopt the sliding window of GBN protocol to ensure the orderly transmission of data frames.

A request response mechanism with timeout is implemented, and the business layer is responsible for timeout retransmission to ensure the reliability of data transmission under certain limits.

Add custom headers, use status codes to split packets and simulate handshake connections.

Use non-blocking I/O to prevent performance degradation caused by network congestion, matching UDP short packets with high concurrency transport advantage.

Complete the test of server and client, and visualize the transmission process in the case of given packet loss rate.
