#include <iostream>
#include <map>
#include "../luacpp.h"
using namespace luacpp;
using namespace std;

#undef NDEBUG
#include <assert.h>

static void TestSetGet() {
    LuaState l;
    const int value = 5;

    l.Set("var", value);
    auto lobj = l.Get("var");
    assert(lobj.GetType() == LUA_TNUMBER);
    assert(lobj.ToNumber() == value);
}

static void TestNil() {
    LuaState l;

    auto lobj = l.Get("nilobj");
    assert(lobj.GetType() == LUA_TNIL);
}

static void TestString() {
    LuaState l;

    string var("ouonline");
    l.Set("var", var.c_str(), var.size());
    auto lobj = l.Get("var");
    assert(lobj.GetType() == LUA_TSTRING);
    auto str_ref = lobj.ToString();
    assert(string(str_ref.base, str_ref.size) == var);
}

static void TestTable() {
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

    auto table = l.Get("var").ToTable();
    table.ForEach(iterfunc);

    auto lobj = table.Get("others");
    assert(lobj.GetType() == LUA_TSTRING);
    auto str_ref = lobj.ToString();
    assert(string(str_ref.base, str_ref.size) == "myothers");

    cout << "table2:" << endl;
    auto ltable = l.CreateTable();
    ltable.Set("x", 5);
    ltable.Set("o", "ouonline");
    ltable.Set("t", ltable);
    ltable.ForEach(iterfunc);
}

static void TestFunctionWithReturnValue() {
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
    l.CreateFunction("Echo", Echo);
    auto lfunc = l.Get("Echo").ToFunction();

    l.Set("msg", "calling cpp function with return value from cpp: ");
    lfunc.Exec(resiter1, nullptr, l.Get("msg"));

    l.DoString("res = Echo('calling cpp function with return value from lua: ');"
               "io.write('return value -> ', res, '\\n')");

    l.DoString("function return2(a, b) return a, b end");
    l.Get("return2").ToFunction().Exec(resiter2, nullptr, 5, "ouonline");
}

static void Echo(const char* msg) {
    cout << msg << endl;
}

static void TestFunctionWithoutReturnValue() {
    LuaState l;

    l.CreateFunction("Echo", Echo);
    auto lfunc = l.Get("Echo").ToFunction();
    lfunc.Exec(nullptr, nullptr,
               "calling cpp function without return value from cpp");

    l.DoString("Echo('calling cpp function without return value from lua')");
}

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

private:
    string m_msg;
};

static void TestClass() {
    LuaState l;

    auto lclass = l.RegisterClass<ClassDemo>("ClassDemo");
    lclass.SetConstructor<const char*, int>()
        .SetMemberFunction("set", &ClassDemo::Set)
        .SetMemberFunction("print", &ClassDemo::Print);

    bool ok = l.DoString("tc = ClassDemo('abc', 5);"
                         "tc:set('test class 1');"
                         "tc:print();");
    assert(ok);
}

static void TestClassConstructor() {
    LuaState l;

    auto lclass = l.RegisterClass<ClassDemo>("ClassDemo");

    lclass.SetConstructor();
    bool ok = l.DoString("tc = ClassDemo()");
    assert(ok);

    lclass.SetConstructor<const char*, int>();
    ok = l.DoString("tc = ClassDemo('ouonline', 5)");
    assert(ok);
}

static inline void GenericPrint(const char* msg) {
    cout << "C-style static member function: '" << msg << "'" << endl;
}

static inline void CMemberPrint(void*, const char* msg) {
    cout << "C-style member function: '" << msg << "'" << endl;
}

static void TestClassMemberFunction() {
    LuaState l;

    l.RegisterClass<ClassDemo>("ClassDemo")
        .SetConstructor()
        .SetMemberFunction("set", &ClassDemo::Set)
        .SetMemberFunction("print", &ClassDemo::Print)
        .SetMemberFunction<void, const char*>("echo_str", &ClassDemo::Echo) // overloaded function
        .SetMemberFunction<void, int>("echo_int", &ClassDemo::Echo)
        .SetMemberFunction("echo_m", &CMemberPrint)
        .SetMemberFunction("echo_s", [](void*, const char* msg) -> void {
            cout << "lambda print(str): " << msg << endl;
        });

    bool ok = l.DoString("tc = ClassDemo();"
                         "tc:set('content from lua'); tc:print();"
                         "tc:echo_str('called by instance');"
                         "tc:echo_m('called by instance');"
                         "tc:echo_s('called by instance')");
    assert(ok);

    string errmsg;
    ok = l.DoString("ClassDemo:lambda_print('error!')", &errmsg);
    assert(!ok);
    assert(!errmsg.empty());
    cerr << "error: " << errmsg << endl;
}

