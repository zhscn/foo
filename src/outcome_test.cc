#include <fmt/core.h>
#include <gtest/gtest.h>
#include <outcome.hh>

enum class MyErrc : uint8_t {
  Err1,
  Err2,
};

SYSTEM_ERROR2_NAMESPACE_BEGIN
template <>
struct quick_status_code_from_enum<MyErrc>
    : quick_status_code_from_enum_defaults<MyErrc> {
  static constexpr auto domain_name = "MyErrc";
  // 0x97f3b4b353c531f
  static constexpr auto domain_uuid = "455bc461-b14e-4e59-b46e-073205fdb9c9";
  // 0xd49fd48947f82a34 ^ 0xc44f7bdeb2cc50e9 = 0x10d0af57f5347add
  static constexpr auto payload_uuid = "fa0c63b4-1e0b-49ee-b9ae-ecc08646b0a3";
  static constexpr bool all_failures = true;
  static const std::initializer_list<mapping>& value_mappings() {
    // NOLINTBEGIN
    static const std::initializer_list<mapping> v = {
        {MyErrc::Err1, "Err1", {}},
        {MyErrc::Err2, "Err2", {}},
    };
    // NOLINTEND
    return v;
  }
};
SYSTEM_ERROR2_NAMESPACE_END

using MyError = SYSTEM_ERROR2_NAMESPACE::EnumPayloadError<MyErrc, std::string>;

Result<void> f0() {
  return make_error(MyErrc::Err1, "foo");
}

Result<void> f1() {
  return make_error(MyErrc::Err2, 123);
}

Result<void> f2() {
  return make_error(MyErrc::Err1);
}

Result<void> f3() {
  return MyErrc::Err1;
}

Result<void> f4() {
  return make_error("error");
}

Result<void> f5() {
  return make_error(1024);
}

TEST(outcome, test) {
  auto r0 = f0();
  ASSERT_TRUE(r0.has_failure());
  auto& err0 = r0.error();
  auto r1 = f1();
  ASSERT_TRUE(r1.has_failure());
  auto& err1 = r1.error();
  auto r2 = f2();
  ASSERT_TRUE(r2.has_failure());
  auto& err2 = r2.error();
  auto r3 = f3();
  ASSERT_TRUE(r3.has_failure());
  auto& err3 = r3.error();
  auto r4 = f4();
  ASSERT_TRUE(r4.has_failure());
  auto& err4 = r4.error();
  auto r5 = f5();
  ASSERT_TRUE(r5.has_failure());
  auto& err5 = r5.error();

  ASSERT_NE(err0, err1);
  ASSERT_NE(err1, err0);

  ASSERT_EQ(err0, err2);
  ASSERT_EQ(err2, err0);

  ASSERT_EQ(err0, err3);
  ASSERT_EQ(err3, err0);

  ASSERT_NE(err1, err2);
  ASSERT_NE(err2, err1);

  ASSERT_NE(err1, err3);
  ASSERT_NE(err3, err1);

  ASSERT_EQ(err2, err3);
  ASSERT_EQ(err3, err2);

  ASSERT_NE(err0, err4);
  ASSERT_NE(err1, err4);
  ASSERT_NE(err2, err4);
  ASSERT_NE(err3, err4);
  ASSERT_NE(err4, err0);
  ASSERT_NE(err4, err1);
  ASSERT_NE(err4, err2);
  ASSERT_NE(err4, err3);

  ASSERT_NE(err0, err5);
  ASSERT_NE(err1, err5);
  ASSERT_NE(err2, err5);
  ASSERT_NE(err3, err5);
  ASSERT_NE(err4, err5);
  ASSERT_NE(err5, err0);
  ASSERT_NE(err5, err1);
  ASSERT_NE(err5, err2);
  ASSERT_NE(err5, err3);
  ASSERT_NE(err5, err4);

  // fmt::println("err0: {}", err0);
  // fmt::println("err1: {}", err1);
  // fmt::println("err2: {}", err2);
  // fmt::println("err3: {}", err3);
  // fmt::println("err4: {}", err4);
  // fmt::println("err5: {}", err5);
}
