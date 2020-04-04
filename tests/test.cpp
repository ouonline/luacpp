#include <iostream>
#include "lua-cpp/luacpp.h"
using namespace luacpp;
using namespace std;

static void test_number() {
    LuaState l;

    l.set("var", 5);
    auto lobj = l.get("var");
    cout << "var: type -> " << lobj.typestr()
         << ", value = " << lobj.tonumber() << endl;
}

static void test_nil() {
    LuaState l;

    auto lobj = l.get("nilobj");
    cout << "nilobj: type -> " << lobj.typestr() << endl;
}

static void test_string() {
    LuaState l;

    string var("ouonline");
    l.set("var", var.c_str(), var.size());
    auto lobj = l.get("var");
    cout << "var: type -> " << lobj.typestr()
         << ", value = " << lobj.tostring() << endl;
}

static void test_table() {
    LuaState l;

    auto iterfunc = [] (const LuaObject& key, const LuaObject& value) -> bool {
        cout << "    "; // indention
        if (key.type() == LUA_TNUMBER) {
            cout << key.tonumber();
        } else if (key.type() == LUA_TSTRING) {
            cout << key.tostring();
        } else {
            cout << "unsupported key type -> " << key.typestr() << endl;
            return false;
        }

        if (value.type() == LUA_TNUMBER) {
            cout << " -> " << value.tonumber() << endl;
        } else if (value.type() == LUA_TSTRING) {
            cout << " -> " << value.tostring() << endl;
        } else {
            cout << " -> unsupported iter value type: " << value.typestr() << endl;
        }

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
}

static void test_function_with_return_value() {
    LuaState l;

    auto resiter1 = [] (int, const LuaObject& lobj) -> bool {
        cout << "output from resiter1: " << lobj.tostring() << endl;
        return true;
    };
    auto resiter2 = [] (int n, const LuaObject& lobj) -> bool {
        cout << "output from resiter2: ";
        if (n == 0) {
            cout << lobj.tonumber() << endl;
        } else if (n == 1) {
            cout << lobj.tostring() << endl;
        }

        return true;
    };

    std::function<int (const char*)> echo = [] (const char* msg) -> int {
        cout << msg;
        return 5;
    };
    auto lfunc = l.newfunction(echo, "echo");

    l.set("msg", "calling cpp function with return value from cpp: ");
    lfunc.exec(1, resiter1, nullptr, l.get("msg"));

    l.dostring("res = echo('calling cpp function with return value from lua: ');"
               "io.write('return value -> ', res, '\\n')");

    l.dostring("function return2(a, b) return a, b end");
    l.get("return2").tofunction().exec(2, resiter2, nullptr, 5, "ouonline");
}

static void echo(const char* msg) {
    cout << msg;
}

static void test_function_without_return_value() {
    LuaState l;

    auto lfunc = l.newfunction(echo, "echo");
    lfunc.exec(0, nullptr, nullptr,
               "calling cpp function without return value from cpp\n");

    l.dostring("echo('calling cpp function without return value from lua\\n')");
}

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

    void set(const char* msg) { m_msg = msg; }

    void print() const {
        cout << "TestClass::print(): " << m_msg << endl;
    }
    void echo(const char* msg) const {
        cout << "TestClass::echo(string): " << msg << endl;
    }
    void echo(int v) const {
        cout << "TestClass::echo(int): " << v << endl;
    }
    static void s_echo(const char* msg) {
        cout << "TestClass::s_echo(string): " << msg << endl;
    }

private:
    string m_msg;
};

static void test_class() {
    LuaState l;

    l.newclass<TestClass>("TestClass")
        .setconstructor<const char*, int>()
        .set("set", &TestClass::set)
        .set("print", &TestClass::print);

    l.newclass<TestClass>("TestClass2");

    l.dostring("tc1 = TestClass();"
               "tc1:set('test class 1');"
               "tc1:print();"
               "tc2 = TestClass2();"
               "tc2:set('duplicated test class 2');"
               "tc2:print()");
}

static void test_class_constructor() {
    LuaState l;

    auto lclass = l.newclass<TestClass>("TestClass").setconstructor();
    l.dostring("tc = TestClass()");

    lclass.setconstructor<const char*, int>();
    l.dostring("tc = TestClass('ouonline', 5)");
}

