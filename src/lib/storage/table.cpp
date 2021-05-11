#include "table.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "value_segment.hpp"

#include "resolve_type.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

Table::Table(const ChunkOffset target_chunk_size) {
  _target_chunk_size = target_chunk_size;
  _chunks.push_back(std::make_shared<Chunk>());
}

void Table::_add_segment_to_chunk(std::shared_ptr<Chunk>& chunk, const std::string& type) {
  resolve_data_type(type, [&](const auto data_type_t) {
    using ColumnDataType = typename decltype(data_type_t)::type;
    const auto value_segment = std::make_shared<ValueSegment<ColumnDataType>>();
    chunk->add_segment(value_segment);
  });
}

void Table::add_column(const std::string& name, const std::string& type) {
  Assert(!row_count(), "add_column must be called before adding entries");
  _column_names.push_back(name);
  _column_types.push_back(type);
  _name_id_mapping[name] = static_cast<ColumnID>(_column_names.size() - 1);
  _add_segment_to_chunk(_chunks.back(), type);
}

void Table::append(const std::vector<AllTypeVariant>& values) {
  DebugAssert(values.size() == column_count(), "Values have wrong size. Should be " + std::to_string(column_count()));
  if (_chunks.back()->size() >= _target_chunk_size) {
    auto chunk = std::make_shared<Chunk>();
    for (const std::string& type : _column_types) {
      _add_segment_to_chunk(chunk, type);
    }
    _chunks.push_back(chunk);
  }
  _chunks.back()->append(values);
}

ColumnCount Table::column_count() const { return static_cast<ColumnCount>(_column_names.size()); }

uint64_t Table::row_count() const {
  uint64_t row_count = 0L;
  for (const auto& chunk : _chunks) {
    row_count += chunk->size();
  }
  return row_count;
}

ChunkID Table::chunk_count() const { return static_cast<ChunkID>(_chunks.size()); }

ColumnID Table::column_id_by_name(const std::string& column_name) const { return _name_id_mapping.at(column_name); }

ChunkOffset Table::target_chunk_size() const { return _target_chunk_size; }

const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(const ColumnID column_id) const { return _column_names.at(column_id); }

const std::string& Table::column_type(const ColumnID column_id) const { return _column_types.at(column_id); }

Chunk& Table::get_chunk(ChunkID chunk_id) { return *_chunks.at(chunk_id); }

const Chunk& Table::get_chunk(ChunkID chunk_id) const { return *_chunks.at(chunk_id); }

}  // namespace opossum
