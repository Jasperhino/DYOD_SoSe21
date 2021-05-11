#include "fixed_size_attribute_vector.hpp"

#include <vector>
#include "type_cast.hpp"
#include "types.hpp"

namespace opossum {

template <typename T>
FixedSizeAttributeVector<T>::FixedSizeAttributeVector(const size_t size) {
  _attribute_vector = std::vector<T>(size);
}

template <typename T>
ValueID FixedSizeAttributeVector<T>::get(const size_t i) const {
  return (ValueID)_attribute_vector.at(i);
}

template <typename T>
size_t FixedSizeAttributeVector<T>::size() const {
  return _attribute_vector.size();
}

template <typename T>
AttributeVectorWidth FixedSizeAttributeVector<T>::width() const {
  return sizeof(T);
}

template <typename T>
void FixedSizeAttributeVector<T>::set(const size_t i, const ValueID value_id) {
  DebugAssert(i < size(), "Position out of range");
  _attribute_vector[i] = value_id;
}

template class FixedSizeAttributeVector<uint8_t>;
template class FixedSizeAttributeVector<uint16_t>;
template class FixedSizeAttributeVector<uint32_t>;

}  // namespace opossum
