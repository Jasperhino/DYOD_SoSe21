#include <limits>
#include <string>
#include <vector>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/storage/fixed_size_attribute_vector.hpp"

namespace opossum {

class StorageFixedSizeAttributeVectorTest : public BaseTest {
 protected:
  FixedSizeAttributeVector<uint8_t> uint8_attribute_vector{5};
};

TEST_F(StorageFixedSizeAttributeVectorTest, Size) {
  EXPECT_EQ(uint8_attribute_vector.size(), 5u);
}

TEST_F(StorageFixedSizeAttributeVectorTest, Width) {
  EXPECT_EQ(uint8_attribute_vector.width(), sizeof(uint8_t));
}

TEST_F(StorageFixedSizeAttributeVectorTest, SetAndGet) {
  uint8_attribute_vector.set(0, ValueID{0});
  EXPECT_EQ(uint8_attribute_vector.get(0), ValueID{0});
  uint8_attribute_vector.set(0, ValueID{1});
  EXPECT_EQ(uint8_attribute_vector.get(0), ValueID{1});

  EXPECT_THROW(uint8_attribute_vector.set(5, ValueID{5}), std::exception);
  EXPECT_THROW(uint8_attribute_vector.get(5), std::exception);
}

} // namespace opossum
