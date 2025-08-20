#pragma once

#include <fmt/core.h>
#include <outcome.hpp>

#include <source_location>

#define TRY(...) OUTCOME_TRY(__VA_ARGS__)
#define TRYV(...) OUTCOME_TRYV(__VA_ARGS__)

#if defined(__GNUC__) || defined(__clang__)
#define TRYX(...) OUTCOME_TRYX(__VA_ARGS__)
#endif

namespace outcome = OUTCOME_V2_NAMESPACE::experimental;

template <typename R,
          typename S = outcome::erased_errored_status_code<
              typename outcome::system_code::value_type>,
          typename NoValuePolicy =
              outcome::policy::default_status_result_policy<R, S>>
using Result = outcome::status_result<R, S, NoValuePolicy>;

template <typename R,
          typename S = outcome::erased_errored_status_code<
              typename outcome::system_code::value_type>,
          typename P = std::exception_ptr,
          typename NoValuePolicy =
              outcome::policy::default_status_outcome_policy<R, S, P>>
using Outcome = outcome::status_outcome<R, S, P, NoValuePolicy>;

template <>
struct fmt::formatter<SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref>
    : fmt::formatter<std::string_view> {
  using T = SYSTEM_ERROR2_NAMESPACE::status_code_domain::string_ref;
  template <typename FormatContext>
  auto format(const T &c, FormatContext &ctx) const -> decltype(ctx.out()) {
    auto s = std::string_view{c.data(), c.size()};
    return fmt::formatter<std::string_view>::format(s, ctx);
  }
};

template <typename T>
  requires outcome::is_status_code<T>::value ||
           outcome::is_errored_status_code<T>::value
struct fmt::formatter<T> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const T &c, FormatContext &ctx) const -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "ErrorDomain={} {}", c.domain().name(),
                          c.message());
  }
};

template <>
struct fmt::formatter<std::source_location> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const std::source_location &location, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    std::string_view v{location.file_name()};
    return fmt::format_to(ctx.out(), "{}:{}", v.substr(v.find_last_of('/') + 1),
                          location.line());
  }
};

template <>
struct fmt::formatter<std::error_code> : fmt::formatter<std::string_view> {
  template <typename FormatContext>
  auto format(const std::error_code &ec, FormatContext &ctx) const
      -> decltype(ctx.out()) {
    return fmt::format_to(ctx.out(), "{}", ec.message());
  }
};

SYSTEM_ERROR2_NAMESPACE_BEGIN

namespace detail {
using string_ref = status_code_domain::string_ref;
using atomic_refcounted_string_ref =
    status_code_domain::atomic_refcounted_string_ref;

string_ref to_string_ref(const std::string &s) {
  auto p = (char *)malloc(s.size());   // NOLINT
  std::memcpy(p, s.data(), s.size());  // NOLINT
  return atomic_refcounted_string_ref{p, s.size()};
}
}  // namespace detail

template <typename T>
concept AnyhowValue =
    std::is_nothrow_move_constructible_v<T> && fmt::is_formattable<T>::value;

template <typename T>
  requires AnyhowValue<T>
struct AnyhowDomainImpl final : public status_code_domain {
  using Base = status_code_domain;

  struct value_type {
    T value;
    std::source_location location;
  };

  // 0xf1b0a9ec56db8239 ^ 0xc44f7bdeb2cc50e9 = 0x35ffd232e417d2d0
  constexpr AnyhowDomainImpl() : Base(0xf1b0a9ec56db8239) {}
  AnyhowDomainImpl(const AnyhowDomainImpl &) = default;
  AnyhowDomainImpl(AnyhowDomainImpl &&) = default;
  AnyhowDomainImpl &operator=(const AnyhowDomainImpl &) = default;
  AnyhowDomainImpl &operator=(AnyhowDomainImpl &&) = default;
  ~AnyhowDomainImpl() = default;

  string_ref name() const noexcept final {
    return string_ref("Anyhow");
  }

