#include "String.hh"

int main() {
    CMyString s1("Hello"); // 调用带参构造
    CMyString s2 = s1; // 调用拷贝构造
    CMyString s3;

    s3 = s2; // 调用重载赋值运算符
    cout << "s1: " << s1.c_str() << endl;
    cout << "s2: " << s2.c_str() << endl;
    cout << "s3: " << s3.c_str() << endl;

    return 0;
}