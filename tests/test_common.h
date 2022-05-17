#ifndef __LUA_CPP_TEST_COMMON_H__
#define __LUA_CPP_TEST_COMMON_H__

#include <string>
using namespace std;

class ClassDemo {
public:
    ClassDemo() {
        cout << "ClassDemo::ClassDemo() is called without value." << endl;
    }
    ClassDemo(const char* msg, int x) {
        if (msg) {
            m_msg = msg;
        }
        cout << "ClassDemo::ClassDemo() is called with string -> '" << msg << "' and int -> " << x << "." << endl;
    }
    ClassDemo(const ClassDemo& rhs) {
        if (this != &rhs) {
            m_value = rhs.m_value;
            m_msg = rhs.m_msg;
            cout << "ClassDemo's copy constructor is called." << endl;
        }
    }
    virtual ~ClassDemo() {
        cout << "ClassDemo::~ClassDemo() is called." << endl;
    }

    ClassDemo& operator=(const ClassDemo& rhs) {
        if (this != &rhs) {
            m_value = rhs.m_value;
            m_msg = rhs.m_msg;
            cout << "ClassDemo's copy assignment is called." << endl;
        }
        return *this;
    }

    void Set(const char* msg) {
        m_msg = msg;
        cout << "ClassDemo()::Set(msg): '" << msg << "'" << endl;
    }
    void Print() const {
        cout << "ClassDemo::Print(msg): '" << m_msg << "'" << endl;
    }
    virtual const char* Echo(const char* msg) const {
        cout << "ClassDemo::Echo(string): '" << msg << "'" << endl;
        return msg;
    }
    int Echo(int v) const {
        cout << "ClassDemo::Echo(int): " << v << endl;
        return v;
    }
    static const char* StaticEcho(const char* msg) {
        cout << "ClassDemo::StaticEcho(string): '" << msg << "'" << endl;
        return msg;
    }

    static int st_value;
    int m_value = 55555;

private:
    string m_msg;
};

class DerivedDemo1 : public ClassDemo {
public:
    const char* Echo(const char* msg) const override {
        cout << "DerivedDemo1::Echo(string): '" << msg << "'" << endl;
        return msg;
    }
};

class DerivedDemo2 : public DerivedDemo1 {
public:
    const char* Echo(const char* msg) const override {
        cout << "DerivedDemo2::Echo(string): '" << msg << "'" << endl;
        return msg;
    }
};

struct Point final {
    int x = 10;
    int y = 20;
};

template <typename T>
T ReturnSelf(T v) {
    return v;
}

#endif
