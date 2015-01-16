#include <iostream>
#include "luacpp.hpp"
using namespace luacpp;
using namespace std;

static inline void test_number()
{
    LuaState l;

    l.set("var", 5);
    auto lobj = l.get("var");
    cout << "var: type -> " << lobj.typestr()
        << ", value = " << lobj.tonumber() << endl;
}

static inline void test_nil()
{
    LuaState l;

    auto lobj = l.get("nilobj");
    cout << "nilobj: type -> " << lobj.typestr() << endl;
}

static inline void test_string()
{
    LuaState l;

    string var("ouonline");
    l.set("var", var.c_str(), var.size());
    auto lobj = l.get("var");
    cout << "var: type -> " << lobj.typestr()
        << ", value = " << lobj.tostring() << endl;
}

static inline void test_table()
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
}

static inline void test_function_with_return_value()
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
    l.get("return2").tofunction().exec(2, &res, nullptr, 5, "ouonline");
    cout << "calling lua funciont from cpp:" << endl
        << "    res[0] -> " << res[0].tonumber() << endl
        << "    res[1] -> " << res[1].tostring() << endl;
}

static inline void test_function_without_return_value()
{
    LuaState l;

    typedef void (*func_t)(const char*);

    func_t echo = [] (const char* msg) -> void {
        cout << msg;
    };
    auto lfunc = l.newfunction(echo, "echo");
    lfunc.exec(0, nullptr, nullptr,
               "calling cpp function without return value from cpp\n");

    l.dostring("echo('calling cpp function without return value from lua\\n')");
}

class TestClass {

    public:

        TestClass()
        {
            cout << "TestClass::TestClass() is called without value." << endl;
        }

        TestClass(const char* msg, int x)
            : m_msg(msg)
        {
            cout << "TestClass::TestClass() is called with string -> '"
                << msg << "' and value -> " << x << "." << endl;
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

static inline void test_class_constructor()
{
    LuaState l;

    auto lclass = l.newclass<TestClass>("TestClass"); // with default constructor
    l.dostring("tc = TestClass()");

    lclass.setconstructor<const char*, int>();
    l.dostring("tc = TestClass('ouonline', 5)");
}

static inline void test_class_member_function()
{
    LuaState l;

    l.newclass<TestClass>("TestClass")
        .setconstructor()
        .set("set", &TestClass::set)
        .set("print", &TestClass::print)
        .set<void, const char*>("echo_str", &TestClass::echo) // overdofileed function
        .set<void, int>("echo_int", &TestClass::echo)
        .set("s_echo", &TestClass::s_echo); // static member function

    l.dostring("tc = TestClass();"
               "tc:set('content from lua'); tc:print();"
               "tc:echo_str('calling class member function from lua')");
}

static inline void test_class_static_member_function()
{
    LuaState l;

    auto lclass = l.newclass<TestClass>("TestClass"); // with default constructor

    lclass.set("s_echo", &TestClass::s_echo);
    l.dostring("TestClass:s_echo('static member function is called without being instantiated');"
               "tc = TestClass(); tc:s_echo('static member function is called by an instance')");
}

static inline void test_userdata_1()
{
    LuaState l;

    l.newclass<TestClass>("TestClass").set("print", &TestClass::print);
    l.newuserdata<TestClass>("tc").object<TestClass>()->set("in lua: print test data from cpp");
    l.dostring("tc:print()");
}

static inline void test_userdata_2()
{
    LuaState l;

    l.newclass<TestClass>("TestClass").set("set", &TestClass::set);
    l.dostring("tc = TestClass(); tc:set('in cpp: print test data from lua')");
    l.get("tc").touserdata().object<TestClass>()->print();
}

static inline void test_misc()
{
    LuaState l;

    string var;
    if (!l.dofile(__FILE__, &var))
        cerr << "loading " << __FILE__ << " failed: " << var << endl;
}

static struct {
    const char* name;
    void (*func)();
} test_suite[] = {
    {"number test", test_number},
    {"nilobj test", test_nil},
    {"string test", test_string},
    {"table test", test_table},
    {"function with return value test", test_function_with_return_value},
    {"function without return value test", test_function_without_return_value},
    {"class constructor test", test_class_constructor},
    {"class member function test", test_class_member_function},
    {"class static member function test", test_class_static_member_function},
    {"userdata test 1", test_userdata_1},
    {"userdata test 2", test_userdata_2},
    {"misc test", test_misc},
    {nullptr, nullptr},
};

int main(void)
{
    for (int i = 0; test_suite[i].func; ++i) {
        cout << "-------------------- "
            << test_suite[i].name
            << " --------------------" << endl;
        test_suite[i].func();
    }

    return 0;
}
