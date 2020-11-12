#include "storage/new_revertible_document_store.hpp"

#include <iostream>

using fetch::storage::NewRevertibleDocumentStore;
using fetch::byte_array::ConstByteArray;

int main()
{
  // open the default state database for lane 0
  NewRevertibleDocumentStore document_store{};
  document_store.Load("node_storage_lane000_state.db", "node_storage_lane000_state_deltas.db",
                      "node_storage_lane000_state_index.db",
                      "node_storage_lane000_state_index_deltas.db", false);

  // iterate through the values and generate some JSON compatible outputs
  std::cout << "[\n";
  std::size_t i = 0;
  document_store.IterateThrough([&i](ConstByteArray const &key, ConstByteArray const &value) {
    if (i > 0)
    {
      std::cout << ",\n";
    }

    std::cout << R"(  {"key": ")" << key.ToBase64() << R"(", "value": ")" << value.ToBase64() << "\"}";

    ++i;
  });
  std::cout << "\n]" << std::endl;

  return 0;
}