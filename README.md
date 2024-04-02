# Table of Contents

* [Overview](#overview)
* [Building from Source](#building-from-source)
* [Quick Start](#quick-start)
    - [Setting and Getting Variables](#setting-and-getting-variables)
    - [Processing Tables](#processing-tables)
    - [Calling Functions](#calling-functions)
    - [Exporting and Using User-defined Types](#exporting-and-using-user-defined-types)
    - [Class Inheritance](#class-inheritance)
* [Notes](#notes)
* [API Reference](#api-reference)
    - [LuaObject](#luaobject)
    - [LuaTable](#luatable)
    - [LuaFunction](#luafunction)
    - [LuaClass](#luaclass)
    - [LuaState](#luastate)

-----

# Overview

`luacpp` is a C++ library aiming at simplifying the use of Lua APIs. It is compatible with Lua 5.2.0(or above) and needs C++11 support.

[[back to top](#table-of-contents)]

-----

# Building from Source

Prerequisites

* Lua >= 5.2.0
* GCC >= 4.9 or Clang >= 6.0 or Visual Studio >= 2015, with C++11 support
* CMake >= 3.10 (optional)

Building with pre-installed Lua package:

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target install --config Release -j `nproc`
```

or building with Lua source by specifying `LUA_SRC_DIR`:

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DLUA_SRC_DIR=/path/to/lua/src ..
```

If you have a Lua binary package, specify the header directories through `LUA_INCLUDE_DIR` manually:

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DLUA_INCLUDE_DIR=/path/to/lua/include/dir ..
```

and build tests:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DLUACPP_BUILD_TESTS=ON ..
```

or specifying `LUA_INCLUDE_DIR` and `LUA_LIBRARIES`:

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DLUACPP_BUILD_TESTS=ON -DLUA_INCLUDE_DIR=/path/to/lua/include/dir -DLUA_LIBRARIES=/path/to/lua/<liblua.a|liblua.lib> ..
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

#include "luacpp/luacpp.h"
using namespace luacpp;

int main(void) {
    LuaState l(luaL_newstate(), true);

    l.CreateString("Hello, luacpp from ouonline!", "msg");
    auto lobj = l.Get("msg");
    if (lobj.GetType() == LUA_TSTRING) {
        cout << "get msg -> " << lobj.ToString() << endl;
    } else {
        cerr << "unknown object type -> " << lobj.GetTypeName() << endl;
    }

    return 0;
}
```

In this example, we set the variable `msg`'s value to be a string "Hello, luacpp from ouonline!", then use the getter function to fetch its value and print it.

`LuaState::CreateString()`, `LuaState::CreateNumber()` and `LuaState::CreateInteger()` are a series of functions used to set up various kinds of variables. Once a variable is set, its value is kept in the `LuaState` instance until it is modified again or the `LuaState` instance is destroyed.

`LuaState::Get()` is used to get a variable by its name. The value returned by `LuaState::Get()` is a `LuaObject` instance, which can be converted to the proper type later. You'd better check the return value of `LuaObject::GetType()` before any conversions.

[[back to top](#table-of-contents)]

## Processing Tables

```c++
#include <iostream>
using namespace std;

#include "luacpp/luacpp.h"
using namespace luacpp;

int main(void) {
    LuaState l(luaL_newstate(), true);

    auto iterfunc = [] (const LuaObject& key, const LuaObject& value) -> bool {
        cout << "    ";
        if (key.GetType() == LUA_TNUMBER) {
            cout << key.ToNumber();
        } else if (key.GetType() == LUA_TSTRING) {
            cout << key.ToString();
        } else {
            cout << "unsupported key type -> " << key.GetTypeName() << endl;
            return false;
        }

        if (value.GetType() == LUA_TNUMBER) {
            cout << " -> " << value.ToNumber() << endl;
        } else if (value.GetType() == LUA_TSTRING) {
            cout << " -> " << value.ToString() << endl;
        } else {
            cout << " -> unsupported iter value type: " << value.GetTypeName() << endl;
        }

        return true;
    };

    cout << "table1:" << endl;
    l.DoString("var = {'mykey', value = 'myvalue', others = 'myothers'}");
    LuaTable(l.Get("var")).ForEach(iterfunc);

    cout << "table2:" << endl;
    auto ltable = l.CreateTable();
    ltable.SetInteger("x", 5);
    ltable.SetString("o", "ouonline");
    ltable.ForEach(iterfunc);

    return 0;
}
```

We use `LuaState::DoString()` to execute a chunk that creates a table named `var` with 3 fields.

`LuaTable::ForEach()` takes a callback function that is used to iterate each key-value pair in the table. If the callback function returns `false`, `LuaTable::ForEach()` exits and returns `false`.

We can use `LuaState::CreateTable()` to create a new empty table and use `LuaTable::SetNumber()`, `LuaTable::SetInteger()`, `LuaTable::SetString()` and `LuaTable::Set()` to set fields of this table.

[[back to top](#table-of-contents)]

## Calling Functions

```c++
#include <iostream>
using namespace std;

#include "luacpp/luacpp.h"
using namespace luacpp;

int main(void) {
    LuaState l(luaL_newstate(), true);

    auto resiter1 = [] (uint32_t, const LuaObject& lobj) -> bool {
        cout << "output from resiter1: " << lobj.ToString() << endl;
        return true;
    };
    auto resiter2 = [] (uint32_t n, const LuaObject& lobj) -> bool {
        cout << "output from resiter2: ";
        if (n == 0) {
            cout << lobj.ToNumber() << endl;
        } else if (n == 1) {
            cout << lobj.ToString() << endl;
        }

        return true;
    };

    auto lfunc = l.CreateFunction([] (const char* msg) -> int {
        cout << "in std::function Echo(str): '" << msg << "'" << endl;
        return 5;
    }, "Echo");

    l.CreateString("calling cpp function with return value from cpp: ", "msg");
    lfunc.Execute(resiter1, nullptr, l.Get("msg"));

    l.DoString("res = Echo('calling cpp function with return value from lua: ');"
               "io.write('return value -> ', res, '\\n')");

    l.DoString("function return2(a, b) return a, b end");
    LuaFunction(l.Get("return2")).Execute(resiter2, nullptr, 5, "ouonline");

    return 0;
}
```

First we define a C++ function `echo` and set its name to be `echo` in the Lua environment using `LuaState::CreateFunction()`. `LuaFunction::Execute()` invokes the real function. See [LuaFunction](#luafunction) for more details.

[[back to top](#table-of-contents)]

## Exporting and Using User-defined Types

We define a class `ClassDemo` for test:

```c++
class ClassDemo {
public:
    ClassDemo() {
        cout << "ClassDemo::ClassDemo() is called without value." << endl;
    }
    ClassDemo(const char* msg, int x) {
        if (msg) {
            m_msg = msg;
        }
        cout << "ClassDemo::ClassDemo() is called with string -> '"
             << msg << "' and int -> " << x << "." << endl;
    }
    virtual ~ClassDemo() {
        cout << "ClassDemo::~ClassDemo() is called." << endl;
    }

    void Set(const char* msg) {
        m_msg = msg;
        cout << "ClassDemo()::Set(msg): '" << msg << "'" << endl;
    }
    void Print() const {
        cout << "ClassDemo::Print(msg): '" << m_msg << "'" << endl;
    }
    virtual const char* Echo(const char* msg) const {
        cout << "ClassDemo::Echo(string): '" << msg << "'" << endl;
        return msg;
    }
    int Echo(int v) const {
        cout << "ClassDemo::Echo(int): " << v << endl;
        return v;
    }
    static const char* StaticEcho(const char* msg) {
        cout << "ClassDemo::StaticEcho(string): '" << msg << "'" << endl;
        return msg;
    }

    static const int st_value;
    int m_value = 55555;

private:
    string m_msg;
};

const int ClassDemo::st_value = 5;
```

Then we see how to export this class to the Lua environment:

```c++
#include <iostream>
using namespace std;

#include "luacpp/luacpp.h"
using namespace luacpp;

int main(void) {
    LuaState l(luaL_newstate(), true);

    auto lclass = l.CreateClass<ClassDemo>("ClassDemo");

    cout << "--------------------------------------------" << endl;
    lclass.DefConstructor();
    l.DoString("tc = ClassDemo()");

    cout << "--------------------------------------------" << endl;
    lclass.DefConstructor<const char*, int>();
    l.DoString("tc = ClassDemo('ouonline', 5)");

    cout << "--------------------------------------------" << endl;
    lclass.DefMember("print", &ClassDemo::Print)
        .DefMember<const char* (ClassDemo::*)(const char*) const>("echo_str", &ClassDemo::Echo) // overloaded function
        .DefMember<int (ClassDemo::*)(int) const>("echo_int", &ClassDemo::Echo)
        // default is read/write
        .DefMember<int>("m_value",
                        [](const ClassDemo* c) -> int { return c->m_value; },
                        [](ClassDemo* c, int v) -> void { c->m_value = v; })
        // read only
        .DefStatic<int>("st_value", []() -> int { return ClassDemo::st_value; }, nullptr);
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

`LuaClass::DefMember()` is template functions used to export member functions and properties and `LuaClass::DefStatic()` to export static member functions and properties. Member functions and staic member functions can be C-style functions, `std::function`s and lambda functions.

Class member functions should be called with colon operator in the form of `object:func()`, to ensure the object itself is the first argument passed to `func`. Otherwise you need to do it manually, like `object.func(object, <other arguments>)`.

The following snippet displays how to use userdata to exchange data between C++ and Lua:

```c++
#include <iostream>
using namespace std;

#include "luacpp/luacpp.h"
using namespace luacpp;

int main(void) {
    LuaState l(luaL_newstate(), true);

    auto lclass = l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("get_object_addr", [](ClassDemo* obj) -> ClassDemo* {
            return obj;
        })
        .DefMember("print", &ClassDemo::Print);

    auto lud = lclass.CreateInstance("ouonline", 5);
    auto ud = static_cast<ClassDemo*>(lud.ToPointer());
    ud->Set("in lua: Print test data from cpp");
    l.Set("tc", lud);
    l.DoString("tc:print()");

    l.DoString("obj = tc:get_object_addr();");
    auto obj_addr = static_cast<ClassDemo*>(l.Get("obj").ToPointer());
    // ud == obj_addr;

    return 0;
}
```

We create a userdata object of type `ClassDemo` by calling `LuaClass::CreateInstance()` and set its name to be `tc` in the Lua environment. Then we set its content to be a string, which is printed by calling `LuaState::DoString()`.

The following example is similar to the previous one, except that we modify the object's content in the Lua environment but print it in C++.

```c++
#include <iostream>
using namespace std;

#include "luacpp/luacpp.h"
using namespace luacpp;

int main(void) {
    LuaState l(luaL_newstate(), true);

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("set", &ClassDemo::Set);
    l.DoString("tc = ClassDemo('ouonline', 3); tc:set('in cpp: Print test data from lua')");
    static_cast<ClassDemo*>(l.Get("tc").ToPointer())->Print();

    return 0;
}
```

[[back to top](#table-of-contents)]

## Class Inheritance

We define `DerivedDemo1` and `DerivedDemo2`:

```c++
class DerivedDemo1 : public ClassDemo {}; // do nothing

class DerivedDemo2 : public DerivedDemo1 {}; // do nothing
```

and use

```c++
template <typename BaseType>
LuaClass& AddBaseClass(const LuaClass<BaseType>& lclass);
```

to connect derived classes with base classes:

```c++
LuaState l(luaL_newstate(), true);

auto base = l.CreateClass<ClassDemo>("ClassDemo")
    .DefConstructor()
    .DefStatic("StaticEcho", &ClassDemo::StaticEcho)
    .DefMember<const char* (ClassDemo::*)(const char*) const>("echo", &ClassDemo::Echo)
    .DefMember<int>("m_value",
                    [](const ClassDemo* c) -> int {
                        return c->m_value;
                    },
                    [](ClassDemo* c, int v) -> void {
                        c->m_value = v;
                    });

auto derived1 = l.CreateClass<DerivedDemo1>("DerivedDemo1")
    .AddBaseClass(base)
    .DefConstructor();

auto derived2 = l.CreateClass<DerivedDemo2>("DerivedDemo2")
    .AddBaseClass(derived1)
    .DefConstructor();
```


[[back to top](#table-of-contents)]

-----

# Notes

Builtin types(`LuaRefObject`, `LuaObject`, `LuaTable`, `LuaFunction` and `LuaStringRef`) should only be used in exported functions. Don't use them in ordinary c++ functions.

Currently `luacpp` supports exporting functions with the following argument types:

* basic types(`bool`, `float`, `double` and integers)
* const reference of luacpp builtin types
* pointers to basic types and user-defined types.

For example:

```c++
// builtin types(LuaObject for example)
l.CreateFunction([](const LuaObject&) -> void {}); // ok
l.CreateFunction([](LuaObject*) -> void {}); // ok but not recommended: will get the pointer itself
l.CreateFunction([](const LuaObject*) -> void {}); // ok but not recommended: will get the pointer itself
l.CreateFunction([](LuaObject) -> void {}); // ok but not recommended: may cause extra constructions
l.CreateFunction([](LuaObject&) -> void {}); // error: non-const reference of builtin types
l.CreateFunction([](LuaObject&&) -> void {}); // error: reference of builtin types

// basic types(int for example)
l.CreateFunction([](int) -> void {}); // ok
l.CreateFunction([](int*) -> void {}); // ok
l.CreateFunction([](const int*) -> void {}); // ok
l.CreateFunction([](int&) -> void {}); // error: reference of basic types
l.CreateFunction([](const int&) -> void {}); // error: reference of basic types
l.CreateFunction([](int&&) -> void {}); // error: reference of basic types

// user-defined types
l.CreateFunction([](UserType*) -> void {}); // ok
l.CreateFunction([](const UserType*) -> void {}); // ok
l.CreateFunction([](UserType) -> void {}); // error: passed by value
l.CreateFunction([](UserType&) -> void {}); // error: reference of user-defined types
l.CreateFunction([](const UserType&) -> void {}); // error: reference of user-defined types
l.CreateFunction([](UserType&&) -> void {}); // error: reference of user-defined types
```

Types of returned values can be one of:

* basic types(`bool`, `float`, `double` and integers)
* builtin types(`LuaRefObject`, `LuaObject`, `LuaTable`, `LuaFunction` and `LuaStringRef`)
* pointers to user-defined types.

For example:

```c++
// builtin types(LuaObject for example)
l.CreateFunction([]() -> LuaObject {...}); // ok, will be converted to lua types
l.CreateFunction([]() -> LuaObject* {...}); // error: pointers to
l.CreateFunction([]() -> LuaObject& {...}); // error: reference
l.CreateFunction([]() -> const LuaObject& {...}); // error: reference

// basic types(int for example)
l.CreateFunction([]() -> int {...}); // ok
l.CreateFunction([]() -> int* {...}); // ok, will be converted to a light user data in lua(not the value pointed by it)
l.CreateFunction([]() -> int& {...}); // error: reference
l.CreateFunction([]() -> const int& {...}); // error: reference

// user-defined types
l.CreateFunction([]() -> UserType* {...}); // ok, will be converted to a light user data in lua
l.CreateFunction([]() -> UserType {...}); // error: values of user-defined types
l.CreateFunction([]() -> UserType& {...}); // error: reference
l.CreateFunction([]() -> const UserType& {...}); // error: reference
```

If you want to return a user-defined types, you can do this like:

```c++
l.CreateFunction([]() -> LuaObject {
    LuaClass<T> lclass = l.Get("UserType");
    return lclass.CreateInstance(args...); // `args...` are parameters that will be passed to `UserType`s constructor.
})
```

[[back to top](#table-of-contents)]

-----

# API Reference

This section describes all classes and functions provided by `luacpp`.

## LuaObject

`LuaObject` represents an arbitrary item of Lua. You are expected to check the return value of `GetType()` before any of the following functions is called.

```c++
int GetType() const;
```

Returns the type of this object. The return value is one of the following: `LUA_TNIL`, `LUA_TNUMBER`, `LUA_TBOOLEAN`, `LUA_TSTRING`, `LUA_TTABLE`, `LUA_TFUNCTION`, `LUA_TUSERDATA`, `LUA_TTHREAD`, and `LUA_TLIGHTUSERDATA`.

```c++
const char* GetTypeName() const;
```

Returns the name of the object's type.

```c++
bool ToBool() const;
```

Converts this object to a `bool` value.

```c++
LuaStringRef ToStringRef() const;
```

Converts this object to a `LuaStringRef` object, which only contains address and size of the buffer.

```c++
const char* ToString() const;
```

Converts this object to a C-style string.

```c++
lua_Integer ToInteger() const;
```

Converts this object to an integer.

```c++
lua_Number ToNumber() const;
```

Converts this object to a (floating point) number.

```c++
void* ToPointer() const;
```

Converts this object to a pointer.

[[back to top](#table-of-contents)]

## LuaTable

`LuaTable`(inherits from `LuaRefObject`) represents the table type of Lua.

```c++
LuaObject Get(int index) const;
LuaObject Get(const char* name) const;
```

Returns the object in the specified `index` or associated with `name`.

```c++
LuaTable GetTable(int index) const;
LuaTable GetTable(const char* name) const;
```

Returns the object in the specified `index` or associated with `name` as a `LuaTable`.

```c++
LuaFunction GetFunction(int index) const;
LuaFunction GetFunction(const char* name) const;
```

Returns the object in the specified `index` or associated with `name` as a `LuaFunction`.

```c++
template <typename T>
LuaClass<T> GetClass(int index) const;

template <typename T>
LuaClass<T> GetClass(const char* name) const;
```

Returns the object in the specified `index` or associated with `name` as a `LuaClass<T>`.

```c++
LuaStringRef GetStringRef(int index) const;
LuaStringRef GetStringRef(const char* name) const;
```

Returns the object in the specified `index` or associated with `name` as a `LuaStringRef`.

```c++
const char* GetString(int index) const;
const char* GetString(const char* name) const;
```

Returns the object in the specified `index` or associated with `name` as a C-style string.

```c++
lua_Number GetNumber(int index) const;
lua_Number GetNumber(const char* name) const;
```

Returns the object in the specified `index` or associated with `name` as a floating point number.

```c++
lua_Integer GetInteger(int index) const;
lua_Integer GetInteger(const char* name) const;
```

Returns the object in the specified `index` or associated with `name` as an integer.

```c++
void* GetPointer(int index) const;
void* GetPointer(const char* name) const;
```

Returns the object in the specified `index` or associated with `name` as a pointer.

```c++
void Set(int index, const LuaRefObject& lobj);
void Set(const char* name, const LuaRefObject& lobj)
```

Sets the value at `index` or `name` to the object `lobj`. Note that `lobj` and this table **MUST** be created by the same LuaState.

```c++
void SetString(int index, const char* str);
void SetString(const char* name, const char* str);
```

Sets the value at `index` or `name` to the string `str`.

```c++
void SetString(int index, const char* str, uint64_t len);
void SetString(const char* name, const char* str, uint64_t len)
```

Sets the value at `index` or `name` to the string `str` with length `len`.

```c++
void SetNumber(int index, lua_Number n);
void SetNumber(const char* name, lua_Number n);
```

Sets the value at `index` or `name` to a (floating) number `n`.

```c++
void SetInteger(int index, lua_Integer n);
void SetInteger(const char* name, lua_Integer n);
```

Sets the value at `index` or `name` to the integer `n`.

```c++
void SetPointer(int index, void* p);
void SetPointer(const char* name, void* p);
```

Sets the value at `index` or `name` to the pointer `p`.

```c++
uint64_t GetSize() const;
```

Returns the size of this table.

```c++
template <typename FuncType>
bool ForEach(FuncType&& func) const;
```

Itarates the table with the callback function `func`, which is one of the following types:

```c++
template <typename T2>
std::function<bool (uint32_t i, const T2& value)>;

template <typename T1, typename T2>
std::function<bool (const T1& key, const T2& value)>;
```

Note that the parameter `i` starts from 0. Both `T1` and `T2` are builtin types(`LuaRefObject`, `LuaObject`, `LuaFunction`, `LuaTable` and `LuaStringRef`).

[[back to top](#table-of-contents)]

## LuaFunction

`LuaFunction`(inherits from `LuaRefObject`) represents the function type of Lua.

```c++
template <typename... Argv>
bool Execute(const std::function<bool (uint32_t i, const LuaObject&)>& callback = nullptr,
             std::string* errstr = nullptr, Argv&&... argv);
```

Invokes the function with arguments `argv`. `callback` is a callback function used to handle result(s). Note that the first argument `i` of `callback` starts from 0. `errstr` is a string to receive a message if an error occurs. The rest of arguments `argv`, if any, are passed to the real function being called.

[[back to top](#table-of-contents)]

## LuaClass

`LuaClass`(inherits from `LuaRefObject`) is used to export C++ classes and member functions to Lua.

```c++
template<typename... FuncArgType>
LuaClass<T>& DefConstructor();
```

Sets the class's constructor with argument type `FuncArgType` and returns a reference of the class itself.

```c++
template <typename BaseType>
LuaClass& AddBaseClass(const LuaClass<BaseType>& lclass);
```

Connects the derived classes with their base classes.

```c++
template <typename FuncType>
LuaClass& DefMember(const char* name, FuncType&& f);
```

Exports function `f` to be a member function of this class and `name` as the exported function name in Lua. `FuncType` can be C-style functions, class member functions, `std::function`s, lambda functions and lua-style C functions.

```c++
/**
   member property
     - GetterType: (const T*) -> PropertyType
     - SetterType: (T*, PropertyType) -> void
*/
template <typename PropertyType, typename GetterType, typename SetterType>
LuaClass& DefMember(const char* name, GetterType&& getter, SetterType&& setter);
```

Exports a member property `name` of this class in Lua. `nullptr` means that this property cannot be read or written. See `tests/test_class.hpp` for usage examples.

```c++
template <typename FuncType>
LuaClass<T>& DefStatic(const char* name, FuncType&& f);
```

Exports function `f` to be a static member function of this class and `name` as the exported function name in Lua. `FuncType` can be C-style functions, `std::function`s, lambda functions and lua-style C functions.

```c++
/**
   static property.
     - GetterType: () -> PropertyType
     - SetterType: (PropertyType) -> void
*/
template <typename PropertyType, typename GetterType, typename SetterType>
LuaClass& DefStatic(const char* name, GetterType&& getter, SetterType&& setter);
```

Exports a static property `name` of this class in Lua. `nullptr` means that this property cannot be read or written. See `tests/test_class.hpp` for usage examples.

```c++
template<typename... Argv>
LuaObject CreateInstance(Argv&&... argv) const;
```

Creates an instance of this class. The arguments `argv` are passed to the constructor of `T`. The newly created instance can be obtained by calling `LuaObject::ToPointer()`.

[[back to top](#table-of-contents)]

## LuaState

```c++
LuaState(lua_State* l, bool is_owner);
```

The constructor.

```c++
void Set(const char* name, const LuaRefObject& lobj);
```

Sets the variable `name` to be the object `lobj`. Note that `lobj` **MUST** be created by this LuaState.

```c++
LuaObject Get(const char* name) const;
```

Returns an object associated with `name`.

```c++
LuaTable GetTable(const char* name) const;
```

Returns a `LuaTable` associated with `name`.

```c++
LuaFunction GetFunction(const char* name) const;
```

Returns a `LuaFunction` associated with `name`.

```c++
template <typename T>
LuaClass<T> GetClass(const char* name) const;
```

Returns a `LuaClass<T>` associated with `name`.

```c++
void Push(const LuaRefObject& lobj);
```

Pushes an object `lobj`.

```c++
void PushString(const char* str);
```

Pushes a null-terminated string `str`.

```c++
void PushString(const char* str, uint64_t len);
```

Pushes a string `str` with length `len`.

```c++
void PushNumber(lua_Number value);
```

Pushes a (floating) number `value`.

```c++
void PushInteger(lua_Integer value);
```

Pushes an integer `value`.

```c++
void PushNil();
```

Pushes a `nil`.


```c++
LuaObject CreateNil();
```

Returns a `nil` object.

```c++
LuaObject CreateString(const char* str, const char* name = nullptr);
LuaObject CreateString(const char* str, uint64_t len, const char* name = nullptr);
```

Sets the variable `name`(if present) to be a string pointed by `str` with length `len` and returns that object.

```c++
LuaObject CreateNumber(lua_Number value, const char* name = nullptr);
```

Sets the variable `name`(if present) to be a (floating) number `n` and returns that object.

```c++
LuaObject CreateInteger(lua_Integer value, const char* name = nullptr);
```

Sets the variable `name`(if present) to be an integer `n` and returns that object.

```c++
LuaTable CreateTable(const char* name = nullptr);
```

Creates a new table with table name `name`(if present).

```c++
template <typename FuncType>
LuaFunction CreateFunction(FuncType&& f, const char* name = nullptr);
```

Creates a function object from `f` with `name`(if present). `FuncType` can be C-style functions, `std::function`s, lambda functions and lua-style C functions.

```c++
template<typename T>
LuaClass<T> CreateClass(const char* name);
```

Exports a new type `T` with the name `name`. If `name` is already exported, that class is returned.

```c++
bool DoString(const char* chunk, std::string* errstr = nullptr,
              const std::function<bool (uint32_t, const LuaObject&)>& callback = nullptr);
```

Evaluates the chunk `chunk`. The rest of arguments, `errstr` and `callback`, have the same meaning as in `LuaFunction::Execute()`.

```c++
bool DoFile(const char* script, std::string* errstr = nullptr,
            const std::function<bool (uint32_t, const LuaObject&)>& callback = nullptr);
```

Loads and evaluates the Lua script `script`. The rest of arguments, `errstr` and `callback`, have the same meaning as in `LuaFunction::Execute()`.

[[back to top](#table-of-contents)]
