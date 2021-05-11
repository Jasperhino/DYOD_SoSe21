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
#include "dictionary_segment.hpp"

#include "utils/assert.hpp"

namespace opossum {

void Chunk::add_segment(std::shared_ptr<BaseSegment> segment) { _segments.emplace_back(segment); }

void Chunk::replace_segment(ColumnID column_id, const std::shared_ptr<BaseSegment> segment) {
  DebugAssert(column_id < column_count(), "segment with index not existing wrong size");
  _segments[column_id] = segment;
}

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  DebugAssert(values.size() == _segments.size(), "value vector has wrong size");
  std::shared_lock lock(_mutex);
  for (std::vector<AllTypeVariant>::size_type i = 0, size = values.size(); i < size; ++i) {
    _segments[i]->append(values[i]);
  }
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const { return _segments[column_id]; }

ColumnCount Chunk::column_count() const { return (ColumnCount)_segments.size(); }

ChunkOffset Chunk::size() const {
  if (!_segments.empty()) {
    return _segments[0]->size();
  } else {
    return 0;
  }
}

}  // namespace opossum
