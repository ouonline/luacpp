# Table of Contents

* [Overview](#overview)
* [Quick Start](#quick-start)
    - [Setting and Getting Variables](#setting-and-getting-variables)
    - [Processing Tables](#processing-tables)
    - [Calling Functions](#calling-functions)
    - [Exporting and Using User-defined Types](#exporting-and-using-user-defined-types)
* [Classes and APIs](#classes-and-apis)
    - [LuaObject](#luaobject)
    - [LuaTable](#luatable)
    - [LuaFunction](#luafunction)
    - [LuaClass](#luaclass)
    - [LuaUserData](#luauserdata)
    - [LuaState](#luastate)
* [Notes](#notes)
* [License](#license)

-----

# Overview

`lua-cpp` is a C++ library aiming at simplifying the use of Lua API. It is compatible with Lua 5.2.3(or above) and needs C++14 support.

[[back to top](#table-of-contents)]

-----

# Building from Source

Prerequisites

* Lua >= 5.2
* GCC >= 4.9 with c++14 support
* CMake >= 3.10 (optional)

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DLUA_INCLUDE_DIRS=/path/to/lua/include/dir -DLUA_LIBRARIES=/path/to/lua/libs ..
make -j
```

[[back to top](#table-of-contents)]

-----

# Quick Start

This section is a brief introduction of some APIs. Examples can also be found in `test.cpp`. All available classes and functions are listed in Section [Classes and APIs](#classes-and-apis).

## Setting and Getting Variables

Let's start with a simple example, the famous `Hello, world!`:

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    l.Set("msg", "Hello, luacpp from ouonline!");
    auto lobj = l.Get("msg");
    if (lobj.Type() == LUA_TSTRING) {
        auto buf = lobj.ToBufferRef();
        cout << "get msg -> " << buf.base << endl;
    } else {
        cerr << "unknown object type -> " << lobj.TypeName() << endl;
    }

    return 0;
}
```

In this example, we set the variable `msg`'s value to be a string "Hello, luacpp from ouonline!", then use the getter function to fetch its value and print it.

`LuaState::Set()`s are a series of overloaded functions, which can be used to set up various kinds of variables. Once the variable is set, its value is kept in the `LuaState` instance until it is modified again or the `LuaState` instance is destroyed.

`LuaState::Get()` is used to get a variable by its name. The value returned by `LuaState::Get()` is a `LuaObject` instance, which can be converted to the proper type later. You'd better check the return value of `LuaObject::Type()` before any conversions.

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
        cout << "    ";
        if (key.Type() == LUA_TNUMBER) {
            cout << key.ToNumber();
        } else if (key.Type() == LUA_TSTRING) {
            auto buf = key.ToBufferRef();
            cout << buf.base;
        } else {
            cout << "unsupported key type -> " << key.TypeName() << endl;
            return false;
        }

        if (value.Type() == LUA_TNUMBER) {
            cout << " -> " << value.ToNumber() << endl;
        } else if (value.Type() == LUA_TSTRING) {
            auto buf = value.ToBufferRef();
            cout << " -> " << buf.base << endl;
        } else {
            cout << " -> unsupported iter value type: " << value.TypeName() << endl;
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
        auto buf = lobj.ToBufferRef();
        cout << "output from resiter1: " << buf.base << endl;
        return true;
    };
    auto resiter2 = [] (int n, const LuaObject& lobj) -> bool {
        cout << "output from resiter2: ";
        if (n == 0) {
            cout << lobj.ToNumber() << endl;
        } else if (n == 1) {
            auto buf = lobj.ToBufferRef();
            cout << buf.base << endl;
        }

        return true;
    };

    auto lfunc = l.CreateFunction([] (const char* msg) -> int {
        cout << "in std::function Echo(str): '" << msg << "'" << endl;
        return 5;
    }, "Echo");

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
             << msg << "' and int -> " << x << "." << endl;

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

    int m_value;
    static const int st_value;

private:
    string m_msg;
};

const int ClassDemo::st_value = 5;
```

Then we see how to export this class to the Lua environment:

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    auto lclass = l.CreateClass<ClassDemo>("ClassDemo");

    cout << "--------------------------------------------" << endl;
    lclass.DefConstructor();
    l.DoString("tc = ClassDemo()");

    cout << "--------------------------------------------" << endl;
    lclass.DefConstructor<const char*, int>();
    l.DoString("tc = ClassDemo('ouonline', 5)");

    cout << "--------------------------------------------" << endl;
    lclass.DefMember("print", &ClassDemo::Print)
        .DefMember<void, const char*>("echo_str", &ClassDemo::Echo) // overloaded function
        .DefMember<void, int>("echo_int", &ClassDemo::Echo)
        .DefMember("m_value", &ClassDemo::m_value) // default is READWRITE
        .DefStatic("st_value", &ClassDemo::st_value, luacpp::READ);
    l.DoString("tc = ClassDemo('ouonline', 5); tc:print();"
               "tc:echo_str('calling class member function from lua')");

    cout << "--------------------------------------------" << endl;
    lclass.DefStatic("s_echo", &ClassDemo::StaticEcho);
    l.DoString("ClassDemo:s_echo('static member function is called without being instantiated');"
               "tc = ClassDemo(); tc:s_echo('static member function is called by an instance')");

    cout << "--------------------------------------------" << endl;

    return 0;
}
```

`LuaState::CreateClass()` is used to export user-defined classes to Lua. It requires a string `name` as the class's name in the Lua environment, and adds a default constructor and a destructor for this class. You can register different names with the same c++ class.

`LuaClass::DefMember()` is a template function used to export member functions for this class. `LuaClass::DefStatic()` is used to export static member functions. Both member functions and staic member functions can be C-style functions or `std::function`s.

Class member functions should be called with colon operator in the form of `object:func()`, to ensure the object itself is the first argument passed to `func`. Otherwise you need to do it manually, like `object.func(object, <other arguments>)`.

The following snippet displays how to use `LuaUserData` to exchange data between C++ and Lua:

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("print", &ClassDemo::Print);
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

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("set", &ClassDemo::Set);
    l.DoString("tc = ClassDemo('ouonline', 3); tc:set('in cpp: Print test data from lua')");
    l.Get("tc").ToUserData().Get<ClassDemo>()->Print();

    return 0;
}
```

[[back to top](#table-of-contents)]

-----

# Classes and APIs

This section describes all classes and functions provided by `lua-cpp`.

## LuaObject

`LuaObject` represents an arbitrary item of Lua. You are expected to check the return value of `Type()` before any of the following functions is called.

```c++
LuaObject(lua_State* l, int index);
```

Creates a `LuaObject` with the object located in `index` of the lua_State `l`.

```c++
int Type() const;
```

Returns the type of this object. The return value is one of the following: `LUA_TNIL`, `LUA_TNUMBER`, `LUA_TBOOLEAN`, `LUA_TSTRING`, `LUA_TTABLE`, `LUA_TFUNCTION`, `LUA_TUSERDATA`, `LUA_TTHREAD`, and `LUA_TLIGHTUSERDATA`.

```c++
const char* TypeName() const;
```

Returns the name of the object's type.

```c++
bool ToBool() const;
```

Converts this object to a `bool` value.

```c++
LuaBufferRef ToBufferRef() const;
```

Converts this object to a `LuaBufferRef` object, which only contains address and size of the buffer.

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

`LuaTable`(inherits from `LuaObject`) represents the table type of Lua.

```c++
LuaTable(lua_State* l, int index);
```

Creates a `LuaTable` with the table located in `index` of the lua_State `l`.

```c++
uint64_t Size() const;
```

Returns the number of elements in the table. Note that in current implementation this function needs to push and pop the table.

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
void Set(int index, const char* str, uint64_t len);
void Set(const char* name, const char* str, uint64_t len)
```

Sets the value at `index` or `name` to be the string `str` with length `len`.

```c++
void Set(int index, lua_Number n);
void Set(const char* name, lua_Number n);
```

Sets the value at `index` or `name` to be the number `n`.

```c++
void Set(int index, const LuaObject& lobj);
void Set(const char* name, const LuaObject& lobj)
```

Sets the value at `index` or `name` to be the object `lobj`. Note that `lobj` and this table **MUST** be created by the same LuaState.

```c++
bool ForEach(const std::function<bool (const LuaObject& key,
                                       const LuaObject& value)>& func) const;
```

Itarates the table with the callback function `func`.

[[back to top](#table-of-contents)]

## LuaFunction

`LuaFunction`(inherits from `LuaObject`) represents the function type of Lua.

```c++
LuaFunction(lua_State* l, uint64_t index);
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

`LuaClass`(inherits from `LuaObject`) is used to export C++ classes and member functions to Lua. It does not support exporting member variables.

```c++
LuaClass(lua_State* l, int index);
```

Creates a `LuaClass` with the table located in `index` of the lua_State `l`.

```c++
template<typename... FuncArgType>
LuaClass<T>& DefConstructor();
```

Sets the class's constructor with argument type `FuncArgType` and return a reference of the class itself.

```c++
/** member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& DefMember(const char* name, FuncRetType (T::*f)(FuncArgType...));

/** member function with const qualifier */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& DefMember(const char* name, FuncRetType (T::*f)(FuncArgType...) const);

/** std::function member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& DefMember(const char* name, const std::function<FuncRetType (FuncArgType...)>& f);

/** lambda member function */
template <typename FuncType>
LuaClass& DefMember(const char* name, const FuncType& f);

/** c-style member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& DefMember(const char* name, FuncRetType (*f)(FuncArgType...));

/** lua-style member function */
LuaClass<T>& DefMember(const char* name, int (*f)(lua_State* l));
```

Exports function `f` to be a member function of this class and `name` as the exported function name in Lua.

```c++
/** property */
template <typename PropertyType>
LuaClass& DefMember(const char* name, PropertyType T::* mptr, uint32_t permission = READWRITE);
```

Exports a member pointed by `mptr` as a member `name` of this class in Lua.

```c++
/** std::function static member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& DefStatic(const char* name, const std::function<FuncRetType (FuncArgType...)>& f);

/** C-style static member function */
template <typename FuncRetType, typename... FuncArgType>
LuaClass<T>& DefStatic(const char* name, FuncRetType (*f)(FuncArgType...));

/** lambda static member function */
template <typename FuncType>
LuaClass<T>& DefStatic(const char* name, const FuncType& f);

/** lua-style static member function */
LuaClass<T>& DefStatic(const char* name, int (*f)(lua_State* l));
```

Exports function `f` to be a static member function of this class and `name` as the exported function name in Lua.

```c++
/** static property */
template <typename PropertyType>
LuaClass& DefStatic(const char* name, PropertyType* ptr, uint32_t permission = READWRITE);
```

Exports a static member pointed by `ptr` as a member `name` of this class in Lua.

[[back to top](#table-of-contents)]

## LuaUserData

`LuaUserData` (inherits from `LuaObject`) represents the userdata type in Lua.

```c++
LuaUserData(lua_State* l, int index);
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
LuaState(lua_State* l, bool is_owner);
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
void Set(const char* name, const char* str, uint64_t len);
```

Sets the variable `name` to be the string `str` with length `len`.

```c++
void Set(const char* name, lua_Number n);
```

Sets the variable `name` to be the number `n`.

```c++
void Set(const char* name, const LuaObject& lobj);
```

Sets the variable `name` to be the object `lobj`. Note that `lobj` **MUST** be created by this LuaState.

```c++
LuaTable CreateTable(const char* name = nullptr);
```

Creates a new table with table name `name`(if not NULL).

```c++
/** c-style function */
template <typename FuncRetType, typename... FuncArgType>
LuaFunction CreateFunction(FuncRetType (*f)(FuncArgType...), const char* name = nullptr);

/** std::function */
template <typename FuncRetType, typename... FuncArgType>
LuaFunction CreateFunction(const std::function<FuncRetType (FuncArgType...)>& f, const char* name = nullptr);

/** lambda function */
template <typename FuncType>
LuaFunction CreateFunction(const FuncType& f, const char* name = nullptr);
```

Creates a function object from `f` with `name`(if any).

```c++
template<typename T>
LuaClass<T> CreateClass(const char* name);
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

# Notes

* Class member functions should be called with colon operator, in the form of `object:func(...)`, to ensure the object itself is the first argument passed to func. Otherwise you need to do it manually, like `object.func(object, ...)`.
* Currently `luacpp` doesn't support passing and returning values by reference. Only bool/int/float/pointer/LuaObject are supported. You should export functions with supported types.

-----

# License

This project is distributed under the MIT License.
