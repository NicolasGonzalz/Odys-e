#include "odyssee/sim/ProvisionalGround.hpp"

#include "odyssee/net/Quantize.hpp"

namespace odyssee::sim {

std::int32_t provisionalGroundHeight() {
    return net::quantizePosition(kProvisionalGroundMeters);
}

} // namespace odyssee::sim
