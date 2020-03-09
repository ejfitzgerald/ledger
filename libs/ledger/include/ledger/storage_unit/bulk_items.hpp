#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/const_byte_array.hpp"

namespace fetch {
namespace ledger {

/**
 * This class holds a vector of T objects, allowing the user to
 * deserialize large arrays of the object and get the first one
 * immediately etc.
 */
class BulkItems
{
public:
  using ConstByteArray = fetch::byte_array::ConstByteArray;

  /**
   * push back to the underlying vector
   *
   * @param: T The object
   */
  template <typename T>
  void push_back(T const &item)
  {
    serializers::LargeObjectSerializeHelper serializer{};
    serializer << item;

    items_.emplace_back(std::move(serializer.data()));
  }

  /**
   * Get the object at a certain index. Not safe when
   * the index is greater than the array.
   *
   * @param: index The index to access
   *
   * @return: The object at that location
   */
  template <typename T>
  T Get(std::size_t index)
  {
    serializers::LargeObjectSerializeHelper serializer{items_[index]};

    T object;

    serializer >> object;

    return object;
  }

  /**
   * The number of elements in the array
   *
   * @return: The number of elements in the array
   */
  std::size_t size() const
  {
    return items_.size();
  }

  /**
   * Get all of the objects as a vector
   *
   * @return: The objects as a vector
   */
  template <typename T>
  std::vector<T> GetAll()
  {
    std::vector<T> ret;

    for(auto const &item : items_)
    {
      serializers::LargeObjectSerializeHelper serializer{item};

      T object;

      ret.emplace_back(std::move(object));
    }

   return ret;
  }

  std::vector<ConstByteArray> items_;
};

}  // namespace ledger

namespace serializers {

template <typename D>
struct MapSerializer<ledger::BulkItems, D>
{
public:
  using Type       = ledger::BulkItems;
  using DriverType = D;

  static uint8_t const PAYLOAD          = 1;

  template <typename Constructor>
  static void Serialize(Constructor &map_constructor, Type const &item)
  {
    auto map = map_constructor(1);

    map.Append(PAYLOAD, item.items_);

  }

  template <typename MapDeserializer>
  static void Deserialize(MapDeserializer &map, Type &item)
  {
    map.ExpectKeyGetValue(PAYLOAD, item.items_);
  }
};
}  // namespace serializers

}  // namespace fetch

