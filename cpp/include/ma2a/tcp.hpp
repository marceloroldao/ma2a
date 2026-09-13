#pragma once

#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ma2a/framing.hpp>

namespace ma2a {

class SocketHandle {
public:
    explicit SocketHandle(int fd = -1) noexcept : fd_(fd) {}
    ~SocketHandle() { if (fd_ >= 0) ::close(fd_); }
    SocketHandle(const SocketHandle&) = delete;
    SocketHandle& operator=(const SocketHandle&) = delete;
    SocketHandle(SocketHandle&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    SocketHandle& operator=(SocketHandle&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) ::close(fd_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }
    [[nodiscard]] int get() const noexcept { return fd_; }
private:
    int fd_;
};

inline void send_all(int fd, const std::vector<std::uint8_t>& data) {
    std::size_t sent = 0;
    while (sent < data.size()) {
        const auto n = ::send(fd, data.data() + sent, data.size() - sent, 0);
        if (n <= 0) throw std::runtime_error("socket send failed");
        sent += static_cast<std::size_t>(n);
    }
}

inline std::vector<std::uint8_t> recv_exact(int fd, std::size_t size) {
    std::vector<std::uint8_t> out(size);
    std::size_t received = 0;
    while (received < size) {
        const auto n = ::recv(fd, out.data() + received, size - received, 0);
        if (n <= 0) throw std::runtime_error("connection closed before frame completed");
        received += static_cast<std::size_t>(n);
    }
    return out;
}

inline void send_framed_json(int fd, const std::string& json) {
    send_all(fd, encode_frame(json));
}

inline std::string recv_framed_json(int fd) {
    const auto header_raw = recv_exact(fd, 4);
    const std::array<std::uint8_t, 4> header{header_raw[0], header_raw[1], header_raw[2], header_raw[3]};
    const auto body_size = decode_frame_size(header);
    const auto body = recv_exact(fd, body_size);
    return std::string(body.begin(), body.end());
}

inline SocketHandle connect_ipv4(const std::string& host, std::uint16_t port) {
    SocketHandle sock(::socket(AF_INET, SOCK_STREAM, 0));
    if (sock.get() < 0) throw std::runtime_error("socket creation failed");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1)
        throw std::invalid_argument("invalid IPv4 address");
    if (::connect(sock.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        throw std::runtime_error("socket connect failed");
    return sock;
}

} // namespace ma2a
