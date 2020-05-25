#include <iostream>
#include "lua-cpp/luacpp.h"
using namespace luacpp;
using namespace std;

static void test_number() {
    LuaState l;

    l.Set("var", 5);
    auto lobj = l.Get("var");
    cout << "var: type -> " << lobj.GetTypeStr()
         << ", value = " << lobj.ToNumber() << endl;
}

static void test_nil() {
    LuaState l;

    auto lobj = l.Get("nilobj");
    cout << "nilobj: type -> " << lobj.GetTypeStr() << endl;
}

static void test_string() {
    LuaState l;

    string var("ouonline");
    l.Set("var", var.c_str(), var.size());
    auto lobj = l.Get("var");
    cout << "var: type -> " << lobj.GetTypeStr()
         << ", value = " << lobj.ToString() << endl;
}

static void test_table() {
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
}

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

static void test_function_with_return_value() {
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
}

static void Echo(const char* msg) {
    cout << msg;
}

static void test_function_without_return_value() {
    LuaState l;

    auto lfunc = l.CreateFunction(Echo, "Echo");
    lfunc.Exec(0, nullptr, nullptr,
               "calling cpp function without return value from cpp\n");

    l.DoString("Echo('calling cpp function without return value from lua\\n')");
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

    void Set(const char* msg) { m_msg = msg; }

    void Print() const {
        cout << "TestClass::Print(): " << m_msg << endl;
    }
    void Echo(const char* msg) const {
        cout << "TestClass::Echo(string): " << msg << endl;
    }
    void Echo(int v) const {
        cout << "TestClass::Echo(int): " << v << endl;
    }
    static void StaticEcho(const char* msg) {
        cout << "TestClass::StaticEcho(string): " << msg << endl;
    }

private:
    string m_msg;
};

static void test_class() {
    LuaState l;

    l.CreateClass<TestClass>("TestClass")
        .SetConstructor<const char*, int>()
        .Set("set", &TestClass::Set)
        .Set("Print", &TestClass::Print);

    l.CreateClass<TestClass>("TestClass2");

    l.DoString("tc1 = TestClass();"
               "tc1:set('test class 1');"
               "tc1:print();"
               "tc2 = TestClass2();"
               "tc2:set('duplicated test class 2');"
               "tc2:print()");
}

static void test_class_constructor() {
    LuaState l;

    auto lclass = l.CreateClass<TestClass>("TestClass").SetConstructor();
    l.DoString("tc = TestClass()");

    lclass.SetConstructor<const char*, int>();
    l.DoString("tc = TestClass('ouonline', 5)");
}

static void test_class_member_function() {
    LuaState l;

    l.CreateClass<TestClass>("TestClass")
        .SetConstructor<const char*, int>()
        .Set("set", &TestClass::Set)
        .Set("Print", &TestClass::Print)
        .Set<void, const char*>("echo_str", &TestClass::Echo) // overloaded function
        .Set<void, int>("echo_int", &TestClass::Echo);

    l.DoString("tc = TestClass();" // calling TestClass::TestClass(const char*, int) with default values provided by Lua
               "tc:set('content from lua'); tc:Print();"
               "tc:echo_str('calling class member function from lua')");
}

static void test_class_static_member_function() {
    LuaState l;
    string errstr;

    auto lclass = l.CreateClass<TestClass>("TestClass")
        .Set("s_echo", &TestClass::StaticEcho);

    bool ok = l.DoString("TestClass:s_echo('static member function is called without being instantiated');"
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

    auto lclass = l.CreateClass<TestClass>("TestClass")
        .SetConstructor<const char*, int>()
        .Set("print_all_str", &test_print_all_str);

    bool ok = l.DoString("t = TestClass('a', 1); t:print_all_str('3', '5', 'ouonline', '1', '2')",
                         &errstr);
    if (!ok) {
        cerr << "error: " << errstr << endl;
    }
}

static void test_userdata_1() {
    LuaState l;

    l.CreateClass<TestClass>("TestClass")
        .SetConstructor<const char*, int>()
        .Set("Print", &TestClass::Print);
    l.CreateUserData<TestClass>("tc", "ouonline", 5).Get<TestClass>()->Set("in lua: Print test data from cpp");
    l.DoString("tc:Print()");
}

static void test_userdata_2() {
    LuaState l;

    l.CreateClass<TestClass>("TestClass")
        .SetConstructor<const char*, int>()
        .Set("set", &TestClass::Set);
    l.DoString("tc = TestClass('ouonline', 5); tc:set('in cpp: Print test data from lua')");
    l.Get("tc").ToUserData().Get<TestClass>()->Print();
}

static void test_dostring() {
    LuaState l;
    string errstr;

    auto resiter = [] (int n, const LuaObject& lobj) -> bool {
        cout << "output from resiter: ";
        if (n == 0) {
            cout << lobj.ToString() << endl;
        } else if (n == 1) {
            cout << lobj.ToNumber() << endl;
        }

        return true;
    };
    GenericFunctionHelper helper(resiter);

    if (!l.DoString("return 'ouonline', 5", &errstr, &helper)) {
        cerr << "DoString() failed: " << errstr << endl;
    }
}

static void test_misc() {
    LuaState l;

    string errstr;
    if (!l.DoFile(__FILE__, &errstr)) {
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
