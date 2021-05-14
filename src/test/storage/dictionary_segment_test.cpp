#include <memory>
#include <string>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "resolve_type.hpp"
#include "storage/base_segment.hpp"
#include "storage/dictionary_segment.hpp"
#include "storage/value_segment.hpp"

namespace opossum {

class StorageDictionarySegmentTest : public BaseTest {
 protected:
  std::shared_ptr<ValueSegment<int>> vc_int = std::make_shared<ValueSegment<int>>();
  std::shared_ptr<ValueSegment<std::string>> vc_str = std::make_shared<ValueSegment<std::string>>();
};

template <typename T>
std::shared_ptr<DictionarySegment<T>> _convert_to_dictionary_segment(
    const std::shared_ptr<ValueSegment<T>> value_segment, const std::string& type_string) {
  std::shared_ptr<BaseSegment> col;
  resolve_data_type(type_string, [&](auto type) {
    using Type = typename decltype(type)::type;
    col = std::make_shared<DictionarySegment<Type>>(value_segment);
  });
  return std::dynamic_pointer_cast<DictionarySegment<T>>(col);
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
  dict_col->append(2);
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

  EXPECT_EQ(dict_col->lower_bound(4), (ValueID)2);
  EXPECT_EQ(dict_col->upper_bound(4), (ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(5), (ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(5), (ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(15), INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->upper_bound(15), INVALID_VALUE_ID);
}

TEST_F(StorageDictionarySegmentTest, LowerUpperBoundAllTypeVariant) {
  for (int i = 0; i <= 10; i += 2) vc_int->append(i);

  auto const dict_col = _convert_to_dictionary_segment(vc_int, "int");

  EXPECT_EQ(dict_col->lower_bound(AllTypeVariant{4}), (ValueID)2);
  EXPECT_EQ(dict_col->upper_bound(AllTypeVariant{4}), (ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(AllTypeVariant{5}), (ValueID)3);
  EXPECT_EQ(dict_col->upper_bound(AllTypeVariant{5}), (ValueID)3);

  EXPECT_EQ(dict_col->lower_bound(AllTypeVariant{15}), INVALID_VALUE_ID);
  EXPECT_EQ(dict_col->upper_bound(AllTypeVariant{15}), INVALID_VALUE_ID);
}

TEST_F(StorageDictionarySegmentTest, AttributeVectorWidth) {
  for (int i = 0; i < 10; ++i) vc_int->append(i);
  auto dict_col = _convert_to_dictionary_segment(vc_int, "int");
  EXPECT_EQ(dict_col->attribute_vector()->width(), sizeof(uint8_t));
}

TEST_F(StorageDictionarySegmentTest, AttributeVector8) {
  auto attribute_vector = DictionarySegment<int>::attribute_vector_for_dictionary(1, 1);
  EXPECT_EQ(attribute_vector->width(), sizeof(uint8_t));
}

TEST_F(StorageDictionarySegmentTest, AttributeVector16) {
  auto attribute_vector =
      DictionarySegment<int>::attribute_vector_for_dictionary(std::numeric_limits<std::uint8_t>::max(), 1);
  EXPECT_EQ(attribute_vector->width(), sizeof(uint16_t));
}

TEST_F(StorageDictionarySegmentTest, AttributeVector32) {
  auto attribute_vector =
      DictionarySegment<int>::attribute_vector_for_dictionary(std::numeric_limits<std::uint16_t>::max(), 1);
  EXPECT_EQ(attribute_vector->width(), sizeof(uint32_t));
}

TEST_F(StorageDictionarySegmentTest, AttributeVectorTooBig) {
  EXPECT_THROW(DictionarySegment<int>::attribute_vector_for_dictionary(std::numeric_limits<std::uint32_t>::max(), 1),
               std::exception);
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