  payload_info_t payload_info() const noexcept final {
    return {sizeof(value_type),
            sizeof(status_code_domain *) + sizeof(value_type),
            (alignof(value_type) > alignof(status_code_domain *))
                ? alignof(value_type)
                : alignof(status_code_domain *)};
  }

  static constexpr const AnyhowDomainImpl &get();

  bool _do_failure(const status_code<void> &code) const noexcept final {
    assert(code.domain() == *this);
    (void)code;
    if constexpr (std::is_same_v<T, std::error_code>) {
      return static_cast<const status_code<AnyhowDomainImpl<T>> &>(code)
          .value()
          .value;
    }
    return true;
  }

  bool _do_equivalent(
      const status_code<void> &code1,
      const status_code<void> & /*code2*/) const noexcept final {
    assert(code1.domain() == *this);
    (void)code1;
    return false;
  }

  generic_code _generic_code(
      const status_code<void> &code) const noexcept final {
    assert(code.domain() == *this);
    (void)code;
    return errc::unknown;
  }

  string_ref _do_message(const status_code<void> &code) const noexcept final {
    assert(code.domain() == *this);
    using Self = status_code<AnyhowDomainImpl<T>>;
    const auto &value = static_cast<const Self &>(code).value();  // NOLINT
    return detail::to_string_ref(
        fmt::format("{} {}", value.location, value.value));
  }

  void _do_throw_exception(const status_code<void> &code) const final {
    assert(code.domain() == *this);
    using Self = status_code<AnyhowDomainImpl>;
    const auto &c = static_cast<const Self &>(code);  // NOLINT
    throw status_error<AnyhowDomainImpl>(c);
  }
};

template <typename T>
  requires AnyhowValue<T>
constexpr const AnyhowDomainImpl<T> AnyhowDomain = {};

template <typename T>
  requires AnyhowValue<T>
constexpr const AnyhowDomainImpl<T> &AnyhowDomainImpl<T>::get() {
  return AnyhowDomain<T>;
}

template <typename T>
  requires AnyhowValue<T>
inline system_code make_status_code(status_code<AnyhowDomainImpl<T>> value) {
  return make_nested_status_code(std::move(value));
}

template <typename Enum, typename = void>
struct HasQuickEnum : std::false_type {};

template <typename Enum>
struct HasQuickEnum<Enum, std::void_t<quick_status_code_from_enum<Enum>>>
    : std::true_type {};

template <typename Enum, typename Payload>
concept EnumPayload = std::is_enum_v<Enum> && HasQuickEnum<Enum>::value &&
                      requires {
                        { quick_status_code_from_enum<Enum>::payload_uuid };
                        {
                          quick_status_code_from_enum<Enum>::all_failures
                        } -> std::convertible_to<bool>;
                      } && fmt::is_formattable<Payload>::value &&
                      std::is_nothrow_move_constructible_v<Payload>;

template <typename Enum, typename Payload = void>
struct EnumPayloadDomainImpl;

template <typename Enum, typename Payload = void>
using EnumPayloadError = status_code<EnumPayloadDomainImpl<Enum, Payload>>;

template <typename Enum>
  requires std::is_enum_v<Enum> && HasQuickEnum<Enum>::value
struct EnumValueBase {
  using QuickEnum = quick_status_code_from_enum<Enum>;
  using QuickEnumMapping = typename QuickEnum::mapping;

  static const QuickEnumMapping *find_mapping(Enum v) {
    for (const auto &i : QuickEnum::value_mappings()) {
      if (i.value == v) {
        return &i;
      }
    }
    return nullptr;
  }
};

template <typename Enum, typename Payload = void>
struct EnumValue : EnumValueBase<Enum> {
  struct value_type {
    Enum value{};
    std::source_location loc = std::source_location::current();
  };

