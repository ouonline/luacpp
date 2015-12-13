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
    - [LuaUserdata](#luauserdata)
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

int main(void)
{
    LuaState l;

    l.set("msg", "Hello, luacpp from ouonline!");
    auto lobj = l.get("msg");
    if (lobj.type() == LUA_TSTRING)
        cout << "get msg -> " << lobj.tostring() << endl;
    else
        cerr << "unknown object type -> " << lobj.typestr() << endl;

    return 0;
}
```

In this example, we set the variable `msg`'s value to be a string "Hello, luacpp from ouonline!", then use the getter function to fetch its value and print it.

`LuaState::set()`s are a series of overloaded functions, which can be used to set up various kinds of variables. Once the variable is set, its value is kept in the `LuaState` instance until it is modified again or the `LuaState` instance is destroyed.

`LuaState::get()` is used to get a variable by its name. The value returned by `LuaState::get()` is a `LuaObject` instance, which can be converted to the proper type later. You'd better check the return value of `LuaObject::type()` before any conversions.

[[back to top](#table-of-contents)]

## Processing Tables

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void)
{
    LuaState l;

    auto iterfunc = [] (const LuaObject& key, const LuaObject& value) -> bool {
        cout << "    "; // indention
        if (key.type() == LUA_TNUMBER)
            cout << key.tonumber();
        else if (key.type() == LUA_TSTRING)
            cout << key.tostring();
        else {
            cout << "unsupported key type -> " << key.typestr() << endl;
            return false;
        }

        if (value.type() == LUA_TNUMBER)
            cout << " -> " << value.tonumber() << endl;
        else if (value.type() == LUA_TSTRING)
            cout << " -> " << value.tostring() << endl;
        else
            cout << " -> unsupported iter value type: " << value.typestr() << endl;

        return true;
    };

    cout << "table1:" << endl;
    l.dostring("var = {'mykey', value = 'myvalue', others = 'myothers'}");
    l.get("var").totable().foreach(iterfunc);

    cout << "table2:" << endl;
    auto ltable = l.newtable();
    ltable.set("x", 5);
    ltable.set("o", "ouonline");
    ltable.set("t", ltable);
    ltable.foreach(iterfunc);

    return 0;
}
```

At first we use `LuaState::dostring()` to execute a chunk that creates a table named `var` with 3 fields.

`LuaTable::foreach()` takes a callback function that is used to iterate each key-value pair in the table. If the callback function returns `false`, `LuaTable::foreach()` exits and returns `false`.

We can use `LuaState::newtable()` to create a new empty table and use `LuaState::set()` to set fields of this table. Like `LuaState::set()`, `LuaTable::set()`s are a series of overloaded functions used to handle various kinds of data types.

[[back to top](#table-of-contents)]

## Calling Functions

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void)
{
    LuaState l;

    vector<LuaObject> res;
    typedef int (*func_t)(const char*);

    func_t echo = [] (const char* msg) -> int {
        cout << msg;
        return 5;
    };
    auto lfunc = l.newfunction(echo, "echo");

    l.set("msg", "calling cpp function with return value from cpp");
    lfunc.exec(1, &res, nullptr, l.get("msg"));
    cout << ", return value -> " << res[0].tonumber() << endl;

    l.dostring("res = echo('calling cpp function with return value from lua');"
               "io.write(', return value -> ', res, '\\n')");

    res.clear();
    l.dostring("function return2(a, b) return a, b end");
    l.get("return2").tofunction().exec(2, &res, nullptr, 2, 3);
    cout << "calling lua funciont from cpp:" << endl;
    for (unsigned int i = 0; i < res.size(); ++i)
        cout << "    res[" << i << "] -> " << res[i].tonumber() << endl;

    return 0;
}
```

First we define a C++ function `echo` and set its name to be `echo` in the Lua environment using `LuaState::newfunction()`.

`LuaFunction::exec()`, which takes as least 3 arguments, invokes the real function. The first argument is an integer `nresults`, the number of return values of this function. The second is a pointer to `std::vector<LuaObject>`, which is used to store return value(s). If it is `nullptr`, all return values are discarded. The third is a porinter to `std::string`, which is used to store error message. The rest of arguments, if any, are passed to the real function being called.

[[back to top](#table-of-contents)]

## Exporting and Using User-defined Types

First we define a class `TestClass` for test:

```c++
class TestClass {

    public:

        TestClass()
        {
            cout << "TestClass::TestClass() is called without value." << endl;
        }

        TestClass(const char* msg, int x)
        {
            if (msg)
                m_msg = msg;

            cout << "TestClass::TestClass() is called with string -> '"
                << m_msg << "' and value -> " << x << "." << endl;
        }

        virtual ~TestClass()
        {
            cout << "TestClass::~TestClass() is called." << endl;
        }

        void set(const char* msg) { m_msg = msg; }

        void print() const
        {
            cout << "TestClass::print(): " << m_msg << endl;
        }

        void echo(const char* msg) const
        {
            cout << "TestClass::echo(string): " << msg << endl;
        }

        void echo(int v) const
        {
            cout << "TestClass::echo(int): " << v << endl;
        }

        static void s_echo(const char* msg)
        {
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

int main(void)
{
    LuaState l;

    // NOTE: export only once
    auto lclass = l.newclass<TestClass>("TestClass").setconstructor();

    cout << "--------------------------------------------" << endl;
    l.dostring("tc = TestClass()");

    cout << "--------------------------------------------" << endl;
    lclass.setconstructor<const char*, int>();
    l.dostring("tc = TestClass('ouonline', 5)");

    cout << "--------------------------------------------" << endl;
    lclass.set("print", &TestClass::print)
        .set<void, const char*>("echo_str", &TestClass::echo) // overloaded function
        .set<void, int>("echo_int", &TestClass::echo);
    l.dostring("tc = TestClass('ouonline', 5); tc:print();"
               "tc:echo_str('calling class member function from lua')");

    cout << "--------------------------------------------" << endl;
    lclass.set("s_echo", &TestClass::s_echo);
    l.dostring("TestClass:s_echo('static member function is called without being instantiated');"
               "tc = TestClass(); tc:s_echo('static member function is called by an instance')");

    cout << "--------------------------------------------" << endl;

    return 0;
}
```

`LuaState::newclass()` is used to export user-defined classes to Lua. It requires a string `name` as the class's name in the Lua environment, and adds a default constructor and a destructor for this class. If the class is already exported, `LuaState::newclass()` throws a `std::runtime_error` exception.

`LuaClass::set()` is a template function used to export member functions for this class.

Class member functions should be called with colon operator, in the form of `object:func()`, to ensure the object itself is the first argument passed to `func`. Otherwise you need to do it manually, like `object.func(object, <other arguments>)`.

The following program displays how to use `LuaUserdata` to exchange data between C++ and Lua.

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void)
{
    LuaState l;

    l.newclass<TestClass>("TestClass").set("print", &TestClass::print);
    l.newuserdata<TestClass>("tc").object<TestClass>()->set("in lua: print test data from cpp");
    l.dostring("tc:print()");

    return 0;
}
```

We create a `LuaUserdata` object of type `TestClass` by calling `LuaClass::newuserdata()` and set its name to be `tc` in the Lua environment. Then we set its content to be a string, which is printed by calling `LuaState::dostring()`.

The following example is similar to the previous one, except that we modify the object's content in the Lua environment but print it in C++.

```c++
#include <iostream>
using namespace std;

#include "luacpp.hpp"
using namespace luacpp;

int main(void)
{
    LuaState l;

    l.newclass<TestClass>("TestClass").set("set", &TestClass::set);
    l.dostring("tc = TestClass(); tc:set('in cpp: print test data from lua')");
    l.get("tc").touserdata().object<TestClass>()->print();

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
int type() const;
```

Gets the type of this object. The return value is one of the following: `LUA_TNIL`, `LUA_TNUMBER`, `LUA_TBOOLEAN`, `LUA_TSTRING`, `LUA_TTABLE`, `LUA_TFUNCTION`, `LUA_TUSERDATA`, `LUA_TTHREAD`, and `LUA_TLIGHTUSERDATA`.

```c++
const char* typestr() const;
```

Returns the name of the object's type.

[[back to top](#table-of-contents)]

## LuaObject

`LuaObject`(inherits from `LuaRefObject`) represents an arbitrary item of Lua. You are expected to check the return value of `type()` before any of the following functions is called.

```c++
std::string tostring() const;
```

Converts this object to a `std::string` object.

```c++
lua_Number tonumber() const;
```

Converts this object to a number.

```c++
LuaTable totable() const;
```

Converts this object to a `LuaTable` object.

```c++
LuaFunction tofunction() const;
```

Converts this object to a `LuaFunction` object.

```c++
LuaUserdata touserdata() const;
```

Converts this object to a `LuaUserdata` object.

[[back to top](#table-of-contents)]

## LuaTable

`LuaTable`(inherits from `LuaRefObject`) represents the table type of Lua.

```c++
LuaObject get(int index) const;
LuaObject get(const char* name) const;
```

Gets an object by its `index` or `name` in this table.

```c++
void set(int index, const char* str);
void set(const char* name, const char* str);
```

Sets the value at `index` or `name` to be the string `str`.

```c++
void set(int index, const char* str, size_t len);
void set(const char* name, const char* str, size_t len)
```

Sets the value at `index` or `name` to be the string `str` with length `len`.

```c++
void set(int index, lua_Number n);
void set(const char* name, lua_Number n);
```

Sets the value at `index` or `name` to be the number `n`.

```c++
bool set(int index, const LuaObject& lobj);
bool set(const char* name, const LuaObject& lobj)
```

Sets the value at `index` or `name` to be the object `lobj`. Note that `lobj` and this table **MUST** be generated by the same LuaState.

```c++
bool set(int index, const LuaTable& ltable);
bool set(const char* name, const LuaTable& ltable);
```

Sets the value at `index` or `name` to be the table `ltable`. Note that `ltable` and the host table **MUST** be generated by the same LuaState.

```c++
bool set(int index, const LuaFunction& lfunc);
bool set(const char* name, const LuaFunction& lfunc);
```

Sets the value at `index` or `name` to be the function `lfunc`. Note that `lfunc` and the host table **MUST** be generated by the same LuaState.

```c++
bool set(int index, const LuaUserdata& lud);
bool set(const char* name, const LuaUserdata& lud);
```

Sets the value at `index` or `name` to be a userdata `lud`. Note that `lud` and the host table **MUST** be generated by the same LuaState.

```c++
bool foreach(const std::function<bool (const LuaObject& key,
                                       const LuaObject& value)>& func) const;
```

Itarates the table with the callback function `func`.

[[back to top](#table-of-contents)]

## LuaFunction

`LuaFunction`(inherits from `LuaRefObject`) represents the function type of Lua.

```c++
template<typename... Argv>
bool exec(int nresults = 0, std::vector<LuaObject>* res = nullptr,
          std::string* errstr = nullptr, const Argv&... argv);
```

Invokes the function with arguments `argv`. The function is expected to return `nresults` results, which are stored in `res` if it is not `nullptr`. If error occurs, error message is stored in `errstr`.

[[back to top](#table-of-contents)]

## LuaClass

`LuaClass`(inherits from `LuaRefObject`) is used to export C++ classes and member functions to Lua. It does not support exporting member variables.

```c++
template<typename... FuncArgType>
LuaClass<T>& setconstructor();
```

Sets the class's constructor with argument type `FuncArgType` and return a reference of the class itself.

```c++
template<typename FuncRetType, typename... FuncArgType>
LuaClass<T>& set(const char* funcname,
                 FuncRetType (T::*func)(FuncArgType...));
```

Exports the member function `func` to be a member function of this class,  with the function name `funcname`.

```c++
template<typename FuncRetType, typename... FuncArgType>
LuaClass<T>& set(const char* funcname,
                 FuncRetType (T::*func)(FuncArgType...) const);
```

Exports the member function `func`(with `const` qualifier) to be a member function of this class  with the function name `funcname`.

```c++
template<typename FuncRetType, typename... FuncArgType>
LuaClass<T>& set(const char* funcname,
                 FuncRetType (*func)(FuncArgType...));
```

Exports the static member function `func` to be a member function of this class  with the function name `funcname`.

[[back to top](#table-of-contents)]

## LuaUserdata

`LuaUserdata` (inherits from `LuaRefObject`) represents the userdata type in Lua.

```c++
template<typename T>
T* object() const;
```

Gets the real data of type `T`.

[[back to top](#table-of-contents)]

## LuaState

```c++
LuaState();
```

The constructor.

```c++
lua_State* ptr() const;
```

Returns the `lua_State` pointer.

```c++
LuaObject get(const char* name) const;
```

Gets an object by its name.

```c++
void set(const char* name, const char* str);
```

Sets the variable `name` to be the string `str`.

```c++
void set(const char* name, const char* str, size_t len);
```

Sets the variable `name` to be the string `str` with length `len`.

```c++
void set(const char* name, lua_Number n);
```

Sets the variable `name` to be the number `n`.

```c++
bool set(const char* name, const LuaObject& lobj);
```

Sets the variable `name` to be the object `lobj`. Note that `lobj` **MUST** be generated by this LuaState.

```c++
bool set(const char* name, const LuaTable& ltable);
```

Sets the variable `name` to be the table `ltable`. Note that `ltable` **MUST** be generated by this LuaState.

```c++
bool set(const char* name, const LuaFunction& lfunc);
```

Sets the variable `name` to be the function `lfunc`. Note that `lfunc` **MUST** be generated by this LuaState.

```c++
bool set(const char* name, const LuaUserdata& lud);
```

Sets the variable `name` to be the userdata `lud`. Note that `lud` **MUST** be generated by this LuaState.

```c++
LuaTable newtable(const char* name = nullptr);
```

Creates a new table with table name `name`(if not NULL).

```c++
template<typename FuncRetType, typename... FuncArgType>
LuaFunction newfunction(FuncRetType (*)(FuncArgType...),
                        const char* name = nullptr);
```

Creates a new function with function name `name`(if not NULL).

```c++
template<typename T>
LuaClass<T> newclass(const char* name);
```

Exports a new type `T` with the name `name`. If `T` is already exported, it throws a `std::runtime_error` exception.

```c++
template<typename T, typename... Argv>
LuaUserdata newuserdata(const char* name = nullptr,
                        const Argv&... argv);
```

Creates a `LuaUserdata` of type `T` with the name `name`(if not NULL). Arguments `argv` are passed to the constructor of `T` to create an instance. If `T` is not exported, it throws a `std::runtime_error` exception.

```c++
bool dostring(const char* chunk, int nresults = 0,
              std::vector<LuaObject>* res = nullptr,
              std::string* errstr = nullptr);
```

Evaluates the chunk `chunk`. Results are stored in `res`. If error occurs, error message is stored in `errstr`.

```c++
bool dofile(const char* script, int nresults = 0,
            std::vector<LuaObject>* res = nullptr,
            std::string* errstr = nullptr);
```

Loads and evaluates the Lua script `script`. Results are stored in `res`. If error occurs, error message is stored in `errstr`.

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
