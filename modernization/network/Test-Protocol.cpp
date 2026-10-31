#include "Protocol.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            printf("FAIL line %d: %s\n", __LINE__, #x);                                            \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)
int main() {
    uint64_t values[] = {1, 0x109000000000001ULL, ~0ULL, 109775244001234567ULL};
    for (auto id : values) {
        uint64_t decoded = 0;
        auto code = dt::Code(id);
        CHECK(dt::Decode(code, decoded) && decoded == id);
        code[5] = (code[5] == '0' ? '1' : '0');
        CHECK(!dt::Decode(code, decoded));
    }
    uint64_t n = 0;
    CHECK(!dt::Decode("DT-ZZZZZZZZZZZZZ-ZZ", n));
    CHECK(!dt::Decode("", n));
    const char body[] = "lockstep command";
    auto bytes = dt::Packet(dt::Data, 42, body, sizeof(body));
    dt::Header h{};
    const uint8_t *payload = nullptr;
    CHECK(dt::Parse(bytes.data(), bytes.size(), 42, h, payload));
    CHECK(!memcmp(body, payload, sizeof(body)));
    for (size_t len = 0; len < bytes.size(); ++len)
        CHECK(!dt::Parse(bytes.data(), len, 42, h, payload));
    CHECK(!dt::Parse(bytes.data(), bytes.size(), 43, h, payload));
    bytes[4]++;
    CHECK(!dt::Parse(bytes.data(), bytes.size(), 42, h, payload));
    dt::Player player{};
    CHECK(dt::ReadPlayer(&player, sizeof(player), player));
    player.size = 0xffffffff;
    CHECK(!dt::ReadPlayer(&player, sizeof(player), player));
    player = {};
    memset(player.name, 'x', sizeof(player.name));
    CHECK(!dt::ReadPlayer(&player, sizeof(player), player));
    puts("PASS: join-code round trips/checksum/overflow and packet length/version/session/player "
         "bounds");
    return 0;
}
