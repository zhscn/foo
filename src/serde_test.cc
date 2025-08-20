#include "serde.hh"

#include <fmt/core.h>
#include <gtest/gtest.h>
#include <unordered_set>

TEST(Serialize, bool) {
  Serializer serializer;
  serialize(serializer, true);
  auto s = serializer.take();
  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s[0], '\x01');
}

TEST(Serialize, u8) {
  Serializer serializer;
  serialize(serializer, 0xabU);
  auto s = serializer.take();
  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s[0], '\xab');
}

TEST(Serialize, u16) {
  Serializer serializer;
  serialize(serializer, 0xabcdU);
  auto s = serializer.take();
  EXPECT_EQ(s.size(), 3);
  EXPECT_EQ(s[0], '\xfd');
  EXPECT_EQ(s[1], '\xcd');
  EXPECT_EQ(s[2], '\xab');
}

TEST(Serialize, u32) {
  Serializer serializer;
  serialize(serializer, 0x01020304U);
  auto s = serializer.take();
  EXPECT_EQ(s.size(), 5);
  EXPECT_EQ(s[0], '\xfe');
  EXPECT_EQ(s[1], '\x04');
  EXPECT_EQ(s[2], '\x03');
  EXPECT_EQ(s[3], '\x02');
  EXPECT_EQ(s[4], '\x01');
}

TEST(Serialize, u64) {
  Serializer serializer;
  serialize(serializer, 0x01020304050607ULL);
  auto s = serializer.take();
  EXPECT_EQ(s.size(), 9);
  EXPECT_EQ(s[0], '\xff');
  EXPECT_EQ(s[1], '\x07');
  EXPECT_EQ(s[2], '\x06');
  EXPECT_EQ(s[3], '\x05');
  EXPECT_EQ(s[4], '\x04');
  EXPECT_EQ(s[5], '\x03');
  EXPECT_EQ(s[6], '\x02');
  EXPECT_EQ(s[7], '\x01');
  EXPECT_EQ(s[8], '\x00');
}

TEST(Serialize, i8) {
  Serializer serializer;
  serialize(serializer, -42);
  auto s = serializer.take();
  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s[0], (char)detail::zig_zag_encode(-42));
}

TEST(Serialize, i16) {
  Serializer serializer;
  serialize(serializer, int16_t(-12345));
  auto s = serializer.take();
  auto v = detail::zig_zag_encode(-12345);
  EXPECT_EQ(s.size(), 3);
  EXPECT_EQ(s[0], '\xfd');
  EXPECT_EQ(s[1], (char)(v & 0xff));
  EXPECT_EQ(s[2], (char)((v >> 8) & 0xff));
}

TEST(Serialize, i32) {
  Serializer serializer;
  serialize(serializer, int32_t(-123456789));
  auto s = serializer.take();
  auto v = detail::zig_zag_encode(-123456789);
  EXPECT_EQ(s.size(), 5);
  EXPECT_EQ(s[0], '\xfe');
  EXPECT_EQ(s[1], (char)(v & 0xff));
  EXPECT_EQ(s[2], (char)((v >> 8) & 0xff));
  EXPECT_EQ(s[3], (char)((v >> 16) & 0xff));
  EXPECT_EQ(s[4], (char)((v >> 24) & 0xff));
}

TEST(Serialize, i64) {
  Serializer serializer;
  serialize(serializer, int64_t(-1234567890123456789));
  auto s = serializer.take();
  auto v = detail::zig_zag_encode(-1234567890123456789);
  EXPECT_EQ(s.size(), 9);
  EXPECT_EQ(s[0], '\xff');
  EXPECT_EQ(s[1], (char)(v & 0xff));
  EXPECT_EQ(s[2], (char)((v >> 8) & 0xff));
  EXPECT_EQ(s[3], (char)((v >> 16) & 0xff));
  EXPECT_EQ(s[4], (char)((v >> 24) & 0xff));
  EXPECT_EQ(s[5], (char)((v >> 32) & 0xff));
  EXPECT_EQ(s[6], (char)((v >> 40) & 0xff));
  EXPECT_EQ(s[7], (char)((v >> 48) & 0xff));
  EXPECT_EQ(s[8], (char)((v >> 56) & 0xff));
}

TEST(Serialize, str) {
  Serializer serializer;
  serialize(serializer, "hello");
  auto s = serializer.take();
  EXPECT_EQ(s.size(), 6);
  EXPECT_EQ(s[0], '\x05');
  EXPECT_EQ(s[1], 'h');
  EXPECT_EQ(s[2], 'e');
  EXPECT_EQ(s[3], 'l');
  EXPECT_EQ(s[4], 'l');
  EXPECT_EQ(s[5], 'o');
}

TEST(Serialize, str_empty) {
  Serializer serializer;
  serialize(serializer, "");
  auto s = serializer.take();
  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s[0], '\x00');
}

TEST(Deserialize, bool) {
  Serializer serializer;
  serializer.write_uint(1);
  auto s = serializer.take();
  Deserializer deserializer(s);
  bool value = false;
  deserialize(deserializer, value);
  EXPECT_TRUE(value);
}

