#include "test_base.hpp"
#include "test_class.hpp"
#include <map>
using namespace std;

static const map<string, void (*)()> g_test_suite = {
    // ----- test base ----- //

    {"TestSetGet", TestSetGet},
    {"TestNil", TestNil},
    {"TestString", TestString},
    {"TestTable", TestTable},
    {"TestFuncWithReturnValue", TestFuncWithReturnValue},
    {"TestFuncWithoutReturnValue", TestFuncWithoutReturnValue},
    {"TestFuncWithBuiltinReferenceTypes", TestFuncWithBuiltinReferenceTypes},
    {"TestUserdata1", TestUserdata1},
    {"TestUserdata2", TestUserdata2},
    {"TestDoString", TestDoString},
    {"TestDoFile", TestDoFile},

    // ----- test class ----- //

    {"TestClass", TestClass},
    {"TestClassConstructor", TestClassConstructor},
    {"TestClassProperty", TestClassProperty},
    {"TestClassPropertyReadWrite", TestClassPropertyReadWrite},
    {"TestClassMemberFunction", TestClassMemberFunction},
    {"TestClassLuaMemberFunction", TestClassLuaMemberFunction},
    {"TestClassStaticProperty", TestClassStaticProperty},
    {"TestClassStaticPropertyReadWrite", TestClassStaticPropertyReadWrite},
    {"TestClassStaticMemberFunction", TestClassStaticMemberFunction},
    {"TestClassLuaStaticMemberFunction", TestClassLuaStaticMemberFunction},
    {"TestClassStaticMemberInheritance", TestClassStaticMemberInheritance},
    {"TestClassMemberInheritance", TestClassMemberInheritance},
    {"TestClassMemberInheritance3", TestClassMemberInheritance3},
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
