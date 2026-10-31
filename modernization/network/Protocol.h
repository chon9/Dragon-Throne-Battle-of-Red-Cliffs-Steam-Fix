#pragma once
#include <cstring>
#include <limits>
#include <stdint.h>
#include <string>
#include <vector>

namespace dt {
constexpr uint32_t Magic = 0x314e5444; // DTN1
constexpr uint16_t Version = 1;
constexpr size_t MaxData = 65536;
constexpr size_t MaxPlayerData = 16384;
constexpr unsigned MaxPlayers = 8;
constexpr int Channel = 27;
enum Kind : uint16_t {
    Create = 1,
    PlayerInfo,
    Ready,
    RemoveRequest,
    Removed,
    Data,
    Session,
    UpdatePlayer
};
#pragma pack(push, 1)
struct Header {
    uint32_t magic;
    uint16_t version, kind;
    uint32_t size;
    uint64_t lobby;
};
struct Player {
    uint64_t owner;
    uint32_t id, flags, size;
    char name[20], longName[52];
};
struct DataHeader {
    uint32_t from, to;
};
struct SessionInfo {
    uint32_t flags, maxPlayers, currentPlayers;
    uint8_t application[16];
    uint32_t user[4];
    char name[64];
};
#pragma pack(pop)
inline std::vector<uint8_t> Packet(Kind kind, uint64_t lobby, const void *data, size_t size) {
    Header h{Magic, Version, kind, static_cast<uint32_t>(size), lobby};
    std::vector<uint8_t> out(sizeof(h) + size);
    memcpy(out.data(), &h, sizeof(h));
    if (size)
        memcpy(out.data() + sizeof(h), data, size);
    return out;
}
inline bool Parse(const void *data, size_t size, uint64_t lobby, Header &h,
                  const uint8_t *&payload) {
    if (!data || size < sizeof(h) || size > sizeof(h) + MaxData + sizeof(DataHeader))
        return false;
    memcpy(&h, data, sizeof(h));
    if (h.magic != Magic || h.version != Version || h.lobby != lobby ||
        h.size != size - sizeof(h) || h.kind < Create || h.kind > UpdatePlayer)
        return false;
    payload = static_cast<const uint8_t *>(data) + sizeof(h);
    return true;
}
inline bool ReadPlayer(const void *data, size_t size, Player &p) {
    if (size < sizeof(p))
        return false;
    memcpy(&p, data, sizeof(p));
    return p.size <= MaxPlayerData && size == sizeof(p) + p.size &&
           memchr(p.name, 0, sizeof(p.name)) && memchr(p.longName, 0, sizeof(p.longName));
}
inline std::string Hex(const void *ptr, size_t n) {
    const char *digits = "0123456789abcdef";
    const auto *p = static_cast<const uint8_t *>(ptr);
    std::string s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) {
        s += digits[p[i] >> 4];
        s += digits[p[i] & 15];
    }
    return s;
}
inline bool Unhex(const char *s, void *dst, size_t n) {
    if (!s || strlen(s) != n * 2)
        return false;
    auto *p = static_cast<uint8_t *>(dst);
    for (size_t i = 0; i < n; ++i) {
        unsigned v = 0;
        for (unsigned j = 0; j < 2; ++j) {
            char c = s[2 * i + j];
            if (c >= '0' && c <= '9')
                v = v * 16 + c - '0';
            else if (c >= 'a' && c <= 'f')
                v = v * 16 + c - 'a' + 10;
            else
                return false;
        }
        p[i] = static_cast<uint8_t>(v);
    }
    return true;
}
// Fixed-width base32 lobby ID plus checksum catches transcription mistakes.
// The code grants access to an unlisted lobby; it is not a password hash.
inline uint16_t Check(uint64_t n) {
    uint16_t c = 0x3a5;
    for (int i = 0; i < 8; ++i) {
        c = static_cast<uint16_t>((c * 33) ^ (n & 255));
        n >>= 8;
    }
    return c & 1023;
}
inline std::string Code(uint64_t id) {
    const char *a = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    std::string s = "DT-";
    uint64_t n = id;
    char b[13];
    for (int i = 12; i >= 0; --i) {
        b[i] = a[n & 31];
        n >>= 5;
    }
    s.append(b, 13);
    s += '-';
    auto c = Check(id);
    s += a[c >> 5];
    s += a[c & 31];
    return s;
}
inline bool Decode(std::string text, uint64_t &id) {
    std::string s;
    for (char c : text) {
        if (c == '-' || c == ' ' || c == '\r' || c == '\n' || c == '\t')
            continue;
        if (c >= 'a' && c <= 'z')
            c -= 32;
        s += c;
    }
    if (s.size() != 17 || s.substr(0, 2) != "DT")
        return false;
    const char *a = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    uint64_t n = 0;
    unsigned check = 0;
    for (size_t i = 2; i < 17; ++i) {
        const char *p = strchr(a, s[i]);
        if (!p)
            return false;
        unsigned v = static_cast<unsigned>(p - a);
        if (i < 15) {
            if (n > (std::numeric_limits<uint64_t>::max() >> 5))
                return false;
            n = n * 32 + v;
        } else
            check = check * 32 + v;
    }
    if (!n || Check(n) != check)
        return false;
    id = n;
    return true;
}
} // namespace dt
