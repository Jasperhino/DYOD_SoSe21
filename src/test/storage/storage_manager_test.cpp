#include <memory>

#include "../base_test.hpp"
#include "gtest/gtest.h"

#include "../lib/storage/storage_manager.hpp"
#include "../lib/storage/table.hpp"

namespace opossum {

class StorageStorageManagerTest : public BaseTest {
 protected:
  void SetUp() override {
    auto& sm = StorageManager::get();
    t1 = std::make_shared<Table>();
    t2 = std::make_shared<Table>(4);

    sm.add_table("first_table", t1);
    sm.add_table("second_table", t2);
  }

  std::shared_ptr<Table> t1 = nullptr;
  std::shared_ptr<Table> t2 = nullptr;
};

TEST_F(StorageStorageManagerTest, AddTable) {
  auto& sm = StorageManager::get();
  auto t3 = std::make_shared<Table>();
  EXPECT_THROW(sm.add_table("first_table", t3), std::exception);
}

TEST_F(StorageStorageManagerTest, GetTable) {
  auto& sm = StorageManager::get();
  auto t3 = sm.get_table("first_table");
  auto t4 = sm.get_table("second_table");
  EXPECT_EQ(t3, t1);
  EXPECT_EQ(t4, t2);

  EXPECT_THROW(sm.get_table("third_table"), std::exception);
}

TEST_F(StorageStorageManagerTest, TableNames) {
  auto& sm = StorageManager::get();
  std::vector table_names = sm.table_names();
  EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "first_table") != table_names.end());
  EXPECT_TRUE(std::find(table_names.begin(), table_names.end(), "second_table") != table_names.end());
}

TEST_F(StorageStorageManagerTest, PrintTable) {
  auto& sm = StorageManager::get();
  std::ostringstream stream;
  sm.print(stream);
  std::string printed = stream.str();
  EXPECT_TRUE(printed.find("Table: second_table, Column Count: 0, Row Count: 0, Chunk Count: 1") != std::string::npos);
  EXPECT_TRUE(printed.find("Table: first_table, Column Count: 0, Row Count: 0, Chunk Count: 1") != std::string::npos);
}

TEST_F(StorageStorageManagerTest, DropTable) {
  auto& sm = StorageManager::get();
  sm.drop_table("first_table");
  EXPECT_THROW(sm.get_table("first_table"), std::exception);
  EXPECT_THROW(sm.drop_table("first_table"), std::exception);
}

TEST_F(StorageStorageManagerTest, ResetTable) {
  StorageManager::get().reset();
  auto& sm = StorageManager::get();
  EXPECT_THROW(sm.get_table("first_table"), std::exception);
}

TEST_F(StorageStorageManagerTest, DoesNotHaveTable) {
  auto& sm = StorageManager::get();
  EXPECT_EQ(sm.has_table("third_table"), false);
}

TEST_F(StorageStorageManagerTest, HasTable) {
  auto& sm = StorageManager::get();
  EXPECT_EQ(sm.has_table("first_table"), true);
}

}  // namespace opossum
