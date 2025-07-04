#pragma once

#include <stdint.h>
#include <exception>
#include <stdexcept>
#include <vector>

class BitMap {
public:
  explicit BitMap(size_t size) : size_(size), data_((size + 63) / 64, 0) {}

  void set(size_t offset) {
    if (offset >= size_) {
      throw std::out_of_range("BitMap::set");
    }
    data_[offset / 64] |= 1ULL << (offset % 64);
  }

  void reset(size_t offset) {
    if (offset >= size_) {
      throw std::out_of_range("BitMap::reset");
    }
    data_[offset / 64] &= ~(1ULL << (offset % 64));
  }

  bool at(size_t offset) const {
    if (offset >= size_) {
      throw std::out_of_range("BitMap::at");
    }
    return !!(data_[offset / 64] & (1ULL << (offset % 64)));
  }

  bool operator[](size_t offset) const {
    return !!(data_[offset / 64] & (1ULL << (offset % 64)));
  }

  size_t size() const {
    return size_;
  }

private:
  size_t size_;
  std::vector<uint64_t> data_;
};

int main() {
  return 0;
}
