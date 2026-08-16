#ifndef CTMD_SCHEMA_HPP
#define CTMD_SCHEMA_HPP

#include <concepts>
#include <cstdint>
#include <type_traits>

namespace ctmd {

template <typename T>
concept AccrualLike = requires(T t) {
    { t.notional }        -> std::convertible_to<std::int64_t>;
    { t.fixed_rate }      -> std::convertible_to<double>;
    { t.accrual_factor }  -> std::convertible_to<double>;
    { t.discount_factor } -> std::convertible_to<double>;
    { t.dv01 }            -> std::convertible_to<double>;
} && std::is_integral_v<decltype(T::notional)>;

template <typename T>
concept SnapshotLike = requires(T t) {
    { t.schema_version } -> std::convertible_to<std::int64_t>;
    requires AccrualLike
        decltype(t.firm.region.desk.book.portfolio
                  .position.instrument.pay_leg.accrual)>;
};

} // namespace ctmd

#endif // CTMD_SCHEMA_HPP
