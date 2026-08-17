#include "ctmd/snapshot.hpp"
#include "ctmd/schema.hpp"

#include <cstdint>
#include <print>
#include <string>
#include <string_view>

static_assert(ctmd::SnapshotLike<decltype(ctmd::snapshot)>,
              "market_snapshot.json does not match the expected schema");

namespace {

// Synthesised string fields: normalise whatever reflection produced.
template <typename S>
constexpr std::string_view sv(const S& s) {
    std::string_view out;
    if constexpr (requires { std::string_view(s); }) {
        out = std::string_view(s);
    } else {
        out = std::string_view(s.data(), s.size());
    }
    while (!out.empty() && out.back() == '\0') out.remove_suffix(1);
    return out;
}

std::string group(std::string digits) {
    std::size_t dot = digits.find('.');
    std::size_t end = (dot == std::string::npos) ? digits.size() : dot;
    std::size_t start = (!digits.empty() && digits[0] == '-') ? 1 : 0;
    for (std::size_t i = end; i > start + 3; ) {
        i -= 3;
        digits.insert(i, ",");
    }
    return digits;
}

std::string num(std::int64_t v)      { return group(std::to_string(v)); }
std::string num(double v, int dp = 2) { return group(std::format("{:.{}f}", v, dp)); }

constexpr auto& S = ctmd::snapshot;
constexpr auto& A = ctmd::accrual;

void row(int layer, std::string_view node, std::string_view field, std::string_view value) {
    std::println("  {:>3}   {:<12}  {:<17}  {}", layer, node, field, value);
}

void rule(std::string_view label = {}) {
    if (!label.empty()) std::println("\n  {}", label);
    std::println("  {:-<3}   {:-<12}  {:-<17}  {:-<26}", "", "", "", "");
}

} // namespace

// Derived quantities — all of these fold to constants at -O3.
double pv_fixed_leg() {
    return A.notional * A.fixed_rate * A.accrual_factor * A.discount_factor;
}
double coupon_amount()  { return A.notional * A.fixed_rate * A.accrual_factor; }
double dv01_per_mm()    { return A.dv01 / (A.notional / 1'000'000.0); }

int main() {
    std::println("\n  constexpr-marketdata  ·  {} bytes embedded, 10 levels deep, 0 parsed at runtime",
                 sizeof(ctmd::snapshot_bytes) - 1);

    rule("DOCUMENT");
    row( 1, "root",       "schema_version", num((std::int64_t)S.schema_version));
    row( 1, "root",       "as_of",          sv(S.as_of));
    row( 1, "root",       "reporting_ccy",  sv(S.reporting_ccy));
    row( 2, "firm",       "lei",            sv(S.firm.lei));
    row( 3, "region",     "code",           sv(S.firm.region.code));
    row( 3, "region",     "clearing_house", sv(S.firm.region.clearing_house));
    row( 4, "desk",       "name",           sv(S.firm.region.desk.name));
    row( 4, "desk",       "cost_centre",    num((std::int64_t)S.firm.region.desk.cost_centre));

    constexpr auto& B = S.firm.region.desk.book;
    row( 5, "book",       "id",             sv(B.id));
    row( 5, "book",       "base_ccy",       sv(B.base_ccy));
    row( 6, "portfolio",  "id",             sv(B.portfolio.id));
    row( 6, "portfolio",  "mandate",        sv(B.portfolio.mandate));

    constexpr auto& P = B.portfolio.position;
    row( 7, "position",   "trade_id",       sv(P.trade_id));
    row( 7, "position",   "direction",      sv(P.direction));
    row( 8, "instrument", "type",           sv(P.instrument.type));
    row( 8, "instrument", "maturity_years", num((std::int64_t)P.instrument.maturity_years));
    row( 9, "pay_leg",    "index",          sv(P.instrument.pay_leg.index));
    row( 9, "pay_leg",    "spread_bp",      num((std::int64_t)P.instrument.pay_leg.spread_bp));
    row(10, "accrual",    "day_count",      sv(A.day_count));
    row(10, "accrual",    "notional",       num((std::int64_t)A.notional) + " " + std::string(sv(B.base_ccy)));
    row(10, "accrual",    "fixed_rate",     num(A.fixed_rate * 100.0, 4) + " %");
    row(10, "accrual",    "accrual_factor", num(A.accrual_factor, 8));
    row(10, "accrual",    "discount_factor",num(A.discount_factor, 6));
    row(10, "accrual",    "dv01",           num(A.dv01));

    rule("DERIVED  (folded to immediates at -O3)");
    row(0, "", "coupon",        num(coupon_amount())    + " " + std::string(sv(B.base_ccy)));
    row(0, "", "pv_fixed_leg",  num(pv_fixed_leg())     + " " + std::string(sv(B.base_ccy)));
    row(0, "", "dv01_per_mm",   num(dv01_per_mm())      + " per MM");
    std::println("");
    return 0;
}
