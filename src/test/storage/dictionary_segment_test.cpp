#include <memory>
#include <string>

#include "gtest/gtest.h"

#include "../../lib/resolve_type.hpp"
#include "../../lib/storage/base_segment.hpp"
#include "../../lib/storage/dictionary_segment.hpp"
#include "../../lib/storage/value_segment.hpp"

namespace opossum {

class StorageDictionarySegmentTest : public ::testing::Test {
 protected:
  std::shared_ptr<opossum::ValueSegment<int>> vc_int = std::make_shared<opossum::ValueSegment<int>>();
  std::shared_ptr<opossum::ValueSegment<std::string>> vc_str = std::make_shared<opossum::ValueSegment<std::string>>();
};

template <typename T>
std::shared_ptr<opossum::DictionarySegment<T>> _convert_to_dictionary_segment(
    const std::shared_ptr<opossum::ValueSegment<T>> value_segment, const std::string& type_string) {
  std::shared_ptr<BaseSegment> col;
  resolve_data_type(type_string, [&](auto type) {
    using Type = typename decltype(type)::type;
    col = std::make_shared<DictionarySegment<Type>>(value_segment);
  });
  return std::dynamic_pointer_cast<opossum::DictionarySegment<T>>(col);
}

TEST_F(StorageDictionarySegmentTest, CompressSegmentString) {
  vc_str->append("Bill");
  vc_str->append("Steve");
  vc_str->append("Alexander");
  vc_str->append("Steve");
  vc_str->append("Hasso");
  vc_str->append("Bill");

  auto dict_col = _convert_to_dictionary_segment(vc_str, "string");

  // Test dictionary size (uniqueness)
  EXPECT_EQ(dict_col->unique_values_count(), 4u);

  // Test sorting
  auto dict = dict_col->dictionary();
  EXPECT_EQ((*dict)[0], "Alexander");
  EXPECT_EQ((*dict)[1], "Bill");
  EXPECT_EQ((*dict)[2], "Hasso");
  EXPECT_EQ((*dict)[3], "Steve");

  // Test attribute_vector size
  EXPECT_EQ(dict_col->size(), 6u);
  EXPECT_EQ(dict_col->attribute_vector()->size(), 6u);
}

TEST_F(StorageDictionarySegmentTest, AccessValues) {
  vc_str->append("Bill");
  vc_str->append("Steve");
  vc_str->append("Alexander");
  vc_str->append("Steve");
  vc_str->append("Hasso");
  vc_str->append("Bill");
  auto dict_col = _convert_to_dictionary_segment(vc_str, "string");

  EXPECT_EQ(type_cast<std::string>((*dict_col)[0]), "Bill");
  EXPECT_EQ(type_cast<std::string>((*dict_col)[1]), "Steve");
  EXPECT_EQ(type_cast<std::string>((*dict_col)[2]), "Alexander");
  EXPECT_EQ(type_cast<std::string>((*dict_col)[3]), "Steve");
  EXPECT_EQ(type_cast<std::string>((*dict_col)[4]), "Hasso");
  EXPECT_EQ(type_cast<std::string>((*dict_col)[5]), "Bill");

  if constexpr (HYRISE_DEBUG) {
    EXPECT_THROW((*dict_col)[6], std::exception);
  }
}

TEST_F(StorageDictionarySegmentTest, GetValues) {
  vc_str->append("Bill");
  vc_str->append("Steve");
  vc_str->append("Alexander");
  vc_str->append("Steve");
  vc_str->append("Hasso");
  vc_str->append("Bill");
  auto dict_col = _convert_to_dictionary_segment(vc_str, "string");

  EXPECT_EQ(dict_col->get(0), "Bill");
  EXPECT_EQ(dict_col->get(1), "Steve");
  EXPECT_EQ(dict_col->get(2), "Alexander");
  EXPECT_EQ(dict_col->get(3), "Steve");
  EXPECT_EQ(dict_col->get(4), "Hasso");
  EXPECT_EQ(dict_col->get(5), "Bill");

  if constexpr (HYRISE_DEBUG) {
    EXPECT_THROW(dict_col->get(6), std::exception);
  }
}

TEST_F(StorageDictionarySegmentTest, AppendValues) {
  vc_int->append(1);
  auto dict_col = _convert_to_dictionary_segment(vc_int, "int");

  EXPECT_EQ(dict_col->size(), 1u);

  // Test that appending to a dictionary segment does not add values
  vc_int->append(2);
  EXPECT_EQ(dict_col->size(), 1u);
}

TEST_F(StorageDictionarySegmentTest, ValueByValueID) {
  vc_str->append("Bill");
  vc_str->append("Steve");
  vc_str->append("Alexander");
  vc_str->append("Steve");
  vc_str->append("Hasso");
  vc_str->append("Bill");
  auto dict_col = _convert_to_dictionary_segment(vc_str, "string");

  EXPECT_EQ(dict_col->value_by_value_id(ValueID{0}), "Alexander");
  EXPECT_EQ(dict_col->value_by_value_id(ValueID{3}), "Steve");

  if constexpr (HYRISE_DEBUG) {
    EXPECT_THROW(dict_col->value_by_value_id(ValueID{4}), std::exception);
  }
}

TEST_F(StorageDictionarySegmentTest, LowerUpperBound) {
  for (int i = 0; i <= 10; i += 2) vc_int->append(i);

  auto dict_col = _convert_to_dictionary_segment(vc_int, "int");

  EXPECT_EQ(dict_col->lower_bound(4), (opossum::ValueID)2);
  EXPECT_EQ(dict_col->upper_bound(4), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(5), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(5), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(15), opossum::INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->upper_bound(15), opossum::INVALID_VALUE_ID);
}

TEST_F(StorageDictionarySegmentTest, LowerUpperBoundAllTypeVariant) {
  for (int i = 0; i <= 10; i += 2) vc_int->append(i);

  auto const dict_col = _convert_to_dictionary_segment(vc_int, "int");

  EXPECT_EQ(dict_col->lower_bound(AllTypeVariant{4}), (opossum::ValueID)2);
  EXPECT_EQ(dict_col->upper_bound(AllTypeVariant{4}), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(AllTypeVariant{5}), (opossum::ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(AllTypeVariant{5}), (opossum::ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(AllTypeVariant{15}), opossum::INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->upper_bound(AllTypeVariant{15}), opossum::INVALID_VALUE_ID);
}

TEST_F(StorageDictionarySegmentTest, AttributeVector8) {
  for (int i = 0; i < std::numeric_limits<std::uint8_t>::max() - 1; ++i) vc_int->append(i);
  auto dict_col = _convert_to_dictionary_segment(vc_int, "int");

  EXPECT_EQ(dict_col->attribute_vector()->width(), sizeof(uint8_t));
}

TEST_F(StorageDictionarySegmentTest, AttributeVector16) {
  // TODO(we): absolutely not performant
  // for (int i = 0; i < std::numeric_limits<std::uint16_t>::max() - 1; ++i) vc_int->append(i);
  // auto dict_col = _convert_to_dictionary_segment(vc_int, "int");

  // EXPECT_EQ(dict_col->attribute_vector()->width(), sizeof(uint16_t));
}

TEST_F(StorageDictionarySegmentTest, AttributeVector32) {
  // TODO(we): absolutely not performant
  // for (uint i = 0; i < std::numeric_limits<std::uint32_t>::max() - 1; ++i) vc_int->append((int)i);
  // auto dict_col = _convert_to_dictionary_segment(vc_int, "int");

  // EXPECT_EQ(dict_col->attribute_vector()->width(), sizeof(uint32_t));
}

TEST_F(StorageDictionarySegmentTest, AttributeVectorTooBig) {
  // TODO(we): absolutely not performant
  // for (int i = 0; i < std::numeric_limits<std::uint32_t>::max(); ++i) vc_int->append(i);
  // EXPECT_THROW(_convert_to_dictionary_segment(vc_int, "int"), std::exception);
}

TEST_F(StorageDictionarySegmentTest, MemoryUsage) {
  vc_int->append(1);
  auto dict_col = _convert_to_dictionary_segment(vc_int, "int");
  EXPECT_EQ(dict_col->estimate_memory_usage(), size_t{5});
  vc_int->append(1);
  dict_col = _convert_to_dictionary_segment(vc_int, "int");
  EXPECT_EQ(dict_col->estimate_memory_usage(), size_t{6});
  vc_int->append(2);
  dict_col = _convert_to_dictionary_segment(vc_int, "int");
  EXPECT_EQ(dict_col->estimate_memory_usage(), size_t{11});
  vc_int->append(1);
  dict_col = _convert_to_dictionary_segment(vc_int, "int");
  EXPECT_EQ(dict_col->estimate_memory_usage(), size_t{12});
}


}  // namespace opossum
