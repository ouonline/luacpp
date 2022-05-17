#include "test_base.hpp"
#include "test_class.hpp"
#include <vector>
using namespace std;

#define TEST_CASE(func) {#func, func}

static const vector<pair<string, void (*)()>> g_test_suite = {
    // ----- test base ----- //

    TEST_CASE(TestLuaStateSetGet),
    TEST_CASE(TestLuaStatePush),
    TEST_CASE(TestNil),
    TEST_CASE(TestString),
    TEST_CASE(TestTable),
    TEST_CASE(TestTableGetSet),
    TEST_CASE(TestFuncWithReturnValue),
    TEST_CASE(TestFuncWithoutReturnValue),
    TEST_CASE(TestFuncWithBuiltinReferenceTypes),
    TEST_CASE(TestVariadicArguments),
    TEST_CASE(TestUserdata1),
    TEST_CASE(TestUserdata2),
    TEST_CASE(TestDoString),
    TEST_CASE(TestDoFile),

    // ----- test class ----- //

    TEST_CASE(TestClass),
    TEST_CASE(TestClassConstructor),
    TEST_CASE(TestClassProperty),
    TEST_CASE(TestClassPropertyReadWrite),
    TEST_CASE(TestClassMemberFunction),
    TEST_CASE(TestClassLuaMemberFunction),
    TEST_CASE(TestClassStaticProperty),
    TEST_CASE(TestClassStaticPropertyReadWrite),
    TEST_CASE(TestClassStaticMemberFunction),
    TEST_CASE(TestClassLuaStaticMemberFunction),
    TEST_CASE(TestClassStaticMemberInheritance),
    TEST_CASE(TestClassMemberInheritance),
    TEST_CASE(TestClassMemberInheritance3),
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
