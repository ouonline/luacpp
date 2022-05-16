#include "test_common.h"

#undef NDEBUG
#include <assert.h>

int ClassDemo::st_value = 42;

static void TestClass() {
    LuaState l(luaL_newstate(), true);

    auto lclass = l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
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

static void TestClassProperty() {
    LuaState l(luaL_newstate(), true);
    auto lclass = l.CreateClass<Point>("Point")
        .DefConstructor()
        .DefMember<int>("x",
                        [](const Point* p) -> int {
                            return p->x;
                        },
                        [](Point* p, int v) -> void {
                            p->x = v;
                        })
        .DefMember<int>("y",
                        [](const Point* p) -> int {
                            return p->y;
                        },
                        [](Point* p, int v) -> void {
                            p->y = v;
                        });

    auto lp1 = lclass.CreateInstance();
    auto p1 = static_cast<Point*>(lp1.ToPointer());
    assert(p1->x == 10);
    assert(p1->y == 20);

    l.Set("p1", lp1);
    l.DoString("p2 = Point();"
               "print('x = ', p1.x);"
               "p1.x = 12345; p1.y = 54321;");
    assert(p1->x == 12345);
    assert(p1->y == 54321);

    auto lp2 = l.Get("p2");
    auto p2 = static_cast<Point*>(lp2.ToPointer());
    assert(p2->x == 10);
    assert(p2->y == 20);

    cout << "in cpp, p1 is [" << p1->x << ", " << p1->y << "], p2 is ["
         << p2->x << ", " << p2->y << "]" << endl;
}

static void TestClassPropertyReadWrite() {
    LuaState l(luaL_newstate(), true);
    auto lclass = l.CreateClass<Point>("Point")
        .DefConstructor()
        // read only
        .DefMember<int>("x",
                        [](const Point* p) -> int {
                            return p->x;
                        },
                        nullptr)
        // write only
        .DefMember<int>("y",
                        nullptr,
                        [](Point* p, int v) -> void {
                            p->y = v;
                        });

    auto lp1 = lclass.CreateInstance();
    auto p1 = static_cast<Point*>(lp1.ToPointer());
    assert(p1->x == 10);
    assert(p1->y == 20);

    l.Set("p1", lp1);
    string errmsg;
    bool ok = l.DoString("print('x = ', p1.x);"
                         "p1.y = 54321; p1.x = 12345;", &errmsg);
    assert(!ok);
    assert(!errmsg.empty());
    cerr << "errmsg -> '" << errmsg << "'" << endl;
    assert(p1->x == 10);
    assert(p1->y == 54321);

    cout << "in cpp, p1 is [" << p1->x << ", " << p1->y << "]" << endl;
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
        .DefMember<const char* (ClassDemo::*)(const char*) const>("echo_str", &ClassDemo::Echo) // overloaded function
        .DefMember<int (ClassDemo::*)(int) const>("echo_int", &ClassDemo::Echo)
        .DefMember("echo_m", &CMemberPrint)
        .DefMember("echo_s", [](ClassDemo*, const char* msg) -> void {
            cout << "lambda print(str): " << msg << endl;
        });

    const string expected_str = "test string in cpp 23333";
    const string chunk = "tc = ClassDemo();"
        "tc:set('content from lua'); tc:print();"
        "var1 = tc:echo_str('" + expected_str + "');"
        "var2 = tc:echo_int(55555);"
        "tc:echo_m('called by instance');"
        "tc:echo_s('called by instance')";

    bool ok = l.DoString(chunk.c_str());
    assert(ok);

    auto buf = l.Get("var1").ToStringRef();
    assert(string(buf.base, buf.size) ==  expected_str);

    auto var2 = l.Get("var2").ToInteger();
    assert(var2 == 55555);

    string errmsg;
    ok = l.DoString("ClassDemo:lambda_print('error!')", &errmsg);
    assert(!ok);
    assert(!errmsg.empty());
    cerr << "error: " << errmsg << endl;
}

static void TestClassStaticProperty() {
    LuaState l(luaL_newstate(), true);

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefStatic<int>("st_value",
                        []() -> int {
                            return ClassDemo::st_value;
                        },
                        [](int v) -> void {
                            ClassDemo::st_value = v;
                        });

    bool ok = l.DoString("vvv = ClassDemo.st_value");
    assert(ok);
    auto vvv = l.Get("vvv").ToInteger();
    assert(vvv == ClassDemo::st_value);
    cout << "get st_value = " << vvv << endl;

    ok = l.DoString("ClassDemo.st_value = 1023");
    assert(ok);
    assert(ClassDemo::st_value == 1023);
    cout << "after modification, get st_value = " << ClassDemo::st_value << endl;
}

static void TestClassStaticPropertyReadWrite() {
    LuaState l(luaL_newstate(), true);

    l.CreateClass<ClassDemo>("ClassDemo")
        .DefStatic<int>("st_value",
                        nullptr,
                        [](int v) -> void {
                            ClassDemo::st_value = v;
                        });

    string errmsg;
    bool ok = l.DoString("vvv = ClassDemo.st_value", &errmsg);
    assert(!ok);
    assert(!errmsg.empty());
    cout << "errmsg -> '" << errmsg << "'" << endl;
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

static void TestClassStaticMemberInheritance() {
    LuaState l(luaL_newstate(), true);

    auto base = l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor()
        .DefStatic("StaticEcho", &ClassDemo::StaticEcho)
        .DefStatic<int>("st_value",
                        []() -> int {
                            return ClassDemo::st_value;
                        },
                        [](int v) -> void {
                            ClassDemo::st_value = v;
                        });

    l.CreateClass<DerivedDemo1>("DerivedDemo1")
        .AddBaseClass(base)
        .DefConstructor();

    string errmsg;
    bool ok = l.DoString("d1 = DerivedDemo1(); print(d1)", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    ok = l.DoString("DerivedDemo1:StaticEcho('derived class')", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    ok = l.DoString("DerivedDemo1.st_value = 35142", &errmsg);
    assert(ok);
    assert(errmsg.empty());
    assert(DerivedDemo1::st_value == 35142);

    ok = l.DoString("d1.st_value = 76532", &errmsg);
    assert(ok);
    assert(errmsg.empty());
    assert(DerivedDemo1::st_value == 76532);
}

static void TestClassMemberInheritance() {
    LuaState l(luaL_newstate(), true);

    auto base = l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor()
        .DefMember<const char* (ClassDemo::*)(const char*) const>("echo", &ClassDemo::Echo)
        .DefStatic("StaticEcho", &ClassDemo::StaticEcho)
        .DefMember<int>("m_value",
                        [](const ClassDemo* c) -> int {
                            return c->m_value;
                        },
                        [](ClassDemo* c, int v) -> void {
                            c->m_value = v;
                        });

    l.CreateClass<DerivedDemo1>("DerivedDemo1")
        .AddBaseClass(base)
        .DefConstructor();

    string errmsg;
    bool ok = l.DoString("d1 = DerivedDemo1(); print(d1)", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    ok = l.DoString("print('d1.m_value = ', d1.m_value)", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    ok = l.DoString("d1.m_value = 35142; print('after update, m_value = ', d1.m_value)", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    auto lud = l.Get("d1");
    assert(static_cast<DerivedDemo1*>(lud.ToPointer())->m_value == 35142);

    ok = l.DoString("d1:StaticEcho('derived instance')", &errmsg);
    assert(ok);
    assert(errmsg.empty());
}

static void TestClassMemberInheritance3() {
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

    string errmsg;
    bool ok = l.DoString("d2 = DerivedDemo2(); print(d2)", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    ok = l.DoString("print('d2.m_value = ', d2.m_value)", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    ok = l.DoString("d2.m_value = 35142; print('after update, m_value = ', d2.m_value)", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    auto lud = l.Get("d2");
    assert(static_cast<DerivedDemo1*>(lud.ToPointer())->m_value == 35142);
    assert(static_cast<DerivedDemo2*>(lud.ToPointer())->m_value == 35142);

    ok = l.DoString("d2:StaticEcho('derived instance2')", &errmsg);
    assert(ok);
    assert(errmsg.empty());
}
