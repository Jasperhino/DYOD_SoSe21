#include "table.hpp"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "dictionary_segment.hpp"
#include "value_segment.hpp"

#include "resolve_type.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

Table::Table(const ChunkOffset target_chunk_size) {
  _chunk_size = target_chunk_size;
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
  DebugAssert(!row_count(), "add_column must be called before adding entries");
  _column_names.push_back(name);
  _column_types.push_back(type);
  _name_id_mapping[name] = (ColumnID)(_column_names.size() - 1);
  _add_segment_to_chunk(_chunks.back(), type);
}

void Table::append(const std::vector<AllTypeVariant>& values) {
  if (_chunks.back()->size() >= _chunk_size) {
    auto chunk = std::make_shared<Chunk>();
    for (const std::string& type : _column_types) {
      _add_segment_to_chunk(chunk, type);
    }
    _chunks.push_back(chunk);
  }
  _chunks.back()->append(values);
}

ColumnCount Table::column_count() const { return (ColumnCount)_column_names.size(); }

uint64_t Table::row_count() const { return _chunk_size * (_chunks.size() - 1) + _chunks.back()->size(); }

ChunkID Table::chunk_count() const { return (ChunkID)_chunks.size(); }

ColumnID Table::column_id_by_name(const std::string& column_name) const { return _name_id_mapping.at(column_name); }

ChunkOffset Table::target_chunk_size() const { return _chunk_size; }

const std::vector<std::string>& Table::column_names() const { return _column_names; }

const std::string& Table::column_name(const ColumnID column_id) const { return _column_names[column_id]; }

const std::string& Table::column_type(const ColumnID column_id) const { return _column_types[column_id]; }

Chunk& Table::get_chunk(ChunkID chunk_id) { return *_chunks[chunk_id]; }

const Chunk& Table::get_chunk(ChunkID chunk_id) const { return *_chunks[chunk_id]; }

void Table::_compress_segment(ChunkID chunk_id, ColumnID column_id, std::shared_ptr<BaseSegment> segment_new) {
  const Chunk& chunk_old = get_chunk(chunk_id);
  auto segment_old = chunk_old.get_segment(column_id);

  resolve_data_type(column_type(column_id), [&](auto type) {
    using Type = typename decltype(type)::type;
    segment_new = std::make_shared<DictionarySegment<Type>>(segment_old);
  });
}

void Table::compress_chunk(ChunkID chunk_id) {
  // TODO(we): mutex on old chunk while compressing

  const Chunk& chunk_old = get_chunk(chunk_id);
  auto chunk_new = std::make_shared<Chunk>();

  std::vector<std::shared_ptr<std::thread>> threads;

  size_t segment_count = chunk_old.column_count();
  for (ColumnID column_id{0}; column_id < segment_count; ++column_id) {
    std::shared_ptr<BaseSegment> segment_new;
    chunk_new->add_segment(segment_new);
    auto th = std::make_shared<std::thread>(
        [this, chunk_id, column_id, segment_new]() { this->_compress_segment(chunk_id, column_id, segment_new); });

    threads.push_back(std::move(th));
  }
  for (auto thread : threads) thread->join();

  _chunks[chunk_id] = chunk_new;
}

}  // namespace opossum
