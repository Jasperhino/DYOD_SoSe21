#pragma once

#include <vector>
#include "base_attribute_vector.hpp"
#include "utils/assert.hpp"

namespace opossum {

template <typename uintX_t>
class FixedSizeAttributeVector : public BaseAttributeVector {
 public:
  explicit FixedSizeAttributeVector(const size_t size);

  ~FixedSizeAttributeVector() = default;

  ValueID get(const size_t i) const override;

  void set(const size_t i, const ValueID value_id) override;

  size_t size() const override;

  AttributeVectorWidth width() const final;

 protected:
  std::vector<uintX_t> _attribute_vector;
};

}  // namespace opossum