  static std::string to_string(const value_type &v) {
    auto m = EnumValueBase<Enum>::find_mapping(v.value);
    assert(m);
    return fmt::format("{} {}", v.loc, m->message);
  }
};

template <typename Enum, typename Payload>
  requires EnumPayload<Enum, Payload>
struct EnumValue<Enum, Payload> : EnumValueBase<Enum> {
  struct value_type {
    Enum value{};
    Payload payload{};
    std::source_location loc = std::source_location::current();
  };
  static std::string to_string(const value_type &v) {
    auto m = EnumValueBase<Enum>::find_mapping(v.value);
    assert(m);
    return fmt::format("{} {} {}", v.loc, m->message, v.payload);
  }
};

template <typename Enum, typename Payload>
struct EnumPayloadDomainImpl final : public status_code_domain {
  using Base = status_code_domain;
  using QuickEnum = quick_status_code_from_enum<Enum>;
  using EnumPayloadErrorSelf = EnumPayloadError<Enum, Payload>;

  static constexpr size_t uuid_size = detail::cstrlen(QuickEnum::payload_uuid);
  static constexpr uint64_t payload_uuid =
      detail::parse_uuid_from_pointer<uuid_size>(QuickEnum::payload_uuid);

  constexpr EnumPayloadDomainImpl() : Base(payload_uuid) {}
  EnumPayloadDomainImpl(const EnumPayloadDomainImpl &) = default;
  EnumPayloadDomainImpl(EnumPayloadDomainImpl &&) = default;
  EnumPayloadDomainImpl &operator=(const EnumPayloadDomainImpl &) = default;
  EnumPayloadDomainImpl &operator=(EnumPayloadDomainImpl &&) = default;
  ~EnumPayloadDomainImpl() = default;

  using EnumValueType = EnumValue<Enum, Payload>;
  using value_type = typename EnumValueType::value_type;

  string_ref name() const noexcept final {
    return string_ref{QuickEnum::domain_name};
  }

  payload_info_t payload_info() const noexcept final {
    return {sizeof(value_type),
            sizeof(status_code_domain *) + sizeof(value_type),
            (alignof(value_type) > alignof(status_code_domain *))
                ? alignof(value_type)
                : alignof(status_code_domain *)};
  }

  static constexpr const EnumPayloadDomainImpl &get();

  bool _do_failure(const status_code<void> &code) const noexcept final {
    assert(code.domain() == *this);
    if constexpr (QuickEnum::all_failures) {
      return true;
    } else {
      const auto &c = static_cast<const EnumPayloadErrorSelf &>(code);
      const auto *_mapping = EnumValueType::find_mapping(c.value().value);
      assert(_mapping != nullptr);
      for (auto ec : _mapping->code_mappings) {
        if (ec == errc::success) {
          return false;
        }
      }
      return true;
    }
  }

  bool _do_equivalent(const status_code<void> &code1,
                      const status_code<void> &code2) const noexcept final {
    assert(code1.domain() == *this);
    auto code1_id = code1.domain().id();
    auto code2_id = code2.domain().id();

    const auto &c1 = static_cast<const EnumPayloadErrorSelf &>(code1);
    if (code2.domain() == *this) {
      const auto &c2 = static_cast<const EnumPayloadErrorSelf &>(code2);
      return c1.value().value == c2.value().value;
    }

    assert(code1_id == payload_uuid);
    if (code1_id == (code2_id ^ 0xc44f7bdeb2cc50e9)) {
      using IndirectCode = status_code<detail::indirecting_domain<
          EnumPayloadErrorSelf, std::allocator<EnumPayloadErrorSelf>>>;
      const auto &c2 = static_cast<const IndirectCode &>(code2);
      return c1.value().value == c2.value()->sc.value().value;
    }

    if (code2.domain() == quick_status_code_from_enum_domain<Enum>) {
      using QuickCode = quick_status_code_from_enum_code<Enum>;
      const auto &c2 = static_cast<const QuickCode &>(code2);
      return c1.value().value == c2.value();
    }

    if (code2.domain() == generic_code_domain) {
      const auto &c2 = static_cast<const generic_code &>(code2);  // NOLINT
      const auto *_mapping = EnumValueType::find_mapping(c1.value().value);
      assert(_mapping != nullptr);
      for (auto ec : _mapping->code_mappings) {
        if (ec == c2.value()) {
          return true;
        }
      }
    }
    return false;
  }

