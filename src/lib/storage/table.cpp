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
  _target_chunk_size = target_chunk_size;
  create_new_chunk();
}

void Table::_add_segment_to_chunk(std::shared_ptr<Chunk>& chunk, const std::string& type) {
  resolve_data_type(type, [&](const auto data_type_t) {
    using ColumnDataType = typename decltype(data_type_t)::type;
    const auto value_segment = std::make_shared<ValueSegment<ColumnDataType>>();
    chunk->add_segment(value_segment);
  });
}

void Table::add_column_definition(const std::string& name, const std::string& type) {
  Assert(!row_count(), "column must be added before adding entries");
  _column_names.push_back(name);
  _column_types.push_back(type);
  _name_id_mapping[name] = static_cast<ColumnID>(_column_names.size() - 1);
}

void Table::add_column(const std::string& name, const std::string& type) {
  add_column_definition(name, type);
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

void Table::create_new_chunk() { _chunks.push_back(std::make_shared<Chunk>()); }

// TODO(we): We changed signature here
void Table::emplace_chunk(std::shared_ptr<Chunk> chunk) {
  if (_chunks.front()->size() == 0) {
    _chunks[0] = chunk;
  } else {
    _chunks.push_back(chunk);
  }
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

void Table::compress_chunk(ChunkID chunk_id) {
  auto& chunk_old = get_chunk(chunk_id);
  // lock chunk before compressing (prevents appending to this chunk)
  std::shared_lock lock(chunk_old.get_mutex());

  auto chunk_new = std::make_shared<Chunk>();

  std::vector<std::thread> threads;
  ColumnCount segment_count = chunk_old.column_count();
  threads.reserve(segment_count);

  for (ColumnID column_id{0}; column_id < segment_count; ++column_id) {
    std::shared_ptr<BaseSegment> segment_empty;
    chunk_new->add_segment(segment_empty);

    auto thread = std::thread([&, column_id]() {
      resolve_data_type(column_type(column_id), [&](auto type) {
        using Type = typename decltype(type)::type;

        auto segment_old = chunk_old.get_segment(column_id);
        auto segment_new = std::make_shared<DictionarySegment<Type>>(segment_old);
        chunk_new->replace_segment(column_id, segment_new);
      });
    });
    threads.push_back(std::move(thread));
  }

  for (auto& thread : threads) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  _chunks[chunk_id] = chunk_new;
}

}  // namespace opossum
