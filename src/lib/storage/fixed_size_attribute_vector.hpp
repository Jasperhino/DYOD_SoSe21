#pragma once

#include "base_attribute_vector.hpp"

namespace opossum {

template <typename T>
class FixedSizeAttributeVector : public BaseAttributeVector {
 public:
  ~FixedSizeAttributeVector() = default;

  ValueID get(const size_t i) const override;

  void set(const size_t i, const ValueID value_id) override;

  size_t size() const override;

  AttributeVectorWidth width() const final;

 protected:
  std::vector<T> _attribute_vector = {};
};

}  // namespace opossum