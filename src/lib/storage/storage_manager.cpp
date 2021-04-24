#include "storage_manager.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <algorithm>

#include "utils/assert.hpp"

namespace opossum {

StorageManager& StorageManager::get() {
  static StorageManager instance;
  return instance;
}

void StorageManager::add_table(const std::string& name, std::shared_ptr<Table> table) { _tables[name] = table; }

void StorageManager::drop_table(const std::string& name) {
  DebugAssert(has_table(name), "cannot drop non-existing table");
  _tables.erase(name);
}

std::shared_ptr<Table> StorageManager::get_table(const std::string& name) const { return _tables.at(name); }

bool StorageManager::has_table(const std::string& name) const { return _tables.find(name) != _tables.end(); }

std::vector<std::string> StorageManager::table_names() const {
  std::vector<std::string> names;
  std::transform(_tables.begin(), _tables.end(), std::back_inserter(names), [](auto entry) { return entry.first; });
  return names;
}

void StorageManager::print(std::ostream& out) const {
  for (auto const& [table_name, table] : _tables) {
    out << table_name << ", " << table->column_count() << ", " << table->row_count() << ", " << table->chunk_count()
        << std::endl;
  }
}

void StorageManager::reset() { _tables.clear(); }

}  // namespace opossum
