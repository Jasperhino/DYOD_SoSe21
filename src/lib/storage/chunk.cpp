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

void Chunk::add_segment(std::shared_ptr<BaseSegment> segment) { vec.push_back(segment); }

void Chunk::append(const std::vector<AllTypeVariant>& values) {
  DebugAssert(values.size() == vec.size(), "value vector has wrong size");
  for (std::vector<AllTypeVariant>::size_type i = 0; i < values.size(); ++i) {
    vec[i]->append(values[i]);
  }
}

std::shared_ptr<BaseSegment> Chunk::get_segment(ColumnID column_id) const { return vec[column_id]; }

ColumnCount Chunk::column_count() const { return ColumnCount{(ColumnID)vec.size()}; }

ChunkOffset Chunk::size() const {
  if (vec.size()) {
    return vec[0]->size();
  } else {
    return 0;
  }
}

}  // namespace opossum
