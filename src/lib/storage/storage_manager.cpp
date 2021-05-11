#include "storage_manager.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "utils/assert.hpp"

namespace opossum {

StorageManager& StorageManager::get() {
  static StorageManager instance;
  return instance;
}

void StorageManager::add_table(const std::string& name, std::shared_ptr<Table> table) {
  Assert(!has_table(name), "Table " + name + " exists already.");
  _tables[name] = table;
}

void StorageManager::drop_table(const std::string& name) {
  Assert(has_table(name), "Cannot drop non-existing table " + name);
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
    out << "Table: " << table_name << ", Column Count: " << table->column_count()
        << ", Row Count: " << table->row_count() << ", Chunk Count: " << table->chunk_count() << std::endl;
  }
}

void StorageManager::reset() { get() = StorageManager{}; }

}  // namespace opossum
