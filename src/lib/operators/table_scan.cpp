#include "table_scan.hpp"

#include <algorithm>
#include <stdexcept>

#include "resolve_type.hpp"
#include "storage/reference_segment.hpp"
#include "storage/value_segment.hpp"
#include "type_cast.hpp"
#include "utils/assert.hpp"

namespace opossum {

TableScan::TableScan(const std::shared_ptr<const AbstractOperator>& in, const ColumnID column_id,
                     const ScanType scan_type, const AllTypeVariant search_value)
    : AbstractOperator(in), _column_id(column_id), _scan_type(scan_type), _search_value(search_value) {}

ColumnID TableScan::column_id() const { return _column_id; }

ScanType TableScan::scan_type() const { return _scan_type; }

const AllTypeVariant& TableScan::search_value() const { return _search_value; }

template <typename T>
std::function<bool(T&)> _make_scan_type_lambda(T& search_value, const ScanType scan_type) {
  switch (scan_type) {
    case ScanType::OpEquals:
      return [search_value](T& v) { return v == search_value; };
    case ScanType::OpNotEquals:
      return [search_value](T& v) { return v != search_value; };
    case ScanType::OpLessThan:
      return [search_value](T& v) { return v < search_value; };
    case ScanType::OpLessThanEquals:
      return [search_value](T& v) { return v <= search_value; };
    case ScanType::OpGreaterThan:
      return [search_value](T& v) { return v > search_value; };
    case ScanType::OpGreaterThanEquals:
      return [search_value](T& v) { return v >= search_value; };
  }
  Assert(false, "Unsupported ScanType.");
}

template <typename T>
void _scan_reference_segment(const std::shared_ptr<PosList> position_list, ChunkID chunk_id,
                             std::shared_ptr<ReferenceSegment> segment, std::function<bool(T&)> scan_type_lambda) {
  const auto referenced_pos_list = segment->pos_list();
  for (ChunkOffset reference_index{0}, segment_size = segment->size(); reference_index < segment_size;
       ++reference_index) {
    T value = type_cast<T>((*segment)[reference_index]);
    if (scan_type_lambda(value)) {
      position_list->push_back((*referenced_pos_list)[reference_index]);
    }
  }
}

template <typename T>
void _scan_value_segment(const std::shared_ptr<PosList> position_list, ChunkID chunk_id,
                         std::shared_ptr<ValueSegment<T>> typed_segment, std::function<bool(T&)> scan_type_lambda) {
  auto values = typed_segment->values();
  auto iter = values.begin();

  while ((iter = std::find_if(iter, values.end(), scan_type_lambda)) != values.end()) {
    auto position = std::distance(values.begin(), iter);
    position_list->emplace_back(RowID{chunk_id, static_cast<ChunkOffset>(position)});
    iter++;
  }
}

template <typename T>
void _scan_dictionary_segment(const std::shared_ptr<PosList> position_list, ChunkID chunk_id,
                              std::shared_ptr<DictionarySegment<T>> typed_segment, ScanType scan_type,
                              const T search_value) {
  ValueID lower_bound = typed_segment->lower_bound(search_value);
  ValueID upper_bound = typed_segment->upper_bound(search_value);
  ValueID search_value_id = INVALID_VALUE_ID;

  switch (scan_type) {
    case ScanType::OpEquals:
    case ScanType::OpNotEquals:
      if (lower_bound != upper_bound) {
        // value does appear in dictionary
        search_value_id = lower_bound;
      }
      break;
    case ScanType::OpGreaterThanEquals:
      search_value_id = lower_bound;
      break;
    case ScanType::OpGreaterThan:
      search_value_id = upper_bound;
      scan_type = ScanType::OpGreaterThanEquals;
      break;
    case ScanType::OpLessThanEquals:
      search_value_id = lower_bound;
      if (lower_bound == upper_bound) {
        // value does not appear in dictionary
        scan_type = ScanType::OpLessThan;
      }
      break;
    case ScanType::OpLessThan:
      search_value_id = lower_bound;
      break;
  }

  auto scan_type_lambda = _make_scan_type_lambda<ValueID>(search_value_id, scan_type);

  auto attribute_vector = typed_segment->attribute_vector();
  for (size_t attribute_index = 0, attribute_count = attribute_vector->size(); attribute_index < attribute_count;
       ++attribute_index) {
    ValueID value_id = attribute_vector->get(attribute_index);
    if (scan_type_lambda(value_id)) {
      position_list->emplace_back(RowID{chunk_id, static_cast<ChunkOffset>(attribute_index)});
    }
  }
}

std::shared_ptr<const Table> TableScan::_on_execute() {
  auto input_table = _left_input_table();

  std::shared_ptr<Table> output_table = std::make_shared<Table>();
  for (auto column_id = ColumnID{0}, column_count = static_cast<ColumnID>(input_table->column_count());
       column_id < column_count; ++column_id) {
    output_table->add_column_definition(input_table->column_name(column_id), input_table->column_type(column_id));
  }

  if (input_table->row_count() == 0) return output_table;

  auto column_type_name = input_table->column_type(_column_id);

  resolve_data_type(column_type_name, [&](auto type) {
    using Type = typename decltype(type)::type;

    Type search_value;
    try {
      search_value = type_cast<Type>(_search_value);
    } catch (...) {
      throw std::invalid_argument("Incompatible data types for column and search value");
    }

    auto scan_type_lambda = _make_scan_type_lambda(search_value, _scan_type);

    for (auto chunk_id = ChunkID{0}, chunk_count = input_table->chunk_count(); chunk_id < chunk_count; ++chunk_id) {
      auto segment = input_table->get_chunk(chunk_id).get_segment(_column_id);
      auto position_list = std::make_shared<PosList>();
      std::shared_ptr<const Table> referenced_table;

      const auto typed_value_segment = std::dynamic_pointer_cast<ValueSegment<Type>>(segment);
      if (typed_value_segment) {
        _scan_value_segment<Type>(position_list, chunk_id, typed_value_segment, scan_type_lambda);
        referenced_table = input_table;
      } else {
        const auto typed_dictionary_segment = std::dynamic_pointer_cast<DictionarySegment<Type>>(segment);
        if (typed_dictionary_segment) {
          _scan_dictionary_segment<Type>(position_list, chunk_id, typed_dictionary_segment, _scan_type, search_value);
          referenced_table = input_table;
        } else {
          const auto reference_segment = std::dynamic_pointer_cast<ReferenceSegment>(segment);
          if (reference_segment) {
            _scan_reference_segment<Type>(position_list, chunk_id, reference_segment, scan_type_lambda);
            referenced_table = reference_segment->referenced_table();
          } else {
            throw std::invalid_argument("Segment has unsupported type.");
          }
        }
      }

      auto new_chunk = std::make_shared<Chunk>();
      for (auto column_id = ColumnID{0}, column_count = static_cast<ColumnID>(input_table->column_count());
           column_id < column_count; ++column_id) {
        auto reference_segment = std::make_shared<ReferenceSegment>(referenced_table, column_id, position_list);
        new_chunk->add_segment(reference_segment);
      }

      output_table->emplace_chunk(new_chunk);
    }
  });

  return output_table;
}

}  // namespace opossum