#include "ctmd/snapshot.hpp"
#include "ctmd/schema.hpp"

namespace {
constexpr auto& s = ctmd::snapshot;
constexpr auto& a = ctmd::accrual;
}

static_assert(s.schema_version == 3);
static_assert(s.firm.region.code[0] == 'E');
static_assert(s.firm.region.desk.cost_centre == 88214);
static_assert(s.firm.region.desk.book.portfolio.mandate[0] == 'r');
static_assert(s.firm.region.desk.book.portfolio.position
               .instrument.maturity_years == 10);
static_assert(s.firm.region.desk.book.portfolio.position
               .instrument.pay_leg.spread_bp == 12);
static_assert(a.notional == 250'000'000);
static_assert(a.fixed_rate > 0.03 && a.fixed_rate < 0.04);
static_assert(a.dv01 > 0.0);

static_assert(ctmd::SnapshotLike<decltype(ctmd::snapshot)>);
static_assert(ctmd::AccrualLike<decltype(ctmd::accrual)>);

int main() { return 0; }
