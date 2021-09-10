#include <iostream>
#include "../luacpp.h"
#include "test_common.h"
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

    l.DoString("arr = {'a', 'c', 'e'}");
    LuaTable(l.Get("arr")).ForEach([](int i, const LuaObject& value) -> bool {
        auto buf_ref = value.ToBufferRef();
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

static void TestFuncWithoutReturnValue() {
    LuaState l(luaL_newstate(), true);

    auto lfunc = l.CreateFunction(GlobalEcho, "Echo");
    lfunc.Exec(nullptr, nullptr, "calling cpp function without return value from cpp");

    string errmsg;
    bool ok = l.DoString("Echo('calling cpp function without return value from lua')", &errmsg);
    assert(ok);
    assert(errmsg.empty());
}

static void TestFuncWithBuiltinReferenceTypes() {
    LuaState l(luaL_newstate(), true);
    auto lobj = l.CreateObject(53142, "var");
    auto lfunc = l.CreateFunction([](const LuaObject& lobj) -> void {
        cout << "empty function for testing reference arguments." << endl;
        cout << "value = " << lobj.ToInteger<int>() << endl;
    });

    string errmsg;
    bool ok = lfunc.Exec(nullptr, &errmsg, lobj);
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
