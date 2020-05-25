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

`lua-cpp` is a C++ library aiming at simplifying the use of Lua API. It is compatible with Lua 5.2.3(or above) and needs C++11 support.

To use `lua-cpp`, all you need to do is to include the header file `luacpp.hpp` and import classes and functions in the namespace `luacpp`.

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
        cout << "get msg -> " << lobj.ToString() << endl;
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
            cout << key.ToString();
        } else {
            cout << "unsupported key type -> " << key.GetTypeStr() << endl;
            return false;
        }

        if (value.GetType() == LUA_TNUMBER) {
            cout << " -> " << value.ToNumber() << endl;
        } else if (value.GetType() == LUA_TSTRING) {
            cout << " -> " << value.ToString() << endl;
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

class GenericFunctionHelper final : public LuaFunctionHelper {
public:
    GenericFunctionHelper(const std::function<bool (int, const LuaObject&)>& f)
        : m_func(f) {}
    bool BeforeProcess(int) override { return true; }
    bool Process(int i, const LuaObject& lobj) override {
        return m_func(i, lobj);
    }
    void AfterProcess() {}
private:
    std::function<bool (int, const LuaObject&)> m_func;
};

int main(void) {
    LuaState l;

    auto resiter1 = [] (int, const LuaObject& lobj) -> bool {
        cout << "output from resiter1: " << lobj.ToString() << endl;
        return true;
    };
    auto resiter2 = [] (int n, const LuaObject& lobj) -> bool {
        cout << "output from resiter2: ";
        if (n == 0) {
            cout << lobj.ToNumber() << endl;
        } else if (n == 1) {
            cout << lobj.ToString() << endl;
        }

        return true;
    };

    GenericFunctionHelper helper1(resiter1), helper2(resiter2);

    std::function<int (const char*)> Echo = [] (const char* msg) -> int {
        cout << msg;
        return 5;
    };
    auto lfunc = l.CreateFunction(Echo, "Echo");

    l.Set("msg", "calling cpp function with return value from cpp: ");
    lfunc.Exec(nullptr, &helper1, l.Get("msg"));

    l.DoString("res = Echo('calling cpp function with return value from lua: ');"
               "io.write('return value -> ', res, '\\n')");

    l.DoString("function return2(a, b) return a, b end");
    l.Get("return2").ToFunctiong().Exec(nullptr, &helper2, 5, "ouonline");

    return 0;
}
```

First we define a C++ function `echo` and set its name to be `echo` in the Lua environment using `LuaState::CreateFunction()`. `LuaFunction::Exec()` invokes the real function. See [LuaFunction](#luafunction) for more details.

[[back to top](#table-of-contents)]

## Exporting and Using User-defined Types

First we define a class `TestClass` for test:

```c++
class TestClass final {
public:
    TestClass() {
        cout << "TestClass::TestClass() is called without value." << endl;
    }
    TestClass(const char* msg, int x) {
        if (msg) {
            m_msg = msg;
        }

        cout << "TestClass::TestClass() is called with string -> '"
             << m_msg << "' and value -> " << x << "." << endl;
    }
    ~TestClass() {
        cout << "TestClass::~TestClass() is called." << endl;
    }

    void Set(const char* msg) { m_msg = msg; }

    void Print() const {
        cout << "TestClass::print(): " << m_msg << endl;
    }
    void Echo(const char* msg) const {
        cout << "TestClass::echo(string): " << msg << endl;
    }
    void Echo(int v) const {
        cout << "TestClass::echo(int): " << v << endl;
    }
    static void StaticEcho(const char* msg) {
        cout << "TestClass::s_echo(string): " << msg << endl;
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

    // NOTE: export only once
    auto lclass = l.CreateClass<TestClass>("TestClass").SetConstructor();

    cout << "--------------------------------------------" << endl;
    l.DoString("tc = TestClass()");

    cout << "--------------------------------------------" << endl;
    lclass.SetConstructor<const char*, int>();
    l.DoString("tc = TestClass('ouonline', 5)");

    cout << "--------------------------------------------" << endl;
    lclass.Set("print", &TestClass::print)
        .Set<void, const char*>("echo_str", &TestClass::Echo) // overloaded function
        .Set<void, int>("echo_int", &TestClass::Echo);
    l.DoString("tc = TestClass('ouonline', 5); tc:print();"
               "tc:echo_str('calling class member function from lua')");

    cout << "--------------------------------------------" << endl;
    lclass.Set("s_echo", &TestClass::StaticEcho);
    l.DoString("TestClass:s_echo('static member function is called without being instantiated');"
               "tc = TestClass(); tc:s_echo('static member function is called by an instance')");

    cout << "--------------------------------------------" << endl;

    return 0;
}
```

`LuaState::CreateClass()` is used to export user-defined classes to Lua. It requires a string `name` as the class's name in the Lua environment, and adds a default constructor and a destructor for this class. If the class is already exported, `LuaState::CreateClass()` throws a `std::runtime_error` exception.

`LuaClass::Set()` is a template function used to export member functions for this class.

Class member functions should be called with colon operator, in the form of `object:func()`, to ensure the object itself is the first argument passed to `func`. Otherwise you need to do it manually, like `object.func(object, <other arguments>)`.

The following program displays how to use `LuaUserData` to exchange data between C++ and Lua.

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    l.CreateClass<TestClass>("TestClass")
        .SetConstructor<const char*, int>()
        .Set("print", &TestClass::Print);
    l.CreateUserData<TestClass>("tc", "ouonline", 5).Get<TestClass>()->Set("in lua: print test data from cpp");
    l.DoString("tc:print()");

    return 0;
}
```

We create a `LuaUserData` object of type `TestClass` by calling `LuaClass::CreateUserData()` and set its name to be `tc` in the Lua environment. Then we set its content to be a string, which is printed by calling `LuaState::DoString()`.

The following example is similar to the previous one, except that we modify the object's content in the Lua environment but print it in C++.

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void) {
    LuaState l;

    l.CreateClass<TestClass>("TestClass")
        .SetConstructor<const char*, int>()
        .Set("set", &TestClass::Set);
    l.DoString("tc = TestClass('ouonline', 5); tc:set('in cpp: print test data from lua')");
    l.Get("tc").ToUserData().object<TestClass>()->Print();

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
int GetType() const;
```

Gets the type of this object. The return value is one of the following: `LUA_TNIL`, `LUA_TNUMBER`, `LUA_TBOOLEAN`, `LUA_TSTRING`, `LUA_TTABLE`, `LUA_TFUNCTION`, `LUA_TUSERDATA`, `LUA_TTHREAD`, and `LUA_TLIGHTUSERDATA`.

```c++
const char* GetTypeStr() const;
```

Returns the name of the object's type.

[[back to top](#table-of-contents)]

## LuaObject

`LuaObject`(inherits from `LuaRefObject`) represents an arbitrary item of Lua. You are expected to check the return value of `GetType()` before any of the following functions is called.

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
std::string ToString() const;
```

Converts this object to a `std::string` object.

```c++
lua_Number ToNumber() const;
```

Converts this object to a number.

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
LuaObject Get(int index) const;
LuaObject Get(const char* name) const;
```

Gets an object by its `index` or `name` in this table.

```c++
bool Set(int index, const char* str);
bool Set(const char* name, const char* str);
```

Sets the value at `index` or `name` to be the string `str`.

```c++
bool Set(int index, const char* str, size_t len);
bool Set(const char* name, const char* str, size_t len)
```

Sets the value at `index` or `name` to be the string `str` with length `len`.

```c++
bool Set(int index, lua_Number n);
bool Set(const char* name, lua_Number n);
```

Sets the value at `index` or `name` to be the number `n`.

```c++
bool Set(int index, const LuaObject& lobj);
bool Set(const char* name, const LuaObject& lobj)
```

Sets the value at `index` or `name` to be the object `lobj`. Note that `lobj` and this table **MUST** be created by the same LuaState.

```c++
bool Set(int index, const LuaTable& ltable);
bool Set(const char* name, const LuaTable& ltable);
```

Sets the value at `index` or `name` to be the table `ltable`. Note that `ltable` and the host table **MUST** be created by the same LuaState.

```c++
bool Set(int index, const LuaFunction& lfunc);
bool Set(const char* name, const LuaFunction& lfunc);
```

Sets the value at `index` or `name` to be the function `lfunc`. Note that `lfunc` and the host table **MUST** be created by the same LuaState.

```c++
bool Set(int index, const LuaUserData& lud);
bool Set(const char* name, const LuaUserData& lud);
```

Sets the value at `index` or `name` to be a userdata `lud`. Note that `lud` and the host table **MUST** be created by the same LuaState.

```c++
bool ForEach(const std::function<bool (const LuaObject& key,
                                       const LuaObject& value)>& func) const;
```

Itarates the table with the callback function `func`.

[[back to top](#table-of-contents)]

## LuaFunction

`LuaFunction`(inherits from `LuaRefObject`) represents the function type of Lua.

```c++
class LuaFunctionHelper {
public:
    virtual ~LuaFunctionHelper() {}
    virtual bool BeforeProcess(int nresults) = 0;
    virtual bool Process(int i, const LuaObject&) = 0;
    virtual void AfterProcess() = 0;
};

template<typename ... Argv>
bool Exec(std::string* errstr = nullptr, LuaFunctionHelper* helper = nullptr,
          const Argv&... argv);
```

Invokes the function with arguments `argv`. The first argument `errstr` is a string to receive a message if an error occurs. The second argument `helper`, which points to an instance of `LuaFunctionHelper`, is used to handle result(s). The rest of arguments `argv`, if any, are passed to the real function being called. Note that the first argument `i` of `LuaFunctionHelper::Process()` counts from 0.

[[back to top](#table-of-contents)]

## LuaClass

`LuaClass`(inherits from `LuaRefObject`) is used to export C++ classes and member functions to Lua. It does not support exporting member variables.

```c++
template<typename ... FuncArgType>
LuaClass<T>& SetConstructor();
```

Sets the class's constructor with argument type `FuncArgType` and return a reference of the class itself.

```c++
template<typename FuncRetType, typename ... FuncArgType>
LuaClass<T>& Set(const char* funcname,
                 FuncRetType (T::*func)(FuncArgType...));
```

Exports member function `func` to be a member function of this class, with function name `funcname`.

```c++
template<typename FuncRetType, typename ... FuncArgType>
LuaClass<T>& Set(const char* funcname,
                 FuncRetType (T::*func)(FuncArgType...) const);
```

Exports member function `func`(with `const` qualifier) to be a member function of this class with function name `funcname`.

```c++
template<typename FuncRetType, typename ... FuncArgType>
LuaClass<T>& Set(const char* funcname,
                 FuncRetType (*func)(FuncArgType...));
```

Exports static member function `func` to be a member function of this class with function name `funcname`.

```c++
LuaClass<T>& Set(const char* funcname, int (*func)(lua_State*));
```

Exports a lua-style function `func` to be a member function of this class with function name `funcname`.

[[back to top](#table-of-contents)]

## LuaUserData

`LuaUserData` (inherits from `LuaRefObject`) represents the userdata type in Lua.

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
bool Set(const char* name, const char* str);
```

Sets the variable `name` to be the string `str`.

```c++
bool Set(const char* name, const char* str, size_t len);
```

Sets the variable `name` to be the string `str` with length `len`.

```c++
bool Set(const char* name, lua_Number n);
```

Sets the variable `name` to be the number `n`.

```c++
bool Set(const char* name, const LuaObject& lobj);
```

Sets the variable `name` to be the object `lobj`. Note that `lobj` **MUST** be created by this LuaState.

```c++
bool Set(const char* name, const LuaTable& ltable);
```

Sets the variable `name` to be the table `ltable`. Note that `ltable` **MUST** be created by this LuaState.

```c++
bool Set(const char* name, const LuaFunction& lfunc);
```

Sets the variable `name` to be the function `lfunc`. Note that `lfunc` **MUST** be created by this LuaState.

```c++
bool Set(const char* name, const LuaUserData& lud);
```

Sets the variable `name` to be the userdata `lud`. Note that `lud` **MUST** be created by this LuaState.

```c++
template<typename T>
bool Set(const char* name, const LuaClass<T>& lclass);
```

Sets the variable `name` to be the user-defined class `lclass`. Note that `lclass` **MUST** be created by this LuaState.

```c++
LuaTable CreateTable(const char* name = nullptr);
```

Creates a new table with table name `name`(if not NULL).

```c++
template<typename FuncRetType, typename ... FuncArgType>
LuaFunction CreateFunction(FuncRetType (*)(FuncArgType...),
                           const char* name = nullptr);
```

Creates a new function with function name `name`(if not NULL).

```c++
template<typename FuncRetType, typename ... FuncArgType>
LuaFunction CreateFunction(const std::function<FuncRetType (FuncArgType...)>&,
                           const char* name = nullptr);
```

Creates a new function with function name `name`(if not NULL).

```c++
template<typename T>
LuaClass<T> CreateClass(const char* name = nullptr);
```

Exports a new type `T` with the name `name`. If `T` is already exported, the class is returned.

```c++
template<typename T, typename ... Argv>
LuaUserData CreateUserData(const char* name = nullptr,
                           const Argv&... argv);
```

Creates a `LuaUserData` of type `T` with the name `name`(if not NULL). Arguments `argv` are passed to the constructor of `T` to create an instance. If `T` is not exported, it throws a `std::runtime_error` exception.

```c++
bool DoString(const char* chunk, std::string* errstr = nullptr,
              LuaFunctionHelper* helper = nullptr);
```

Evaluates the chunk `chunk`. The rest of arguments, `errstr` and `helper`, have the same meaning as in `LuaFunction::Exec()`.

```c++
bool DoFile(const char* script, std::string* errstr = nullptr,
            LuaFunctionHelper* helper = nullptr);
```

Loads and evaluates the Lua script `script`. The rest of arguments, `errstr` and `helper`, have the same meaning as in `LuaFunction::Exec()`.

[[back to top](#table-of-contents)]

-----

# FAQ

* The `lua_State` object inside each `LuaState` instance is handled by `std::shared_ptr`, so all objects can be passed around freely.
* Arguments of user-defined types of exported C++ functions must be passed by pointer.
* Only numbers and dynamic allocated objects can be returned from exported C++ functions.
* Class member functions should be called with colon operator, in the form of `object:func()`, to ensure the object itself is the first argument passed to `func`. Otherwise you need to do it manually, like `object.func(object, <other arguments>)`.

[[back to top](#table-of-contents)]

-----

# License

This program is distributed under the MIT License.
