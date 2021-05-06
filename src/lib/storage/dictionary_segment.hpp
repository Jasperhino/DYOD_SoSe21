#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cstdint>

#include "all_type_variant.hpp"
#include "base_attribute_vector.hpp"
#include "fixed_size_attribute_vector.hpp"
#include "base_segment.hpp"
#include "type_cast.hpp"
#include "types.hpp"

namespace opossum {

class BaseAttributeVector;
class BaseSegment;

// Even though ValueIDs do not have to use the full width of ValueID (uint32_t), this will also work for smaller ValueID
// types (uint8_t, uint16_t) since after a down-cast INVALID_VALUE_ID will look like their numeric_limit::max()
constexpr ValueID INVALID_VALUE_ID{std::numeric_limits<ValueID::base_type>::max()};

// Dictionary is a specific segment type that stores all its values in a vector
template <typename T>
class DictionarySegment : public BaseSegment {
 public:
  /**
   * Creates a Dictionary segment from a given value segment.
   */
  explicit DictionarySegment(const std::shared_ptr<BaseSegment>& base_segment) {
    std::set<T> _dictionary_set;

    for (ChunkOffset index = 0, base_segment_size = base_segment->size(); index < base_segment_size; index++) {
      T element = type_cast<T>((*base_segment)[index]);
      _dictionary_set.emplace(element);
    }
    _dictionary = std::make_shared<std::vector<T>>(_dictionary_set.begin(), _dictionary_set.end());

    const size_t dictionary_size = unique_values_count();
    if(dictionary_size < 256){
      _attribute_vector = std::make_shared<FixedSizeAttributeVector<std::uint8_t>>();
    } else if(dictionary_size < 65536){
      _attribute_vector = std::make_shared<FixedSizeAttributeVector<std::uint16_t>>();
    } else {
      _attribute_vector = std::make_shared<FixedSizeAttributeVector<std::uint32_t>>();
    }

    for (ChunkOffset index = 0, base_segment_size = base_segment->size(); index < base_segment_size; index++) {
      AllTypeVariant element = (*base_segment)[index];

      auto result = std::find(_dictionary->begin(), _dictionary->end(), type_cast<T>(element));

      if (result != _dictionary->end()) {
        ValueID dictionary_index = (ValueID)std::distance(_dictionary->begin(), result);
        _attribute_vector->set(index, dictionary_index);
      }
    }
  }

  // SEMINAR INFORMATION: Since most of these methods depend on the template parameter, you will have to implement
  // the DictionarySegment in this file. Replace the method signatures with actual implementations.

  // return the value at a certain position. If you want to write efficient operators, back off!
  AllTypeVariant operator[](const ChunkOffset chunk_offset) const override { return (*_dictionary)[0]; };

  // return the value at a certain position.
  T get(const size_t chunk_offset) const;

  // dictionary segments are immutable
  void append(const AllTypeVariant& val) override{};

  // returns an underlying dictionary
  std::shared_ptr<const std::vector<T>> dictionary() const { return _dictionary; };

  // returns an underlying data structure
  std::shared_ptr<const BaseAttributeVector> attribute_vector() const;

  // return the value represented by a given ValueID
  const T& value_by_value_id(ValueID value_id) const;

  // returns the first value ID that refers to a value >= the search value
  // returns INVALID_VALUE_ID if all values are smaller than the search value
  ValueID lower_bound(T value) const {
    auto result = std::lower_bound(_dictionary->begin(), _dictionary->end(), value);
    if (result != _dictionary->end()) {
      return (ValueID)std::distance(_dictionary->begin(), result);
    } else {
      return INVALID_VALUE_ID;
    }
  };

  // same as lower_bound(T), but accepts an AllTypeVariant
  ValueID lower_bound(const AllTypeVariant& value) const { return lower_bound(type_cast<T>(value)); };

  // returns the first value ID that refers to a value > the search value
  // returns INVALID_VALUE_ID if all values are smaller than or equal to the search value
  ValueID upper_bound(T value) const {
    auto result = std::upper_bound(_dictionary->begin(), _dictionary->end(), value);
    if (result != _dictionary->end()) {
      return (ValueID)std::distance(_dictionary->begin(), result);
    } else {
      return INVALID_VALUE_ID;
    }
  };

  // same as upper_bound(T), but accepts an AllTypeVariant
  ValueID upper_bound(const AllTypeVariant& value) const { return upper_bound(type_cast<T>(value)); };

  // return the number of unique_values (dictionary entries)
  size_t unique_values_count() const { return _dictionary->size(); };

  // return the number of entries
  ChunkOffset size() const override { return _attribute_vector->size(); };

  // returns the calculated memory usage
  size_t estimate_memory_usage() const final {
    // TODO(WE)
    return 0;
  };

 protected:
  std::shared_ptr<std::vector<T>> _dictionary;
  std::shared_ptr<BaseAttributeVector> _attribute_vector;
};

}  // namespace opossum
