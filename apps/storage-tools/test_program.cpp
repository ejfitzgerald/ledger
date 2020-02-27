
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

#include "chain/transaction_builder.hpp"
#include "chain/transaction_serializer.hpp"
#include "core/byte_array/const_byte_array.hpp"
#include "core/digest.hpp"
#include "core/serializers/main_serializer.hpp"
#include "crypto/ecdsa.hpp"
#include "vectorise/threading/pool.hpp"
#include "storage/resource_mapper.hpp"
#include "crypto/identity.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using fetch::byte_array::ConstByteArray;
using fetch::crypto::ECDSASigner;
using fetch::chain::TransactionBuilder;
using fetch::chain::TransactionSerializer;
using fetch::chain::Address;
using fetch::threading::Pool;
using fetch::serializers::LargeObjectSerializeHelper;

using SignerPtr  = std::unique_ptr<ECDSASigner>;
using AddressPtr = std::unique_ptr<Address>;

int main(int , char **)
{

  while(true)
  {
    SignerPtr certificate        = std::make_unique<ECDSASigner>();
    certificate->GenerateKeys();

    std::cout << Address(fetch::crypto::Identity{certificate->public_key()}).display() << std::endl;
    std::cout << certificate->private_key().ToBase64() << std::endl;

    //ECDSASigner reconstructed{(certificate->private_key().ToBase64()).FromBase64()};
    ECDSASigner reconstructed{certificate->private_key()};

    std::cout << Address(fetch::crypto::Identity{reconstructed.public_key()}).display() << std::endl;
    std::cout << reconstructed.private_key().ToBase64() << std::endl;
    std::cout << "" << std::endl;


    if(certificate->private_key().ToBase64().FromBase64() != certificate->private_key())
    {
    }

    if(certificate->private_key().ToBase64().FromBase64() != certificate->private_key())
    {
      break;
    }

    if(certificate->private_key() != reconstructed.private_key())
    {
      break;
    }
  }

  return 0;
}
