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

    auto lobj = l.CreateObject(value);
    assert(lobj.Type() == LUA_TNUMBER);
    assert(lobj.ToNumber() == value);
}

static void TestNil() {
    LuaState l(luaL_newstate(), true);

    auto lobj = l.Get("nilobj");
    assert(lobj.Type() == LUA_TNIL);
}

static void TestString() {
    LuaState l(luaL_newstate(), true);

    const string var("ouonline");
    auto lobj = l.CreateObject(var.c_str(), var.size(), "var");
    assert(lobj.Type() == LUA_TSTRING);

    auto buf = lobj.ToBufferRef();
    assert(string(buf.base, buf.size) == var);

    auto lobj2 = l.Get("var");
    auto buf2 = lobj2.ToBufferRef();
    assert(string(buf2.base, buf2.size) == var);
}

static void TestTable() {
    LuaState l(luaL_newstate(), true);

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

    LuaTable table(l.Get("var"));
    table.ForEach(iterfunc);

    auto lobj = table.Get("others");
    assert(lobj.Type() == LUA_TSTRING);
    auto buf = lobj.ToBufferRef();
    assert(string(buf.base, buf.size) == "myothers");

    cout << "table2:" << endl;
    auto ltable = l.CreateTable();
    ltable.Set("x", 5);
    ltable.Set("o", "ouonline");
    ltable.Set("t", table);
    ltable.ForEach(iterfunc);
}

static void TestFunctionWithReturnValue() {
    LuaState l(luaL_newstate(), true);

    auto lfunc = l.CreateFunction([] (const char* msg) -> int {
        cout << "in std::function Echo(str): '" << msg << "'" << endl;
        return 5;
    }, "Echo");

    const string msg = "calling cpp function with return value from cpp.";
    l.CreateObject(msg.data(), msg.size(), "msg");

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
    LuaFunction(l.Get("return2")).Exec([&msg2](int i, const LuaObject& lobj) -> bool {
        if (i == 0) {
            assert(lobj.ToInteger<int32_t>() == 5);
            cout << "get [0] -> " << lobj.ToInteger<int32_t>() << endl;
        } else if (i == 1) {
            auto buf = lobj.ToBufferRef();
            const string res2(buf.base, buf.size);
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

    auto lclass = l.CreateClass<ClassDemo>("ClassDemo");
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

    auto lclass = l.CreateClass<ClassDemo>("ClassDemo");

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
    auto lclass = l.CreateClass<Point>("Point");
    lclass.DefConstructor();

    lclass.DefMember("x", &Point::x).DefMember("y", &Point::y);

    auto p = l.CreateUserData("Point", "p").Get<Point>();
    assert(p->x == 10);
    assert(p->y == 20);

    l.DoString("print('x = ', p.x);"
               "p.x = 12345; p.y = 54321;");
    assert(p->x == 12345);
    assert(p->y == 54321);
    cout << "in cpp, point is [" << p->x << ", " << p->y << "]" << endl;
}

static inline void GenericPrint(const char* msg) {
    cout << "C-style static member function: '" << msg << "'" << endl;
}

static inline void CMemberPrint(ClassDemo*, const char* msg) {
    cout << "C-style member function: '" << msg << "'" << endl;
}

static void TestClassMemberFunction() {
    LuaState l(luaL_newstate(), true);

    l.CreateClass<ClassDemo>("ClassDemo")
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

    l.CreateClass<ClassDemo>("ClassDemo")
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

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefStatic("st_value", &ClassDemo::st_value, luacpp::WRITE);

    bool ok = l.DoString("vvv = ClassDemo.st_value");
    assert(ok);
    assert(l.Get("vvv").Type() == LUA_TNIL);
    cout << "cannot read st_value" << endl;
}

static void TestClassStaticMemberFunction() {
    LuaState l(luaL_newstate(), true);

    l.CreateClass<ClassDemo>("ClassDemo")
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

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("print_all_str", &PrintAllStr);

    bool ok = l.DoString("t = ClassDemo('a', 1); t:print_all_str('3', '5', 'ouonline', '1', '2')");
    assert(ok);
}

static void TestClassLuaStaticMemberFunction() {
    LuaState l(luaL_newstate(), true);

    l.CreateClass<ClassDemo>("ClassDemo")
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

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("get_object_addr", [](ClassDemo* obj) -> ClassDemo* {
            return obj;
        })
        .DefMember("print", &ClassDemo::Print);

    auto ud = l.CreateUserData("ClassDemo", "tc", "ouonline", 5).Get<ClassDemo>();
    ud->Set("in lua: Print test data from cpp");
    l.DoString("tc:print()");

    l.DoString("obj = tc:get_object_addr();");
    auto obj_addr = LuaUserData(l.Get("obj")).Get<ClassDemo>();

    assert(ud == obj_addr);
}

static void TestUserdata2() {
    LuaState l(luaL_newstate(), true);

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("set", &ClassDemo::Set);
    l.DoString("tc = ClassDemo('ouonline', 3); tc:set('in cpp: Print test data from lua')");
    LuaUserData(l.Get("tc")).Get<ClassDemo>()->Print();
}

static void TestDoString() {
    LuaState l(luaL_newstate(), true);
    string errstr;
    bool ok = l.DoString("return 'ouonline', 5", &errstr,
                         [] (int n, const LuaObject& lobj) -> bool {
                             cout << "output from resiter: ";
                             if (n == 0) {
                                 auto buf = lobj.ToBufferRef();
                                 cout << buf.base << endl;
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
