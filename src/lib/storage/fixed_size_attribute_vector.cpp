#include "fixed_size_attribute_vector.hpp"

namespace opossum {

template <typename T>
ValueID FixedSizeAttributeVector<T>::get(const size_t i) const {
  return _attribute_vector.at(i);
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
  _attribute_vector.reserve(i);
  _attribute_vector[i] = value_id;
}

}  // namespace opossum
