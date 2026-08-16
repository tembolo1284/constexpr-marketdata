#include "ctmd/snapshot.hpp"
#include "ctmd/schema.hpp"

#include <cstdint>
#include <cstdio>

static_assert(ctmd::SnapshotLike<decltype(ctmd::snapshot)>,
              "market_snapshot.json does not match the expected schema");

double pv_fixed_leg() {
    return ctmd::accrual.notional * ctmd::accrual.fixed_rate
         * ctmd::accrual.accrual_factor * ctmd::accrual.discount_factor;
}

std::int64_t notional_mm() { return ctmd::accrual.notional / 1'000'000; }

int main() {
    std::printf("schema v%lld  notional %lld MM  PV %.2f  DV01 %.2f\n",
                (long long)ctmd::snapshot.schema_version,
                (long long)notional_mm(),
                pv_fixed_leg(),
                ctmd::accrual.dv01);
}
