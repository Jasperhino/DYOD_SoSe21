#include "table_scan.hpp"

#include <algorithm>
#include <boost/core/typeinfo.hpp>

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
std::function<bool(T&)> _make_scan_type_lambda(T search_value, const ScanType scan_type) {
  switch(scan_type){
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
  return nullptr;
}

std::shared_ptr<const Table> TableScan::_on_execute() {
  auto in = _left_input_table();

  std::shared_ptr<Table> out = std::make_shared<Table>();
  for (auto column_id = ColumnID{0}, column_count = static_cast<ColumnID>(in->column_count()); column_id < column_count;
       ++column_id) {
    out->add_column_definition(in->column_name(column_id), in->column_type(column_id));
  }

  auto column_type = in->column_type(_column_id);
  DebugAssert(column_type == boost::core::demangled_name(_search_value.type()),
              "Incompatible data types for column and search value");


  resolve_data_type(column_type, [&](auto type) {
    using Type = typename decltype(type)::type;

    Type search_value = type_cast<Type>(_search_value);

    auto scan_type_lambda = _make_scan_type_lambda(search_value, _scan_type);


    for (auto chunk_id = ChunkID{0}, chunk_count = in->chunk_count(); chunk_id < chunk_count; ++chunk_id) {
      auto segment = in->get_chunk(chunk_id).get_segment(_column_id);
      auto new_chunk = std::make_shared<Chunk>();

      const auto typed_segment = std::dynamic_pointer_cast<ValueSegment<Type>>(segment);

      auto position_list = std::make_shared<PosList>();
      auto values = typed_segment->values();
      auto iter = values.begin();

      while ((iter = std::find_if(iter, values.end(), scan_type_lambda)) != values.end()) {
        auto position = std::distance(values.begin(), iter);
        position_list->emplace_back(RowID{chunk_id, static_cast<ChunkOffset>(position)});
        iter++;
      }

      for (auto column_id = ColumnID{0}, column_count = static_cast<ColumnID>(in->column_count());
           column_id < column_count; ++column_id) {
        auto reference_segment = std::make_shared<ReferenceSegment>(in, column_id, position_list);
        new_chunk->add_segment(reference_segment);
      }

      out->emplace_chunk(new_chunk);
    }
  });

  return out;
}

}  // namespace opossum
