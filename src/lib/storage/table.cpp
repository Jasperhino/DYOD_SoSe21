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
  _chunk_size = target_chunk_size;
  _newest_chunk = std::make_shared<Chunk>();
}

void Table::add_column(const std::string& name, const std::string& type) {
  DebugAssert(!row_count(), "add_column must be called before adding entries");
  _column_names.push_back(name);
  _column_types.push_back(type);
  _name_id_mapping[name] = ColumnID{ (ColumnID) (_column_names.size() - 1)};
  resolve_data_type(type, [&](const auto data_type_t) {
    using ColumnDataType = typename decltype(data_type_t)::type;
    const auto value_segment = std::make_shared<ValueSegment<ColumnDataType>>();
    _newest_chunk->add_segment(value_segment);
  });
}

void Table::append(const std::vector<AllTypeVariant>& values) {
  if(_newest_chunk->size() >= _chunk_size) {
    _immutable_chunks.push_back(_newest_chunk);
    _newest_chunk = std::make_shared<Chunk>();
  }
  _newest_chunk->append(values);
}

ColumnCount Table::column_count() const {
  return ColumnCount{(ColumnID) _column_names.size()};
}

uint64_t Table::row_count() const {
  return _chunk_size * _immutable_chunks.size() + _newest_chunk->size();
}

ChunkID Table::chunk_count() const {
  // Implementation goes here
  return ChunkID{(ChunkID) _immutable_chunks.size() + 1};
}

ColumnID Table::column_id_by_name(const std::string& column_name) const {
  return _name_id_mapping.at(column_name);
}

ChunkOffset Table::target_chunk_size() const { return _chunk_size; }

const std::vector<std::string>& Table::column_names() const {
  return _column_names;
}

const std::string& Table::column_name(const ColumnID column_id) const {
  return _column_names[column_id];
}

const std::string& Table::column_type(const ColumnID column_id) const {
  return _column_types[column_id];
}

Chunk& Table::get_chunk(ChunkID chunk_id) {
  return *_newest_chunk;
}

const Chunk& Table::get_chunk(ChunkID chunk_id) const {
  return *_immutable_chunks[chunk_id];
}

}  // namespace opossum
