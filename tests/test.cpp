#include <iostream>
#include <map>
#include "../luacpp.h"
using namespace luacpp;
using namespace std;

#undef NDEBUG
#include <assert.h>

static void TestSetGet() {
    LuaState l(luaL_newstate(), true);
    const int value = 5;

    l.Set("var", value);
    auto lobj = l.Get("var");
    assert(lobj.GetType() == LUA_TNUMBER);
    assert(lobj.ToNumber() == value);
}

static void TestNil() {
    LuaState l(luaL_newstate(), true);

    auto lobj = l.Get("nilobj");
    assert(lobj.GetType() == LUA_TNIL);
}

static void TestString() {
    LuaState l(luaL_newstate(), true);

    string var("ouonline");
    l.Set("var", var.c_str(), var.size());
    auto lobj = l.Get("var");
    assert(lobj.GetType() == LUA_TSTRING);
    auto str_ref = lobj.ToString();
    assert(string(str_ref.base, str_ref.size) == var);
}

static void TestTable() {
    LuaState l(luaL_newstate(), true);

    auto iterfunc = [] (const LuaObject& key, const LuaObject& value) -> bool {
        cout << "    ";
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
    LuaState l(luaL_newstate(), true);

    auto lfunc = l.CreateFunction([] (const char* msg) -> int {
        cout << "in std::function Echo(str): '" << msg << "'" << endl;
        return 5;
    }, "Echo");

    const string msg = "calling cpp function with return value from cpp.";
    l.Set("msg", msg.data(), msg.size());

    lfunc.Exec([](int, const LuaObject& lobj) -> bool {
        auto res = lobj.ToInteger<int32_t>();
        assert(res == 5);
        return true;
    }, nullptr, l.Get("msg"));

    string errmsg;
    bool ok = l.DoString("res = Echo('calling cpp function with return value from lua: ');"
                         "io.write('return value -> ', res, '\\n')", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    const string msg2 = "ouonline.net";
    l.DoString("function return2(a, b) return a, b end");
    l.Get("return2").ToFunction().Exec([&msg2](int i, const LuaObject& lobj) -> bool {
        if (i == 0) {
            assert(lobj.ToInteger<int32_t>() == 5);
            cout << "get [0] -> " << lobj.ToInteger<int32_t>() << endl;
        } else if (i == 1) {
            auto str_ref = lobj.ToString();
            const string res2(str_ref.base, str_ref.size);
            assert(res2 == msg2);
            cout << "get [1] -> '" << res2 << "'" << endl;
        }
        return true;
    }, nullptr, 5, msg2.c_str());
}

static void GlobalEcho(const char* msg) {
    cout << msg << endl;
}

static void TestFunctionWithoutReturnValue() {
    LuaState l(luaL_newstate(), true);

    auto lfunc = l.CreateFunction(GlobalEcho, "Echo");
    lfunc.Exec(nullptr, nullptr, "calling cpp function without return value from cpp");

    string errmsg;
    bool ok = l.DoString("Echo('calling cpp function without return value from lua')", &errmsg);
    assert(ok);
    assert(errmsg.empty());
}

class ClassDemo final {
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
    ~ClassDemo() {
        cout << "ClassDemo::~ClassDemo() is called." << endl;
    }

    void Set(const char* msg) {
        m_msg = msg;
        cout << "ClassDemo()::Set(msg): '" << msg << "'" << endl;
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

    static int st_value;

private:
    string m_msg;
};

int ClassDemo::st_value = 12;

static void TestClass() {
    LuaState l(luaL_newstate(), true);

    auto lclass = l.RegisterClass<ClassDemo>("ClassDemo");
    lclass.DefConstructor<const char*, int>()
        .DefMember("set", &ClassDemo::Set)
        .DefMember("print", &ClassDemo::Print);

    string errmsg;
    bool ok = l.DoString("tc = ClassDemo('abc', 5);"
                         "tc:set('test class 1');"
                         "tc:print();", &errmsg);
    assert(ok);
    assert(errmsg.empty());
}

static void TestClassConstructor() {
    LuaState l(luaL_newstate(), true);

    auto lclass = l.RegisterClass<ClassDemo>("ClassDemo");

    lclass.DefConstructor();
    bool ok = l.DoString("tc = ClassDemo()");
    assert(ok);

    lclass.DefConstructor<const char*, int>();
    ok = l.DoString("tc = ClassDemo('ouonline', 5)");
    assert(ok);
}

struct Point final {
    int x = 10;
    int y = 20;
};

static void TestClassProperty() {
    LuaState l(luaL_newstate(), true);
    auto lclass = l.RegisterClass<Point>("Point");
    lclass.DefConstructor();

    lclass.DefMember("x", &Point::x).DefMember("y", &Point::y);

    l.DoString("p = Point();");
    auto p = l.Get("p").ToUserData().Get<Point>();
    assert(p->x == 10);
    assert(p->y == 20);

    l.DoString("print('x = ', p.x);"
               "p.x = 12345; p.y = 54321;");
    assert(p->x == 12345);
    assert(p->y == 54321);
    cout << "in cpp, x = " << p->x << endl;
}

static inline void GenericPrint(const char* msg) {
    cout << "C-style static member function: '" << msg << "'" << endl;
}

static inline void CMemberPrint(ClassDemo*, const char* msg) {
    cout << "C-style member function: '" << msg << "'" << endl;
}

static void TestClassMemberFunction() {
    LuaState l(luaL_newstate(), true);

    l.RegisterClass<ClassDemo>("ClassDemo")
        .DefConstructor()
        .DefMember("set", &ClassDemo::Set)
        .DefMember("print", &ClassDemo::Print)
        .DefMember<void, const char*>("echo_str", &ClassDemo::Echo) // overloaded function
        .DefMember<void, int>("echo_int", &ClassDemo::Echo)
        .DefMember("echo_m", &CMemberPrint)
        .DefMember("echo_s", [](ClassDemo*, const char* msg) -> void {
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

static void TestClassStaticProperty() {
    LuaState l(luaL_newstate(), true);

    l.RegisterClass<ClassDemo>("ClassDemo")
        .DefStatic("st_value", &ClassDemo::st_value);

    bool ok = l.DoString("vvv = ClassDemo.st_value");
    assert(ok);
    auto vvv = l.Get("vvv").ToInteger<int32_t>();
    assert(vvv == ClassDemo::st_value);
    cout << "get st_value = " << vvv << endl;

    ok = l.DoString("ClassDemo.st_value = 1023");
    assert(ok);
    assert(ClassDemo::st_value == 1023);
    cout << "after modification, get st_value = " << ClassDemo::st_value << endl;
}

static void TestClassPropertyReadWrite() {
    LuaState l(luaL_newstate(), true);

    l.RegisterClass<ClassDemo>("ClassDemo")
        .DefStatic("st_value", &ClassDemo::st_value, luacpp::WRITE);

    bool ok = l.DoString("vvv = ClassDemo.st_value");
    assert(ok);
    assert(l.Get("vvv").IsNil());
    cout << "cannot read st_value" << endl;
}

static void TestClassStaticMemberFunction() {
    LuaState l(luaL_newstate(), true);

    l.RegisterClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefStatic("s_echo", &ClassDemo::StaticEcho)
        .DefStatic("s_print", GenericPrint)
        .DefStatic("lambda_print", [](const char* msg) -> void {
            cout << "lambda print(str): '" << msg << "'" << endl;
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

static void TestClassLuaMemberFunction() {
    LuaState l(luaL_newstate(), true);

    l.RegisterClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("print_all_str", &PrintAllStr);

    bool ok = l.DoString("t = ClassDemo('a', 1); t:print_all_str('3', '5', 'ouonline', '1', '2')");
    assert(ok);
}

static void TestClassLuaStaticMemberFunction() {
    LuaState l(luaL_newstate(), true);

    l.RegisterClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefStatic("print_all_str", &PrintAllStr);

    bool ok = l.DoString("t = ClassDemo('b', 8);"
                         "print('********************');"
                         "t:print_all_str('1', 'ouonline', '3');"
                         "print('********************');"
                         "ClassDemo:print_all_str('2', '5', 'ouonline', '6');"
                         "print('********************');");
    assert(ok);
}

static void TestUserdata1() {
    LuaState l(luaL_newstate(), true);

    l.RegisterClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("get_object_addr", [](ClassDemo* obj) -> ClassDemo* {
            return obj;
        })
        .DefMember("print", &ClassDemo::Print);

    auto ud = l.CreateUserData("ClassDemo", "tc", "ouonline", 5).Get<ClassDemo>();
    ud->Set("in lua: Print test data from cpp");
    l.DoString("tc:print()");

    l.DoString("obj = tc:get_object_addr();");
    auto obj_addr = l.Get("obj").ToUserData().Get<ClassDemo>();

    assert(ud == obj_addr);
}

static void TestUserdata2() {
    LuaState l(luaL_newstate(), true);

    l.RegisterClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("set", &ClassDemo::Set);
    l.DoString("tc = ClassDemo('ouonline', 3); tc:set('in cpp: Print test data from lua')");
    l.Get("tc").ToUserData().Get<ClassDemo>()->Print();
}

static void TestDoString() {
    LuaState l(luaL_newstate(), true);
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
    LuaState l(luaL_newstate(), true);
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
    {"TestClassProperty", TestClassProperty},
    {"TestClassMemberFunction", TestClassMemberFunction},
    {"TestClassLuaMemberFunction", TestClassLuaMemberFunction},
    {"TestClassStaticProperty", TestClassStaticProperty},
    {"TestClassStaticMemberFunction", TestClassStaticMemberFunction},
    {"TestClassLuaStaticMemberFunction", TestClassLuaStaticMemberFunction},
    {"TestClassPropertyReadWrite", TestClassPropertyReadWrite},
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

    cout << "All tests are passed." << endl;
    return 0;
}
