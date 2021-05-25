#include <iomanip>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "base_segment.hpp"
#include "chunk.hpp"

#include "utils/assert.hpp"

namespace opossum {

void Chunk::add_segment(std::shared_ptr<BaseSegment> segment) {
  DebugAssert(segment == nullptr || column_count() == 0 || segment->size() == size(),
              "Segment has wrong size. Should be " + std::to_string(size()));
  _segments.push_back(segment);
}

void Chunk::replace_segment(ColumnID column_id, const std::shared_ptr<BaseSegment> segment) {
  std::unique_lock lock(_mutex);
  _segments.at(column_id) = segment;
}

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  DebugAssert(values.size() == column_count(),
              "Value vector has wrong size. Should be " + std::to_string(column_count()));
  for (std::vector<AllTypeVariant>::size_type column_index = 0, size = values.size(); column_index < size;
       ++column_index) {
    _segments[column_index]->append(values[column_index]);
  }
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const { return _segments.at(column_id); }

ColumnCount Chunk::column_count() const { return static_cast<ColumnCount>(_segments.size()); }

ChunkOffset Chunk::size() const {
  if (!_segments.empty()) {
    return _segments[0]->size();
  } else {
    return 0;
  }
}

}  // namespace opossum
