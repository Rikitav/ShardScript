## ​🛠 Creating Native Modules
​You can extend ShardScript's functionality using the FrameworkModule mechanism, which links declarations in .ss files with C++ implementations.

### ​1. API Declaration (ShardScript)
​Create a file (e.g., MyClass.ss) where you declare your class and methods with the extern modifier. This tells the compiler that the implementation will be provided externally.
```
namespace Rikitav.Example
{
    public class MyClass
    {
        // Mark the method as extern
        public extern void NativeLog(string message);
    }
}
```

### 2. Implementation (C++)
​Create a class inheriting from shard::FrameworkModule. You need to implement methods to bind symbols (Bind...) and return the source code.
```
#include <shard/framework/FrameworkModule.h>
#include <shard/runtime/ObjectInstance.h>
#include <shard/runtime/ConsoleHelper.h>

using namespace shard;

class MyModule : public FrameworkModule
{
    // Native method implementation
    static ObjectInstance* Impl_NativeLog(const MethodSymbol* symbol, InboundVariablesContext* args)
    {
        // Get argument by name
        ObjectInstance* msgInstance = args->Variables.at(L"message");
        std::wstring text = msgInstance->AsString();
        
        // Logic
        std::wcout << L"[Native]: " << text << std::endl;

        return nullptr; // void
    }

public:
    // 1. Provide the source code (.ss)
    SourceReader* GetSource() override
    {
        // You can load this from resources or a raw string
        return new StringStreamReader(L"MyClass.ss", L"namespace Rikitav.Example { ... }");
    }

    // 2. Bind methods
    bool BindMethod(MethodSymbol* symbol) override
    {
        if (symbol->Name == L"NativeLog")
        {
            symbol->FunctionPointer = Impl_NativeLog;
            return true;
        }
        return false;
    }

    // Stubs for constructors/properties if not needed
    bool BindConstructor(ConstructorSymbol* symbol) override { return false; }
    bool BindAccessor(AccessorSymbol* symbol) override { return false; }
};
```

### 3. Registration
To enable the module, register its instance during the DLL linking :
```
case DLL_PROCESS_ATTACH:
{
    FrameworkLoader::AddModule(new MyModule());
    break;
}
```

### 4. Runtime
If you do everything right, and placed your DLL in the same folder as interpreter, you should be able access you types and functions inside ShardScript's runtime
```
using Rikitav.Example;

namespace Shard
{
    public class Script
    {
        public static void Main()
        {
            MyClass.NativeLog("Hello, World!");
        }
    }
}
```