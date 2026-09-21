#pragma once

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/time.h>
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
        const auto n = ::send(fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n > 0) {
            sent += static_cast<std::size_t>(n);
            continue;
        }
        if (n < 0 && errno == EINTR) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
            throw std::runtime_error("socket send timeout");
        throw std::runtime_error("socket send failed");
    }
}

inline std::vector<std::uint8_t> recv_exact(int fd, std::size_t size) {
    std::vector<std::uint8_t> out(size);
    std::size_t received = 0;
    while (received < size) {
        const auto n = ::recv(fd, out.data() + received, size - received, 0);
        if (n > 0) {
            received += static_cast<std::size_t>(n);
            continue;
        }
        if (n == 0) throw std::runtime_error("connection closed before frame completed");
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            throw std::runtime_error("socket receive timeout");
        throw std::runtime_error("socket receive failed");
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

inline void set_socket_io_timeout(int fd, std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0) throw std::invalid_argument("socket timeout must be positive");
    const auto total_ms = timeout.count();
    timeval tv{};
    tv.tv_sec = static_cast<time_t>(total_ms / 1000);
    tv.tv_usec = static_cast<suseconds_t>((total_ms % 1000) * 1000);
    if (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
        throw std::runtime_error("failed to set socket receive timeout");
    if (::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) != 0)
        throw std::runtime_error("failed to set socket send timeout");
}

inline sockaddr_in ipv4_address(const std::string& host, std::uint16_t port) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &address.sin_addr) != 1)
        throw std::invalid_argument("invalid IPv4 address");
    return address;
}

inline SocketHandle connect_ipv4(const std::string& host, std::uint16_t port) {
    SocketHandle sock(::socket(AF_INET, SOCK_STREAM, 0));
    if (sock.get() < 0) throw std::runtime_error("socket creation failed");
    auto address = ipv4_address(host, port);
    if (::connect(sock.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0)
        throw std::runtime_error("socket connect failed");
    return sock;
}

inline SocketHandle connect_ipv4_with_timeout(
    const std::string& host,
    std::uint16_t port,
    std::chrono::milliseconds timeout) {
    if (timeout.count() <= 0) throw std::invalid_argument("connect timeout must be positive");

    SocketHandle sock(::socket(AF_INET, SOCK_STREAM, 0));
    if (sock.get() < 0) throw std::runtime_error("socket creation failed");

    const int flags = ::fcntl(sock.get(), F_GETFL, 0);
    if (flags < 0) throw std::runtime_error("failed to read socket flags");
    if (::fcntl(sock.get(), F_SETFL, flags | O_NONBLOCK) != 0)
        throw std::runtime_error("failed to enable nonblocking connect");

    auto address = ipv4_address(host, port);
    const int connect_result =
        ::connect(sock.get(), reinterpret_cast<sockaddr*>(&address), sizeof(address));

    if (connect_result != 0) {
        if (errno != EINPROGRESS)
            throw std::runtime_error("socket connect failed");

        pollfd descriptor{};
        descriptor.fd = sock.get();
        descriptor.events = POLLOUT;

        int poll_result;
        do {
            poll_result = ::poll(&descriptor, 1, static_cast<int>(timeout.count()));
        } while (poll_result < 0 && errno == EINTR);

        if (poll_result == 0) throw std::runtime_error("socket connect timeout");
        if (poll_result < 0) throw std::runtime_error("socket connect poll failed");

        int socket_error = 0;
        socklen_t error_len = sizeof(socket_error);
        if (::getsockopt(sock.get(), SOL_SOCKET, SO_ERROR, &socket_error, &error_len) != 0)
            throw std::runtime_error("failed to read socket connect status");
        if (socket_error != 0)
            throw std::runtime_error("socket connect failed");
    }

    if (::fcntl(sock.get(), F_SETFL, flags) != 0)
        throw std::runtime_error("failed to restore socket flags");

    return sock;
}

} // namespace ma2a