  generic_code _generic_code(
      const status_code<void> &code) const noexcept final {
    assert(code.domain() == *this);
    auto value = static_cast<const EnumPayloadErrorSelf &>(code).value().value;
    const auto *_mapping = EnumValueType::find_mapping(value);
    assert(_mapping != nullptr);
    if (_mapping->code_mappings.size() > 0) {
      return *_mapping->code_mappings.begin();
    }
    return errc::unknown;
  }

  string_ref _do_message(const status_code<void> &code) const noexcept final {
    assert(code.domain() == *this);
    const auto &c = static_cast<const EnumPayloadErrorSelf &>(code);
    return detail::to_string_ref(EnumValueType::to_string(c.value()));
  }

  void _do_throw_exception(const status_code<void> &code) const final;
};

template <typename Enum, typename Payload>
constexpr EnumPayloadDomainImpl<Enum, Payload> EnumPayloadDomain = {};

template <typename Enum, typename Payload>
constexpr const EnumPayloadDomainImpl<Enum, Payload> &
EnumPayloadDomainImpl<Enum, Payload>::get() {
  return EnumPayloadDomain<Enum, Payload>;
}

template <typename Enum, typename Payload>
inline system_code make_status_code(EnumPayloadError<Enum, Payload> e) {
  return make_nested_status_code(std::move(e));
}

template <typename Enum, typename Payload>
void EnumPayloadDomainImpl<Enum, Payload>::_do_throw_exception(
    const status_code<void> &code) const {
  assert(code.domain() == *this);
  const auto &c = static_cast<const EnumPayloadErrorSelf &>(code);
  throw status_error<EnumPayloadDomainImpl<Enum, Payload>>(c);
}

/// make_error factory functions

template <typename Enum>
  requires std::is_enum_v<Enum>
EnumPayloadError<Enum> make_error(
    Enum e, std::source_location loc = std::source_location::current()) {
  return EnumPayloadError<Enum>({e, loc});
}

template <typename Enum, typename Payload>
  requires EnumPayload<Enum, Payload> &&
           std::convertible_to<Payload, std::string>
EnumPayloadError<Enum, std::string> make_error(
    Enum e, Payload &&payload,
    std::source_location loc = std::source_location::current()) {
  return EnumPayloadError<Enum, std::string>(
      {e, std::string(std::forward<Payload>(payload)), loc});
}

template <typename Enum, typename Payload>
  requires EnumPayload<Enum, Payload>
EnumPayloadError<Enum, Payload> make_error(
    Enum e, Payload &&payload,
    std::source_location loc = std::source_location::current()) {
  return EnumPayloadError<Enum, Payload>(
      {e, std::forward<Payload>(payload), loc});
}

template <typename T>
  requires fmt::is_formattable<T>::value
inline auto make_error(
    T &&value, std::source_location location = std::source_location::current())
    -> std::conditional_t<std::convertible_to<T, std::string>,
                          status_code<AnyhowDomainImpl<std::string>>,
                          status_code<AnyhowDomainImpl<T>>> {
  if constexpr (std::convertible_to<T, std::string>) {
    using StatusCode = status_code<AnyhowDomainImpl<std::string>>;
    return StatusCode({std::string(std::forward<T>(value)), location});
  } else {
    using StatusCode = status_code<AnyhowDomainImpl<T>>;
    return StatusCode({std::forward<T>(value), location});
  }
}

SYSTEM_ERROR2_NAMESPACE_END

using SYSTEM_ERROR2_NAMESPACE::make_error;
