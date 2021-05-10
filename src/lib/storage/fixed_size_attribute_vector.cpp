#include "fixed_size_attribute_vector.hpp"

#include "type_cast.hpp"
#include "types.hpp"
#include <math.h>

namespace opossum {

template <typename T>
FixedSizeAttributeVector<T>::FixedSizeAttributeVector(const size_t size) {
  _attribute_vector = std::vector<T>(size);
}

template <typename T>
ValueID FixedSizeAttributeVector<T>::get(const size_t i) const {
  return (ValueID) _attribute_vector.at(i);
}

template <typename T>
size_t FixedSizeAttributeVector<T>::size() const {
  return _attribute_vector.size();
}

template <typename T>
AttributeVectorWidth FixedSizeAttributeVector<T>::width() const {
  // TODO(rethink, understand method!)
  return ceil ((log2 (size() - 1) + 1) / 8);
}

template <typename T>
void FixedSizeAttributeVector<T>::set(const size_t i, const ValueID value_id) {
  DebugAssert(i < size(), "Position out of range");
  _attribute_vector[i] = value_id;
}

// clang-format off
#define data_types_macro_2                              (uint32_t) (uint16_t) (uint8_t)
// clang-format on
BOOST_PP_SEQ_FOR_EACH(EXPLICIT_INSTANTIATION, FixedSizeAttributeVector, data_types_macro_2)

}  // namespace opossum
