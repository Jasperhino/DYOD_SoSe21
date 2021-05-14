#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "abstract_operator.hpp"
#include "all_type_variant.hpp"
#include "storage/table.hpp"
#include "types.hpp"
#include "utils/assert.hpp"

namespace opossum {

class BaseTableScanImpl;
class Table;

class TableScan : public AbstractOperator {
 public:
  TableScan(const std::shared_ptr<const AbstractOperator>& in, const ColumnID column_id, const ScanType scan_type,
            const AllTypeVariant search_value);

  ColumnID column_id() const;
  ScanType scan_type() const;
  const AllTypeVariant& search_value() const;

 protected:
  std::shared_ptr<const Table> _on_execute() override;

  ColumnID _column_id;
  ScanType _scan_type;
  const AllTypeVariant _search_value;

 private:
  static bool _scan_type_equals(AllTypeVariant&, const AllTypeVariant&);
  //static bool _scan_type_not_equals(AllTypeVariant&, AllTypeVariant&);
  static bool _scan_type_less_than(AllTypeVariant&, const AllTypeVariant&);
  //static bool _scan_type_less_than_equals(AllTypeVariant&, AllTypeVariant&);
  static bool _scan_type_greater_than(AllTypeVariant&, const AllTypeVariant&);
  //static bool _scan_type_greater_than_equals(AllTypeVariant&, AllTypeVariant&);
};

}  // namespace opossum
