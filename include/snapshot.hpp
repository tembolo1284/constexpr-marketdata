#ifndef CTMD_SNAPSHOT_HPP
#define CTMD_SNAPSHOT_HPP

#define SIMDJSON_STATIC_REFLECTION 1
#include "simdjson.h"

#ifndef CTMD_SNAPSHOT_PATH
#define CTMD_SNAPSHOT_PATH "market_snapshot.json"
#endif

namespace ctmd {

inline constexpr const char snapshot_bytes[] = {
#embed CTMD_SNAPSHOT_PATH
    , 0
};

inline constexpr auto snapshot =
    simdjson::compile_time::parse_json<snapshot_bytes>();

// Leaf shorthand — still a compile-time constant, not a runtime dereference.
inline constexpr auto accrual =
    snapshot.firm.region.desk.book.portfolio.position.instrument.pay_leg.accrual;

} // namespace ctmd

#endif // CTMD_SNAPSHOT_HPP
