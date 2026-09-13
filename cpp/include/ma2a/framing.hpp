#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace ma2a {

inline constexpr std::size_t kMaxFrameBytes = 4 * 1024 * 1024;

inline std::vector<std::uint8_t> encode_frame(const std::string& payload) {
    if (payload.size() > kMaxFrameBytes) {
        throw std::length_error("frame too large");
    }
    const auto n = static_cast<std::uint32_t>(payload.size());
    std::vector<std::uint8_t> out;
    out.reserve(4 + payload.size());
    out.push_back(static_cast<std::uint8_t>((n >> 24) & 0xff));
    out.push_back(static_cast<std::uint8_t>((n >> 16) & 0xff));
    out.push_back(static_cast<std::uint8_t>((n >> 8) & 0xff));
    out.push_back(static_cast<std::uint8_t>(n & 0xff));
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

inline std::uint32_t decode_frame_size(const std::array<std::uint8_t, 4>& header) {
    const auto n =
        (static_cast<std::uint32_t>(header[0]) << 24) |
        (static_cast<std::uint32_t>(header[1]) << 16) |
        (static_cast<std::uint32_t>(header[2]) << 8) |
        static_cast<std::uint32_t>(header[3]);
    if (n > kMaxFrameBytes) {
        throw std::length_error("frame too large");
    }
    return n;
}

inline std::string decode_complete_frame(const std::vector<std::uint8_t>& frame) {
    if (frame.size() < 4) {
        throw std::invalid_argument("incomplete frame header");
    }
    const std::array<std::uint8_t, 4> header{frame[0], frame[1], frame[2], frame[3]};
    const auto n = decode_frame_size(header);
    if (frame.size() != 4 + static_cast<std::size_t>(n)) {
        throw std::invalid_argument("frame size mismatch");
    }
    return std::string(frame.begin() + 4, frame.end());
}

} // namespace ma2a
