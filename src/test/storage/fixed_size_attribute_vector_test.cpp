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
  EXPECT_EQ(uint8_attribute_vector.width(), sizeof(uint8_t));
}

} // namespace opossum
