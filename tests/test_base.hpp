#include <iostream>
#include "luacpp/luacpp.h"
#include "test_common.h"
using namespace luacpp;
using namespace std;

#undef NDEBUG
#include <assert.h>

static void TestSetGet() {
    LuaState l(luaL_newstate(), true);
    const int value = 5;

    auto lobj = l.CreateInteger(value);
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

    const string var("ouonline");
    auto lobj = l.CreateString(var.c_str(), var.size(), "var");
    assert(lobj.GetType() == LUA_TSTRING);

    auto buf = lobj.ToStringRef();
    assert(string(buf.base, buf.size) == var);

    auto lobj2 = l.Get("var");
    auto buf2 = lobj2.ToStringRef();
    assert(string(buf2.base, buf2.size) == var);
}

static void TestTable() {
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

    LuaTable table(l.Get("var"));
    table.ForEach(iterfunc);

    auto lobj = table.Get("others");
    assert(lobj.GetType() == LUA_TSTRING);
    auto buf = lobj.ToStringRef();
    assert(string(buf.base, buf.size) == "myothers");

    cout << "table2:" << endl;
    auto ltable = l.CreateTable();
    ltable.SetInteger("x", 5);
    ltable.SetString("o", "ouonline");
    ltable.Set("t", table);
    ltable.ForEach(iterfunc);

    l.DoString("arr = {'a', 'c', 'e'}");
    LuaTable(l.Get("arr")).ForEach([](uint32_t i, const LuaObject& value) -> bool {
        auto buf_ref = value.ToStringRef();
        const string s(buf_ref.base, buf_ref.size);
        cout << "[" << i << "] -> " << s << endl;
        if (i == 0) {
            assert(s == "a");
        } else if (i == 1) {
            assert(s == "c");
        } else if (i == 2) {
            assert(s == "e");
        }
        return true;
    });
}

static void TestFuncWithReturnValue() {
    LuaState l(luaL_newstate(), true);

    auto lfunc = l.CreateFunction([] (const char* msg) -> int {
        cout << "in std::function Echo(str): '" << msg << "'" << endl;
        return 5;
    }, "Echo");

    const string msg = "calling cpp function with return value from cpp.";
    l.CreateString(msg.data(), msg.size(), "msg");

    lfunc.Execute([](uint32_t, const LuaObject& lobj) -> bool {
        auto res = lobj.ToInteger();
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
    LuaFunction(l.Get("return2")).Execute([&msg2](uint32_t i, const LuaObject& lobj) -> bool {
        if (i == 0) {
            assert(lobj.ToInteger() == 5);
            cout << "get [0] -> " << lobj.ToInteger() << endl;
        } else if (i == 1) {
            auto buf = lobj.ToStringRef();
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

static void TestFuncWithoutReturnValue() {
    LuaState l(luaL_newstate(), true);

    auto lfunc = l.CreateFunction(GlobalEcho, "Echo");
    lfunc.Execute(nullptr, nullptr, "calling cpp function without return value from cpp");

    string errmsg;
    bool ok = l.DoString("Echo('calling cpp function without return value from lua')", &errmsg);
    assert(ok);
    assert(errmsg.empty());
}

static void TestFuncWithBuiltinReferenceTypes() {
    LuaState l(luaL_newstate(), true);
    auto lobj = l.CreateInteger(53142, "var");
    auto lfunc = l.CreateFunction([](const LuaObject& lobj) -> void {
        cout << "empty function for testing reference arguments." << endl;
        cout << "value = " << lobj.ToInteger() << endl;
    });

    string errmsg;
    bool ok = lfunc.Execute(nullptr, &errmsg, lobj);
    assert(ok);
    assert(errmsg.empty());
}

static int variadic_argument_func_demo(lua_State* l) {
    cout << "get argc = " << lua_gettop(l) << endl;
    return 0;
}

static void TestVariadicArguments() {
    LuaState l(luaL_newstate(), true);
    auto lfunc = l.CreateFunction(variadic_argument_func_demo, "VariadicFunc");

    string errmsg;
    bool ok = l.DoString("VariadicFunc(5)", &errmsg);
    assert(ok);
    assert(errmsg.empty());

    ok = l.DoString("VariadicFunc(123, {'a', ['b'] = 2, ['c'] = 'd'}, 'ouonline')", &errmsg);
    assert(ok);
    assert(errmsg.empty());
}

static void TestUserdata1() {
    LuaState l(luaL_newstate(), true);

    auto lclass = l.CreateClass<ClassDemo>("ClassDemo")
        .DefConstructor<const char*, int>()
        .DefMember("get_object_addr", [](ClassDemo* obj) -> ClassDemo* {
            return obj;
        })
        .DefMember("print", &ClassDemo::Print);

    auto lud = lclass.CreateUserData("ouonline", 5);
    auto ud = lud.Get<ClassDemo>();
    ud->Set("in lua: Print test data from cpp");
    l.Set("tc", lud);
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
                         [] (uint32_t n, const LuaObject& lobj) -> bool {
                             cout << "output from resiter: ";
                             if (n == 0) {
                                 cout << lobj.ToString() << endl;
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
