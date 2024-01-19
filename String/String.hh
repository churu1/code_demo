#include <iostream>
#include <cstring>
using namespace std;

class String {
public:
    // 默认构造函数
    String():Str(nullptr){}

    // 带参构造函数
    String(const char* src) {
        if(src != nullptr) {
            // 申请空间
            this->Str = new char[strlen(src) + 1];
            // 拷贝一份过来
            memcpy(this->Str, src, strlen(src) + 1);   
        }else {
            Str = nullptr;
        }
    }

    // 拷贝构造函数
    String(const String& other) {
        if(other.Str != nullptr) {
            this->Str = new char[strlen(other.Str) + 1];
            memcpy(this->Str, other.Str, strlen(other.Str) + 1);
        }else {
            this->Str = nullptr;
        }
    }

    // 重载赋值运算符
    String& operator=(const String& rhs) {
        if(this != nullptr) {
            delete[] Str;

            if(rhs.Str != nullptr) {
                 // 深拷贝
                this->Str = new char[strlen(rhs.Str) + 1];
                memcpy(this->Str, rhs.Str, strlen(rhs.Str) + 1);
            }else{
                this->Str = nullptr;
            }
        }
        return *this;
    }

    // 获取 C 风格字符串
    const char* c_str() const {
        return this->Str;
    }

    //析构函数
    ~String() {
        delete[] Str;
    }


private:
    char* Str;
};