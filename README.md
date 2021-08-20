# Table of Contents

* [Overview](#overview)
* [Quick Start](#quick-start)
    - [Setting and Getting Variables](#setting-and-getting-variables)
    - [Processing Tables](#processing-tables)
    - [Calling Functions](#calling-functions)
    - [Exporting and Using User-defined Types](#exporting-and-using-user-defined-types)
* [Classes and APIs](#classes-and-apis)
    - [LuaRefObject](#luarefobject)
    - [LuaObject](#luaobject)
    - [LuaTable](#luatable)
    - [LuaFunction](#luafunction)
    - [LuaClass](#luaclass)
    - [LuaUserData](#luauserdata)
    - [LuaState](#luastate)
* [FAQ](#faq)
* [License](#license)

-----

# Overview

`lua-cpp` is a C++ library aiming at simplifying the use of Lua API. It is compatible with Lua 5.2.3(or above) and needs C++14 support.

[[back to top](#table-of-contents)]

-----

# Building from Source

Prerequisites

* GCC >= 4.9 with c++14 support
* CMake >= 3.10

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DLUA_INCLUDE_DIRS=/path/to/lua/include/dir -DLUA_LIBRARIES=/path/to/lua/libs ..
make -j
```

[[back to top](#table-of-contents)]

-----

# Quick Start

This section is a brief introduction of some APIs. Examples can also be found in `test.cpp`. All available classes and functions are listed in Section [Classes and APIs](#classes-and-apis).

## Setting and Getting Variables

Let's start with a simple example, the famous `Hello, world!` program:

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    l.Set("msg", "Hello, luacpp from ouonline!");
    auto lobj = l.Get("msg");
    if (lobj.GetType() == LUA_TSTRING) {
        auto str_ref = lobj.ToString();
        cout << "get msg -> " << string(str_ref.base, str_ref.size) << endl;
    } else {
        cerr << "unknown object type -> " << lobj.GetTypeStr() << endl;
    }

    return 0;
}
```

In this example, we set the variable `msg`'s value to be a string "Hello, luacpp from ouonline!", then use the getter function to fetch its value and print it.

`LuaState::Set()`s are a series of overloaded functions, which can be used to set up various kinds of variables. Once the variable is set, its value is kept in the `LuaState` instance until it is modified again or the `LuaState` instance is destroyed.

`LuaState::Get()` is used to get a variable by its name. The value returned by `LuaState::Get()` is a `LuaObject` instance, which can be converted to the proper type later. You'd better check the return value of `LuaObject::GetType()` before any conversions.

[[back to top](#table-of-contents)]

## Processing Tables

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    auto iterfunc = [] (const LuaObject& key, const LuaObject& value) -> bool {
        cout << "    "; // indention
        if (key.GetType() == LUA_TNUMBER) {
            cout << key.ToNumber();
        } else if (key.GetType() == LUA_TSTRING) {
            auto str_ref = key.ToString();
            cout << string(str_ref.base, str_ref.size);
        } else {
            cout << "unsupported key type -> " << key.GetTypeStr() << endl;
            return false;
        }

        if (value.GetType() == LUA_TNUMBER) {
            cout << " -> " << value.ToNumber() << endl;
        } else if (value.GetType() == LUA_TSTRING) {
            auto str_ref = value.ToString();
            cout << " -> " << string(str_ref.base, str_ref.size) << endl;
        } else {
            cout << " -> unsupported iter value type: " << value.GetTypeStr() << endl;
        }

        return true;
    };

    cout << "table1:" << endl;
    l.DoString("var = {'mykey', value = 'myvalue', others = 'myothers'}");
    l.Get("var").ToTable().ForEach(iterfunc);

    cout << "table2:" << endl;
    auto ltable = l.CreateTable();
    ltable.Set("x", 5);
    ltable.Set("o", "ouonline");
    ltable.Set("t", ltable);
    ltable.ForEach(iterfunc);

    return 0;
}
```

At first we use `LuaState::DoString()` to execute a chunk that creates a table named `var` with 3 fields.

`LuaTable::ForEach()` takes a callback function that is used to iterate each key-value pair in the table. If the callback function returns `false`, `LuaTable::ForEach()` exits and returns `false`.

We can use `LuaState::CreateTable()` to create a new empty table and use `LuaState::Set()` to set fields of this table. Like `LuaState::Set()`, `LuaTable::Set()`s are a series of overloaded functions used to handle various kinds of data types.

[[back to top](#table-of-contents)]

## Calling Functions

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    auto resiter1 = [] (int, const LuaObject& lobj) -> bool {
        auto str_ref = lobj.ToString();
        cout << "output from resiter1: " << string(str_ref.base, str_ref.size) << endl;
        return true;
    };
    auto resiter2 = [] (int n, const LuaObject& lobj) -> bool {
        cout << "output from resiter2: ";
        if (n == 0) {
            cout << lobj.ToNumber() << endl;
        } else if (n == 1) {
            auto str_ref = lobj.ToString();
            cout << string(str_ref.base, str_ref.size) << endl;
        }

        return true;
    };

    std::function<int (const char*)> Echo = [] (const char* msg) -> int {
        cout << "in std::function Echo(str): '" << msg << "'" << endl;
        return 5;
    };
    l.Set("Echo", Echo);
    auto lfunc = l.Get("Echo").ToFunction();

    l.Set("msg", "calling cpp function with return value from cpp: ");
    lfunc.Exec(resiter1, nullptr, l.Get("msg"));

    l.DoString("res = Echo('calling cpp function with return value from lua: ');"
               "io.write('return value -> ', res, '\\n')");

    l.DoString("function return2(a, b) return a, b end");
    l.Get("return2").ToFunction().Exec(resiter2, nullptr, 5, "ouonline");

    return 0;
}
```

First we define a C++ function `echo` and set its name to be `echo` in the Lua environment using `LuaState::CreateFunction()`. `LuaFunction::Exec()` invokes the real function. See [LuaFunction](#luafunction) for more details.

[[back to top](#table-of-contents)]

## Exporting and Using User-defined Types

First we define a class `ClassDemo` for test:

```c++
class ClassDemo final {
public:
    ClassDemo() {
        cout << "ClassDemo::ClassDemo() is called without value." << endl;
    }
    ClassDemo(const char* msg, int x) {
        cout << "ClassDemo::ClassDemo() is called with string -> '"
             << m_msg << "' and int -> " << x << "." << endl;

        if (msg) {
            m_msg = msg;
        }
    }
    ~ClassDemo() {
        cout << "ClassDemo::~ClassDemo() is called." << endl;
    }

    void Set(const char* msg) {
        cout << "ClassDemo()::Set(msg): '" << msg << "'" << endl;
        m_msg = msg;
    }
    void Print() const {
        cout << "ClassDemo::Print(msg): '" << m_msg << "'" << endl;
    }
    void Echo(const char* msg) const {
        cout << "ClassDemo::Echo(string): '" << msg << "'" << endl;
    }
    void Echo(int v) const {
        cout << "ClassDemo::Echo(int): " << v << endl;
    }
    static void StaticEcho(const char* msg) {
        cout << "ClassDemo::StaticEcho(string): '" << msg << "'" << endl;
    }

private:
    string m_msg;
};
```

Then we see how to export this class to the Lua environment.

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    auto lclass = l.RegisterClass<ClassDemo>("ClassDemo");

    cout << "--------------------------------------------" << endl;
    lclass.SetConstructor();
    l.DoString("tc = ClassDemo()");

    cout << "--------------------------------------------" << endl;
    lclass.SetConstructor<const char*, int>();
    l.DoString("tc = ClassDemo('ouonline', 5)");

    cout << "--------------------------------------------" << endl;
    lclass.SetMemberFunction("print", &ClassDemo::Print)
        .SetMemberFunction<void, const char*>("echo_str", &ClassDemo::Echo) // overloaded function
        .SetMemberFunction<void, int>("echo_int", &ClassDemo::Echo);
    l.DoString("tc = ClassDemo('ouonline', 5); tc:print();"
               "tc:echo_str('calling class member function from lua')");

    cout << "--------------------------------------------" << endl;
    lclass.SetStaticFunction("s_echo", &ClassDemo::StaticEcho);
    l.DoString("ClassDemo:s_echo('static member function is called without being instantiated');"
               "tc = ClassDemo(); tc:s_echo('static member function is called by an instance')");

    cout << "--------------------------------------------" << endl;

    return 0;
}
```

`LuaState::RegisterClass()` is used to export user-defined classes to Lua. It requires a string `name` as the class's name in the Lua environment, and adds a default constructor and a destructor for this class. You can register different names with the same c++ class.

`LuaClass::SetMemberFunction()` is a template function used to export member functions for this class. `LuaClass::SetStaticFunction()` is used to export static member functions. Both member functions and staic member functions can be C-style functions and `std::function`s.

Class member functions should be called with colon operator in the form of `object:func()`, to ensure the object itself is the first argument passed to `func`. Otherwise you need to do it manually, like `object.func(object, <other arguments>)`.

The following program displays how to use `LuaUserData` to exchange data between C++ and Lua.

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    l.RegisterClass<ClassDemo>("ClassDemo")
        .SetConstructor<const char*, int>()
        .SetMemberFunction("print", &ClassDemo::Print);
    l.CreateUserData("ClassDemo", "tc", "ouonline", 5).Get<ClassDemo>()->Set("in lua: Print test data from cpp");
    l.DoString("tc:print()");

    return 0;
}
```

We create a `LuaUserData` object of type `ClassDemo` by calling `LuaClass::CreateUserData()` and set its name to be `tc` in the Lua environment. Then we set its content to be a string, which is printed by calling `LuaState::DoString()`.

The following example is similar to the previous one, except that we modify the object's content in the Lua environment but print it in C++.

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    l.RegisterClass<ClassDemo>("ClassDemo")
        .SetConstructor<const char*, int>()
        .SetMemberFunction("set", &ClassDemo::Set);
    l.DoString("tc = ClassDemo('ouonline', 3); tc:set('in cpp: Print test data from lua')");
    l.Get("tc").ToUserData().Get<ClassDemo>()->Print();

    return 0;
}
```

[[back to top](#table-of-contents)]

-----

# Classes and APIs

This section describes all classes and functions provided by `lua-cpp`.

## LuaRefObject

`LuaRefObject` is an internal base class for various Lua types.

```c++
LuaRefObject(const std::shared_ptr<lua_State>& l, int index);
```

Creates a `LuaRefObject` with the object located in `index` of the lua_State `l`.

```c++
int GetType() const;
```

Gets the type of this object. The return value is one of the following: `LUA_TNIL`, `LUA_TNUMBER`, `LUA_TBOOLEAN`, `LUA_TSTRING`, `LUA_TTABLE`, `LUA_TFUNCTION`, `LUA_TUSERDATA`, `LUA_TTHREAD`, and `LUA_TLIGHTUSERDATA`.

```c++
const char* GetTypeStr() const;
```

Returns the name of the object's type.

```c++
void PushSelf() const;
```

Pushes self to the top of the stack. This function is usually used by internal functions.

[[back to top](#table-of-contents)]

## LuaObject

`LuaObject`(inherits from `LuaRefObject`) represents an arbitrary item of Lua. You are expected to check the return value of `GetType()` before any of the following functions is called.

```c++
LuaObject(const std::shared_ptr<lua_State>& l, int index) : LuaRefObject(l, index) {}
```

Creates a `LuaObject` with the object located in `index` of the lua_State `l`.

```c++
bool IsNil() const;
```

Tells whether this object is `nil`.

```c++
bool IsBool() const;
```

Tells whether this object is a `bool` value.

```c++
bool IsNumber() const;
```

Tells whether this object is a number.

```c++
bool IsString() const;
```

Tells whether this object is a string.

```c++
bool IsTable() const;
```

Tells whether this object is a table.

```c++
bool IsFunction() const;
```

Tells whether this object is a function.

```c++
bool IsUserData() const;
```

Tells whether this object is a userdata.

```c++
bool IsThread() const;
```

Tells whether this object is a thread.

```c++
bool IsLightUserData() const;
```

Tells whether this object is a light userdata.

```c++
bool ToBool() const;
```

Converts this object to a `bool` value.

```c++
LuaStringRef ToString() const;
```

Converts this object to a `LuaStringRef` object, which only contains address and size of the string.

```c++
template <typename T>
T ToInteger() const;
```

Converts this object to an integer.

```c++
lua_Number ToNumber() const;
```

Converts this object to a (floating point) number.

```c++
LuaTable ToTable() const;
```

Converts this object to a `LuaTable` object.

```c++
LuaFunction ToFunction() const;
```

Converts this object to a `LuaFunction` object.

```c++
LuaUserData ToUserData() const;
```

Converts this object to a `LuaUserData` object.

[[back to top](#table-of-contents)]

## LuaTable

`LuaTable`(inherits from `LuaRefObject`) represents the table type of Lua.

```c++
LuaTable(const std::shared_ptr<lua_State>& l, int index);
```

Creates a `LuaTable` with the table located in `index` of the lua_State `l`.

```c++
LuaObject Get(int index) const;
LuaObject Get(const char* name) const;
```

Gets an object by its `index` or `name` in this table.

```c++
void Set(int index, const char* str);
void Set(const char* name, const char* str);
```

Sets the value at `index` or `name` to be the string `str`.

```c++
void Set(int index, const char* str, size_t len);
void Set(const char* name, const char* str, size_t len)
```

Sets the value at `index` or `name` to be the string `str` with length `len`.

```c++
void Set(int index, lua_Number n);
void Set(const char* name, lua_Number n);
```

Sets the value at `index` or `name` to be the number `n`.

```c++
void Set(int index, const LuaRefObject& lobj);
void Set(const char* name, const LuaRefObject& lobj)
```

Sets the value at `index` or `name` to be the object `lobj`. Note that `lobj` and this table **MUST** be created by the same LuaState.

```c++
bool ForEach(const std::function<bool (const LuaObject& key,
                                       const LuaObject& value)>& func) const;
```

Itarates the table with the callback function `func`.

[[back to top](#table-of-contents)]

## LuaFunction

`LuaFunction`(inherits from `LuaRefObject`) represents the function type of Lua.

```c++
LuaFunction(const std::shared_ptr<lua_State>& l, size_t index);
```

Creates a `LuaFunction` with the table located in `index` of the lua_State `l`.

```c++
template <typename... Argv>
bool Exec(const std::function<bool (int i, const LuaObject&)>& callback = nullptr,
          std::string* errstr = nullptr, Argv&&... argv);
```

Invokes the function with arguments `argv`. `callback` is a callback function used to handle result(s). Note that the first argument `i` of `callback` counts from 0. `errstr` is a string to receive a message if an error occurs. The rest of arguments `argv`, if any, are passed to the real function being called.

[[back to top](#table-of-contents)]

## LuaClass

`LuaClass`(inherits from `LuaRefObject`) is used to export C++ classes and member functions to Lua. It does not support exporting member variables.

```c++
LuaClass(const std::shared_ptr<lua_State>& lp, int index, int gc_table_ref);
```

Creates a `LuaClass` with the table located in `index` of the lua_State `l`. `gc_table_ref` is a reference created by `luaL_ref` and points to a metatable which is used to destroy `DestructorObject`.

```c++
template<typename... FuncArgType>
LuaClass<T>& SetConstructor();
```

Sets the class's constructor with argument type `FuncArgType` and return a reference of the class itself.

```c++
/** member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& SetMemberFunction(const char* name, FuncRetType (T::*f)(FuncArgType...));

/** member function with const qualifier */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& SetMemberFunction(const char* name, FuncRetType (T::*f)(FuncArgType...) const);

/** std::function member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& SetMemberFunction(const char* name, const std::function<FuncRetType (FuncArgType...)>& f);

/** c-style member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& SetMemberFunction(const char* name, FuncRetType (*f)(FuncArgType...));

/** lua-style member function */
LuaClass<T>& SetMemberFunction(const char* name, int (*f)(lua_State* l));
```

Exports function `f` to be a member function of this class and `name` as the exported function name in Lua.

```c++
/** std::function static member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& SetStaticFunction(const char* name, const std::function<FuncRetType (FuncArgType...)>& f);

/** C-style static member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& SetStaticFunction(const char* name, FuncRetType (*f)(FuncArgType...));

/** lambda static member function */
template <typename FuncType>
LuaClass<T>& SetStaticFunction(const char* name, const FuncType& f);

/** lua-style static member function */
LuaClass<T>& SetStaticFunction(const char* name, int (*f)(lua_State* l));
```

Exports function `f` to be a static member function of this class and `name` as the exported function name in Lua.

[[back to top](#table-of-contents)]

## LuaUserData

`LuaUserData` (inherits from `LuaRefObject`) represents the userdata type in Lua.

```c++
LuaUserData(const std::shared_ptr<lua_State>& l, int index);
```

Creates a `LuaUserData` with the object located in `index` of the lua_State `l`.

```c++
template<typename T>
T* Get() const;
```

Gets the real data of type `T`.

[[back to top](#table-of-contents)]

## LuaState

```c++
LuaState();
```

The constructor.

```c++
lua_State* GetRawPtr() const;
```

Returns the `lua_State` pointer.

```c++
LuaObject Get(const char* name) const;
```

Gets an object by its name.

```c++
void Set(const char* name, const char* str);
```

Sets the variable `name` to be the string `str`.

```c++
void Set(const char* name, const char* str, size_t len);
```

Sets the variable `name` to be the string `str` with length `len`.

```c++
void Set(const char* name, lua_Number n);
```

Sets the variable `name` to be the number `n`.

```c++
void Set(const char* name, const LuaRefObject& lobj);
```

Sets the variable `name` to be the object `lobj`. Note that `lobj` **MUST** be created by this LuaState.

```c++
LuaTable CreateTable(const char* name = nullptr);
```

Creates a new table with table name `name`(if not NULL).

```c++
/** c-style function */
template <typename FuncRetType, typename... FuncArgType>
void CreateFunction(const char* name, FuncRetType (*f)(FuncArgType...));

/** std::function */
template <typename FuncRetType, typename... FuncArgType>
void CreateFunction(const char* name, const std::function<FuncRetType (FuncArgType...)>& f);

/** lambda function */
template <typename FuncType>
void CreateFunction(const char* name, const FuncType& f);
```

Creates a function object from `f` with `name`.

```c++
template<typename T>
LuaClass<T> RegisterClass(const char* name);
```

Exports a new type `T` with the name `name`. If `name` is already exported, the class is returned.

```c++
template<typename... Argv>
LuaUserData CreateUserData(const char* classname, const char* name = nullptr, Argv&&... argv);
```

Creates a `LuaUserData` of type `classname` with the name `name`. The arguments `argv` are passed to the constructor of `T` to create an instance. If `classname` is not exported, a `nil` object is returned.

```c++
bool DoString(const char* chunk, std::string* errstr = nullptr,
              const std::function<bool (int, const LuaObject&)>& callback = nullptr);
```

Evaluates the chunk `chunk`. The rest of arguments, `errstr` and `callback`, have the same meaning as in `LuaFunction::Exec()`.

```c++
bool DoFile(const char* script, std::string* errstr = nullptr,
            const std::function<bool (int, const LuaObject&)>& callback = nullptr);
```

Loads and evaluates the Lua script `script`. The rest of arguments, `errstr` and `callback`, have the same meaning as in `LuaFunction::Exec()`.

[[back to top](#table-of-contents)]

-----

# FAQ

* The `lua_State` object inside each `LuaState` instance is handled by `std::shared_ptr`, so all objects can be passed around freely.
* Class member functions should be called with colon operator, in the form of `object:func()`, to ensure the object itself is the first argument passed to `func`. Otherwise you need to do it manually, like `object.func(object, <other arguments>)`.

[[back to top](#table-of-contents)]

-----

# License

This program is distributed under the MIT License.
