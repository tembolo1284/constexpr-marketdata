#ifndef CTMD_SCHEMA_HPP
#define CTMD_SCHEMA_HPP

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace ctmd {

template <typename T> using bare_t = std::remove_cvref_t<T>;

template <typename T>
concept AccrualLike = requires(const T& t) {
    { t.notional }        -> std::convertible_to<std::int64_t>;
    { t.fixed_rate }      -> std::convertible_to<double>;
    { t.accrual_factor }  -> std::convertible_to<double>;
    { t.discount_factor } -> std::convertible_to<double>;
    { t.dv01 }            -> std::convertible_to<double>;
} && std::is_integral_v<bare_t<decltype(T::notional)>>;

template <typename T>
using accrual_of = bare_t<decltype(std::declval<const T&>().firm.region.desk.book.portfolio.position.instrument.pay_leg.accrual)>;

template <typename T>
concept SnapshotLike = requires(const T& t) {
    { t.schema_version } -> std::convertible_to<std::int64_t>;
} && AccrualLike<accrual_of<T>>;

} // namespace ctmd

#endif // CTMD_SCHEMA_HPP
