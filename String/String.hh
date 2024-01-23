#include <iostream>
#include <cstring>
using namespace std;

class CMyString {
public:
    // 默认构造函数
    CMyString():m_pData(nullptr){}

    // 带参构造函数
    CMyString(const char* src) {
        if(src != nullptr) {
            // 申请空间
            this->m_pData = new char[strlen(src) + 1];
            // 拷贝一份过来
            memcpy(this->m_pData, src, strlen(src) + 1);   
        }else {
            m_pData = nullptr;
        }
    }

    // 拷贝构造函数
    CMyString(const CMyString& other) {
        if(other.m_pData != nullptr) {
            this->m_pData = new char[strlen(other.m_pData) + 1];
            memcpy(this->m_pData, other.m_pData, strlen(other.m_pData) + 1);
        }else {
            this->m_pData = nullptr;
        }
    }

  // 一般实现
    // CMyString& operator=(const CMyString& other) {
    //     if(this != &other) {
    //         delete []m_pData;
    //         m_pData = nullptr;

    //         m_pData = new char[strlen(other.m_pData) + 1];
    //         strcpy(m_pData, other.m_pData);
    //     }

    //     return *this;
    // }

    // 考虑异常安全性的实现
    // 可能出现的异常是：内存空间不足，无法重新为 m_pData new 一块空间，返回 bad_alloc 错误
    // 如何体现考虑了异常安全性？
    // 我们使用一个临时的实例来尝试进行 new 空间，如果空间不足，则报错并返回，但此时并没有修改原来的实例，
    // 因此实例的状态还是有效的，通过这样保证了异常安全性
    CMyString& operator=(const CMyString& other) {
        if(this != &other) {
            CMyString tempStr(other);

            char* pTemp = tempStr.m_pData;
            tempStr.m_pData = m_pData;
            m_pData = pTemp;
        }
        return *this;
    }

    // 获取 C 风格字符串
    const char* c_str() const {
        return this->m_pData;
    }

    //析构函数
    ~CMyString() {
        delete[] m_pData;
    }

private:
    char* m_pData;    // 重载赋值运算符

};