static void TestClassStaticMemberFunction() {
    LuaState l;

    l.RegisterClass<ClassDemo>("ClassDemo")
        .SetConstructor<const char*, int>()
        .SetStaticFunction("s_echo", &ClassDemo::StaticEcho)
        .SetStaticFunction("s_print", GenericPrint)
        .SetStaticFunction("lambda_print", [](const char* msg) -> void {
            cout << "lambda print(msg): '" << msg << "'" << endl;
        });

    bool ok = l.DoString("ClassDemo:s_echo('called by class');"
                         "tc = ClassDemo('ouonline', 5);"
                         "tc:s_echo('called by instance');"
                         "ClassDemo:s_print('called by class');"
                         "tc:s_print('called by instance');"
                         "ClassDemo:lambda_print('called by class');"
                         "tc:lambda_print('called by instance');");
    assert(ok);
}

static int PrintAllStr(lua_State* l) {
    int argc = lua_gettop(l);
    for (int i = 2; i <= argc; ++i) {
        const char* s = lua_tostring(l, i);
        cout << "arg[" << i - 2 << "] -> " << s << endl;
    }
    return 0;
}

static void TestClassCommonLuaMemberFunction() {
    LuaState l;

    l.RegisterClass<ClassDemo>("ClassDemo")
        .SetConstructor<const char*, int>()
        .SetMemberFunction("print_all_str", &PrintAllStr);

    bool ok = l.DoString("t = ClassDemo('a', 1); t:print_all_str('3', '5', 'ouonline', '1', '2')");
    assert(ok);
}

static void TestClassCommonLuaStaticMemberFunction() {
    LuaState l;

    l.RegisterClass<ClassDemo>("ClassDemo")
        .SetConstructor<const char*, int>()
        .SetStaticFunction("print_all_str", &PrintAllStr);

    bool ok = l.DoString("t = ClassDemo('b', 8);"
                         "print('********************');"
                         "t:print_all_str('1', 'ouonline', '3');"
                         "print('********************');"
                         "ClassDemo:print_all_str('2', '5', 'ouonline', '6');"
                         "print('********************');");
    assert(ok);
}

static void TestUserdata1() {
    LuaState l;

    l.RegisterClass<ClassDemo>("ClassDemo")
        .SetConstructor<const char*, int>()
        .SetMemberFunction("get_object_addr", [](void* obj) -> void* {
            return obj;
        })
        .SetMemberFunction("print", &ClassDemo::Print);

    auto ud = l.CreateUserData("ClassDemo", "tc", "ouonline", 5).Get<ClassDemo>();
    ud->Set("in lua: Print test data from cpp");
    l.DoString("tc:print()");

    l.DoString("obj = tc:get_object_addr();");
    auto obj_addr = l.Get("obj").ToUserData().Get<void>();

    assert(ud == obj_addr);
}

static void TestUserdata2() {
    LuaState l;

    l.RegisterClass<ClassDemo>("ClassDemo")
        .SetConstructor<const char*, int>()
        .SetMemberFunction("set", &ClassDemo::Set);
    l.DoString("tc = ClassDemo('ouonline', 3); tc:set('in cpp: Print test data from lua')");
    l.Get("tc").ToUserData().Get<ClassDemo>()->Print();
}

static void TestDoString() {
    LuaState l;
    string errstr;
    bool ok = l.DoString("return 'ouonline', 5", &errstr,
                         [] (int n, const LuaObject& lobj) -> bool {
                             cout << "output from resiter: ";
                             if (n == 0) {
                                 auto str_ref = lobj.ToString();
                                 cout << string(str_ref.base, str_ref.size) << endl;
                             } else if (n == 1) {
                                 cout << lobj.ToNumber() << endl;
                             }

                             return true;
                         });
    assert(ok);
    assert(errstr.empty());
}

static void TestDoFile() {
    LuaState l;
    string errstr;
    assert(!l.DoFile(__FILE__, &errstr));
    assert(!errstr.empty());
    cerr << "errmsg -> " << errstr << endl;
}

static const map<string, void (*)()> g_test_suite = {
    {"TestSetGet", TestSetGet},
    {"TestNil", TestNil},
    {"TestString", TestString},
    {"TestTable", TestTable},
    {"TestFunctionWithReturnValue", TestFunctionWithReturnValue},
    {"TestFunctionWithoutReturnValue", TestFunctionWithoutReturnValue},
    {"TestClass", TestClass},
    {"TestClassConstructor", TestClassConstructor},
    {"TestClassMemberFunction", TestClassMemberFunction},
    {"TestClassStaticMemberFunction", TestClassStaticMemberFunction},
    {"TestClassCommonLuaMemberFunction", TestClassCommonLuaMemberFunction},
    {"TestClassCommonLuaStaticMemberFunction", TestClassCommonLuaStaticMemberFunction},
    {"TestUserdata1", TestUserdata1},
    {"TestUserdata2", TestUserdata2},
    {"TestDoString", TestDoString},
    {"TestDoFile", TestDoFile},
};

int main(void) {
    for (auto x : g_test_suite) {
        cout << "-------------------- " << x.first << " --------------------" << endl;
        x.second();
    }
    cout << "--------------------------------------------" << endl;

    cout << "all tests are passed." << endl;
    return 0;
}
