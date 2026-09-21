#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <ma2a/contracts.hpp>

namespace ma2a {

namespace detail {

inline void append_utf8(std::string& out, std::uint32_t cp) {
    if (cp <= 0x7f) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7ff) {
        out.push_back(static_cast<char>(0xc0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else if (cp <= 0xffff) {
        out.push_back(static_cast<char>(0xe0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else if (cp <= 0x10ffff) {
        out.push_back(static_cast<char>(0xf0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3f)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3f)));
    } else {
        throw std::invalid_argument("invalid unicode code point");
    }
}

inline int hex_digit(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + (ch - 'a');
    if (ch >= 'A' && ch <= 'F') return 10 + (ch - 'A');
    throw std::invalid_argument("invalid unicode escape");
}

class FlatJsonReader {
public:
    explicit FlatJsonReader(const std::string& json) : json_(json) {}

    void expect(char ch) {
        if (pos_ >= json_.size() || json_[pos_] != ch)
            throw std::invalid_argument("unexpected JSON token");
        ++pos_;
    }

    void key(std::string_view expected) {
        const auto actual = string();
        if (actual != expected) throw std::invalid_argument("unexpected JSON key");
        expect(':');
    }

    std::string string() {
        expect('"');
        std::string out;
        while (pos_ < json_.size()) {
            const char ch = json_[pos_++];
            if (ch == '"') return out;
            if (ch != '\\') {
                if (static_cast<unsigned char>(ch) < 0x20)
                    throw std::invalid_argument("invalid control character");
                out.push_back(ch);
                continue;
            }
            if (pos_ >= json_.size()) throw std::invalid_argument("truncated JSON escape");
            const char esc = json_[pos_++];
            switch (esc) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    const auto first = unicode_unit();
                    std::uint32_t cp = first;
                    if (first >= 0xd800 && first <= 0xdbff) {
                        if (pos_ + 2 > json_.size() || json_[pos_] != '\\' || json_[pos_ + 1] != 'u')
                            throw std::invalid_argument("missing low surrogate");
                        pos_ += 2;
                        const auto second = unicode_unit();
                        if (second < 0xdc00 || second > 0xdfff)
                            throw std::invalid_argument("invalid low surrogate");
                        cp = 0x10000u + ((first - 0xd800u) << 10) + (second - 0xdc00u);
                    } else if (first >= 0xdc00 && first <= 0xdfff) {
                        throw std::invalid_argument("unexpected low surrogate");
                    }
                    append_utf8(out, cp);
                    break;
                }
                default:
                    throw std::invalid_argument("unsupported JSON escape");
            }
        }
        throw std::invalid_argument("unterminated JSON string");
    }

    std::int64_t integer() {
        if (pos_ >= json_.size()) throw std::invalid_argument("missing integer");
        bool negative = false;
        if (json_[pos_] == '-') {
            negative = true;
            ++pos_;
        }
        if (pos_ >= json_.size() || json_[pos_] < '0' || json_[pos_] > '9')
            throw std::invalid_argument("invalid integer");
        std::int64_t value = 0;
        while (pos_ < json_.size() && json_[pos_] >= '0' && json_[pos_] <= '9') {
            const int digit = json_[pos_++] - '0';
            value = value * 10 + digit;
        }
        return negative ? -value : value;
    }

    [[nodiscard]] bool done() const noexcept { return pos_ == json_.size(); }

private:
    std::uint32_t unicode_unit() {
        if (pos_ + 4 > json_.size()) throw std::invalid_argument("truncated unicode escape");
        std::uint32_t value = 0;
        for (int i = 0; i < 4; ++i)
            value = (value << 4) | static_cast<std::uint32_t>(hex_digit(json_[pos_++]));
        return value;
    }

    const std::string& json_;
    std::size_t pos_{0};
};

inline std::pair<std::string, std::string> split_envelope(const std::string& json) {
    constexpr std::string_view prefix = "{\"payload\":";
    if (json.size() <= prefix.size() || json.compare(0, prefix.size(), prefix) != 0)
        throw std::invalid_argument("invalid MA2A envelope");

    const std::size_t payload_start = prefix.size();
    if (json[payload_start] != '{') throw std::invalid_argument("payload must be an object");

    bool in_string = false;
    bool escaped = false;
    int depth = 0;
    std::size_t payload_end = std::string::npos;
    for (std::size_t i = payload_start; i < json.size(); ++i) {
        const char ch = json[i];
        if (in_string) {
            if (escaped) escaped = false;
            else if (ch == '\\') escaped = true;
            else if (ch == '"') in_string = false;
            continue;
        }
        if (ch == '"') {
            in_string = true;
        } else if (ch == '{') {
            ++depth;
        } else if (ch == '}') {
            --depth;
            if (depth == 0) {
                payload_end = i;
                break;
            }
            if (depth < 0) throw std::invalid_argument("invalid envelope nesting");
        }
    }
    if (payload_end == std::string::npos) throw std::invalid_argument("unterminated envelope payload");

    const auto suffix = json.substr(payload_end + 1);
    constexpr std::string_view type_prefix = ",\"type\":\"";
    if (suffix.size() < type_prefix.size() + 2 ||
        suffix.compare(0, type_prefix.size(), type_prefix) != 0 ||
        suffix.back() != '}')
        throw std::invalid_argument("invalid envelope type");

    const auto quote = suffix.find('"', type_prefix.size());
    if (quote == std::string::npos || quote + 2 != suffix.size())
        throw std::invalid_argument("invalid envelope suffix");

    return {
        json.substr(payload_start, payload_end - payload_start + 1),
        suffix.substr(type_prefix.size(), quote - type_prefix.size())
    };
}

} // namespace detail

inline std::string wire_envelope_type(const std::string& json) {
    return detail::split_envelope(json).second;
}

inline JobResult parse_job_result_envelope_json(const std::string& json) {
    auto [payload, type] = detail::split_envelope(json);
    if (type != "JOB_RESULT") throw std::invalid_argument("expected JOB_RESULT envelope");

    detail::FlatJsonReader r(payload);
    JobResult result;
    r.expect('{');
    r.key("completed_at"); result.completed_at = r.integer(); r.expect(',');
    r.key("message_id"); result.message_id = r.string(); r.expect(',');
    r.key("payload"); result.payload = r.string(); r.expect(',');
    r.key("protocol_version"); result.protocol_version = r.string(); r.expect(',');
    r.key("recipient_node_id"); result.recipient_node_id = r.string(); r.expect(',');
    r.key("request_id"); result.request_id = r.string(); r.expect(',');
    r.key("responder_node_id"); result.responder_node_id = r.string(); r.expect(',');
    r.key("signature"); result.signature = r.string(); r.expect(',');
    r.key("signature_algorithm"); result.signature_algorithm = r.string(); r.expect(',');
    r.key("status"); result.status = r.string();
    r.expect('}');
    if (!r.done()) throw std::invalid_argument("trailing JOB_RESULT data");
    return result;
}

inline FailureNotice parse_failure_notice_envelope_json(const std::string& json) {
    auto [payload, type] = detail::split_envelope(json);
    if (type != "FAILURE_NOTICE") throw std::invalid_argument("expected FAILURE_NOTICE envelope");

    detail::FlatJsonReader r(payload);
    FailureNotice notice;
    r.expect('{');
    r.key("failed_node_id"); notice.failed_node_id = r.string(); r.expect(',');
    r.key("message_id"); notice.message_id = r.string(); r.expect(',');
    r.key("observed_at"); notice.observed_at = r.integer(); r.expect(',');
    r.key("protocol_version"); notice.protocol_version = r.string(); r.expect(',');
    r.key("reason"); notice.reason = r.string(); r.expect(',');
    r.key("reporting_node_id"); notice.reporting_node_id = r.string(); r.expect(',');
    r.key("request_id"); notice.request_id = r.string(); r.expect(',');
    r.key("signature"); notice.signature = r.string(); r.expect(',');
    r.key("signature_algorithm"); notice.signature_algorithm = r.string();
    r.expect('}');
    if (!r.done()) throw std::invalid_argument("trailing FAILURE_NOTICE data");
    return notice;
}

} // namespace ma2a
