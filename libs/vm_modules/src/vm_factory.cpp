//------------------------------------------------------------------------------
//
//   Copyright 2018-2019 Fetch.AI Limited
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

#include "vm_modules/vm_factory.hpp"

namespace fetch {
namespace vm_modules {

std::shared_ptr<fetch::vm::Module> VMFactory::GetModule(uint64_t enabled)
{
  auto module = std::make_shared<fetch::vm::Module>();

  // core modules
  if (MOD_CORE & enabled)
  {
    CreatePrint(*module);
    CreatePanic(*module);
    CreateToString(*module);
    CreateToBool(*module);

    StructuredData::Bind(*module);
    ByteArrayWrapper::Bind(*module);
    math::UInt256Wrapper::Bind(*module);
    SHA256Wrapper::Bind(*module);
  }

  // math modules
  if (MOD_MATH & enabled)
  {
    math::BindExp(*module);
    math::BindSqrt(*module);
    math::BindMath(*module);
  }

  // synergetic modules
  if (MOD_SYN & enabled)
  {
    BindBitShift(*module);
    BindBitwiseOps(*module);
  }

  // ml modules
  if (MOD_ML & enabled)
  {
    ml::BindML(*module);
  }

  return module;
}

VMFactory::Errors VMFactory::Compile(std::shared_ptr<fetch::vm::Module> const &module,
                                     std::string const &source, fetch::vm::Executable &executable)
{
  std::vector<std::string> errors;

  // generate the compiler from the module
  auto   compiler = std::make_shared<fetch::vm::Compiler>(module.get());
  vm::IR ir;

  // compile the source
  bool const compiled = compiler->Compile(source, "default", ir, errors);

  if (!compiled)
  {
    errors.emplace_back("Failed to compile.");
    return errors;
  }

  fetch::vm::VM vm(module.get());  // TODO(tfr): refactor such that IR is first made executable
  if (!vm.GenerateExecutable(ir, "default_ir", executable, errors))
  {
    return errors;
  }

#ifndef NDEBUG
  std::ostringstream all_errors;
  for (auto const &error : errors)
  {
    all_errors << error << std::endl;
  }

  if (!errors.empty())
  {
    FETCH_LOG_WARN("VM_FACTORY", "Found badly constructed SC. Debug:\n", all_errors.str());
  }
#endif

  return errors;
}

}  // namespace vm_modules
}  // namespace fetch