TEST(Deserialize, u8) {
  Serializer serializer;
  serializer.write_uint(0xabU);
  auto s = serializer.take();
  Deserializer deserializer(s);
  uint8_t value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, 0xabU);
}

TEST(Deserialize, u16) {
  Serializer serializer;
  serializer.write_uint(0xabcdU);
  auto s = serializer.take();
  Deserializer deserializer(s);
  uint16_t value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, 0xabcdU);
}

TEST(Deserialize, u32) {
  Serializer serializer;
  serializer.write_uint(0x01020304U);
  auto s = serializer.take();
  Deserializer deserializer(s);
  uint32_t value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, 0x01020304U);
}

TEST(Deserialize, u64) {
  Serializer serializer;
  serializer.write_uint(0x01020304050607ULL);
  auto s = serializer.take();
  Deserializer deserializer(s);
  uint64_t value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, 0x01020304050607ULL);
}

TEST(Deserialize, i8) {
  Serializer serializer;
  serializer.write_int(-42);
  auto s = serializer.take();
  Deserializer deserializer(s);
  int8_t value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, -42);
}

TEST(Deserialize, i16) {
  Serializer serializer;
  serializer.write_int(int16_t(-12345));
  auto s = serializer.take();
  Deserializer deserializer(s);
  int16_t value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, int16_t(-12345));
}

TEST(Deserialize, i32) {
  Serializer serializer;
  serializer.write_int(int32_t(-123456789));
  auto s = serializer.take();
  Deserializer deserializer(s);
  int32_t value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, int32_t(-123456789));
}

TEST(Deserialize, i64) {
  Serializer serializer;
  serializer.write_int(int64_t(-1234567890123456789));
  auto s = serializer.take();
  Deserializer deserializer(s);
  int64_t value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, int64_t(-1234567890123456789));
}

TEST(Deserialize, str) {
  Serializer serializer;
  serializer.write_str("hello");
  auto s = serializer.take();
  Deserializer deserializer(s);
  std::string_view value;
  deserialize(deserializer, value);
  EXPECT_EQ(value, "hello");
}

TEST(Deserialize, optional) {
  Serializer serializer;
  std::optional<int32_t> input = 42;
  serialize(serializer, input);
  auto s = serializer.take();
  Deserializer deserializer(s);
  std::optional<int32_t> value;
  deserialize(deserializer, value);
  EXPECT_TRUE(value.has_value());
  EXPECT_EQ(*value, 42);

  Serializer serializer2;
  std::optional<int32_t> input2;
  serialize(serializer2, input2);
  s = serializer2.take();
  Deserializer deserializer2(s);
  std::optional<int32_t> value2;
  deserialize(deserializer2, value2);
  EXPECT_FALSE(value2.has_value());
}

TEST(Float, float) {
  Serializer serializer;
  serialize(serializer, 1.23F);
  auto s = serializer.take();
  Deserializer deserializer(s);
  float value = 0;
  deserialize(deserializer, value);
  EXPECT_EQ(value, 1.23F);
}

TEST(Container, vector) {
  Serializer serializer;
  std::vector<int32_t> input = {1, 2, 3};
  serialize(serializer, input);
  auto s = serializer.take();
  Deserializer deserializer(s);
  std::vector<int32_t> value;
  deserialize(deserializer, value);
  EXPECT_EQ(value.size(), input.size());
  for (std::size_t i = 0; i < input.size(); ++i) {
    EXPECT_EQ(value[i], input[i]);
  }
}

TEST(Container, set) {
  Serializer serializer;
  std::set<int32_t> input = {1, 2, 3};
  serialize(serializer, input);
  auto s = serializer.take();
  Deserializer deserializer(s);
  std::set<int32_t> value;
  deserialize(deserializer, value);
  EXPECT_EQ(value.size(), input.size());
  for (const auto &v : input) {
    EXPECT_TRUE(value.count(v));
  }
}

TEST(Container, map) {
  Serializer serializer;
  std::map<int32_t, int32_t> input = {{1, 2}, {3, 4}, {5, 6}};
  serialize(serializer, input);
  auto s = serializer.take();
  Deserializer deserializer(s);
  std::map<int32_t, int32_t> value;
  deserialize(deserializer, value);
  EXPECT_EQ(value.size(), input.size());
  for (const auto &[k, v] : input) {
    EXPECT_EQ(value[k], v);
  }
}

TEST(Container, unordered_map) {
  Serializer serializer;
  std::unordered_map<int32_t, int32_t> input = {{1, 2}, {3, 4}, {5, 6}};
  serialize(serializer, input);
  auto s = serializer.take();
  Deserializer deserializer(s);
  std::unordered_map<int32_t, int32_t> value;
  deserialize(deserializer, value);
  EXPECT_EQ(value.size(), input.size());
  for (const auto &[k, v] : input) {
    EXPECT_EQ(value[k], v);
  }
}

TEST(Container, unordered_set) {
  Serializer serializer;
  std::unordered_set<int32_t> input = {1, 2, 3};
  serialize(serializer, input);
  auto s = serializer.take();
  Deserializer deserializer(s);
  std::unordered_set<int32_t> value;
  deserialize(deserializer, value);
  EXPECT_EQ(value.size(), input.size());
  for (const auto &v : input) {
    EXPECT_TRUE(value.count(v));
  }
}
