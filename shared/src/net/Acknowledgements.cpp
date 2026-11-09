#include "odyssee/net/Acknowledgements.hpp"

namespace odyssee::net {

std::vector<Sequence> acknowledgedSequences(Sequence ack, std::uint32_t ackBits) {
    std::vector<Sequence> acknowledged;
    acknowledged.reserve(33);
    acknowledged.push_back(ack);

    for (std::uint16_t i = 0; i < 32; ++i) {
        if ((ackBits & (1u << i)) != 0) {
            // Bit i = paquet (ack - 1 - i) ; le cast gère le rebouclage (0 - 1 = 65535).
            acknowledged.push_back(static_cast<Sequence>(ack - 1 - i));
        }
    }
    return acknowledged;
}

} // namespace odyssee::net