static void test_class_member_function() {
    LuaState l;

    l.newclass<TestClass>("TestClass")
        .setconstructor<const char*, int>()
        .set("set", &TestClass::set)
        .set("print", &TestClass::print)
        .set<void, const char*>("echo_str", &TestClass::echo) // overloaded function
        .set<void, int>("echo_int", &TestClass::echo);

    l.dostring("tc = TestClass();" // calling TestClass::TestClass(const char*, int) with default values provided by Lua
               "tc:set('content from lua'); tc:print();"
               "tc:echo_str('calling class member function from lua')");
}

static void test_class_static_member_function() {
    LuaState l;
    string errstr;

    auto lclass = l.newclass<TestClass>("TestClass")
        .set("s_echo", &TestClass::s_echo);

    bool ok = l.dostring("TestClass:s_echo('static member function is called without being instantiated');"
                         "tc = TestClass('ouonline', 5);" // error: missing constructor
                         "tc:s_echo('static member function is called by an instance')",
                         &errstr);
    if (!ok) {
        cerr << "error: " << errstr << endl;
    }
}

static int test_print_all_str(lua_State* l) {
    int argc = lua_gettop(l);
    for (int i = 2; i <= argc; ++i) {
        const char* s = lua_tostring(l, i);
        cout << "arg[" << i - 2 << "] -> " << s << endl;
    }
    return 0;
}

static void test_class_common_lua_member_function() {
    LuaState l;
    string errstr;

    auto lclass = l.newclass<TestClass>("TestClass")
        .setconstructor<const char*, int>()
        .set("print_all_str", &test_print_all_str);

    bool ok = l.dostring("t = TestClass('a', 1); t:print_all_str('3', '5', 'ouonline', '1', '2')",
                         &errstr);
    if (!ok) {
        cerr << "error: " << errstr << endl;
    }
}

static void test_userdata_1() {
    LuaState l;

    l.newclass<TestClass>("TestClass")
        .setconstructor<const char*, int>()
        .set("print", &TestClass::print);
    l.newuserdata<TestClass>("tc", "ouonline", 5).object<TestClass>()->set("in lua: print test data from cpp");
    l.dostring("tc:print()");
}

static void test_userdata_2() {
    LuaState l;

    l.newclass<TestClass>("TestClass")
        .setconstructor<const char*, int>()
        .set("set", &TestClass::set);
    l.dostring("tc = TestClass('ouonline', 5); tc:set('in cpp: print test data from lua')");
    l.get("tc").touserdata().object<TestClass>()->print();
}

static void test_dostring() {
    LuaState l;
    string errstr;

    auto resiter = [] (int n, const LuaObject& lobj) -> bool {
        cout << "output from resiter: ";
        if (n == 0) {
            cout << lobj.tostring() << endl;
        } else if (n == 1) {
            cout << lobj.tonumber() << endl;
        }

        return true;
    };

    if (!l.dostring("return 'ouonline', 5", &errstr, 2, resiter)) {
        cerr << "dostring() failed: " << errstr << endl;
    }
}

static void test_misc() {
    LuaState l;

    string errstr;
    if (!l.dofile(__FILE__, &errstr)) {
        cerr << "loading " << __FILE__ << " failed: " << errstr << endl;
    }
}

static struct {
    const char* name;
    void (*func)();
} test_suite[] = {
    {"test_number", test_number},
    {"test_nil", test_nil},
    {"test_string", test_string},
    {"test_table", test_table},
    {"test_function_with_return_value", test_function_with_return_value},
    {"test_function_without_return_value", test_function_without_return_value},
    {"test_class", test_class},
    {"test_class_constructor", test_class_constructor},
    {"test_class_member_function", test_class_member_function},
    {"test_class_static_member_function", test_class_static_member_function},
    {"test_class_common_lua_member_function", test_class_common_lua_member_function},
    {"test_userdata_1", test_userdata_1},
    {"test_userdata_2", test_userdata_2},
    {"test_dostring", test_dostring},
    {"test_misc", test_misc},
    {nullptr, nullptr},
};

int main(void) {
    for (int i = 0; test_suite[i].func; ++i) {
        cout << "-------------------- "
             << test_suite[i].name
             << " --------------------" << endl;
        test_suite[i].func();
    }

    return 0;
}
