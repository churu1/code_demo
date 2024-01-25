#include <iostream>
using namespace std;

template<class T>
class CCounter { //捆绑关系
private:
    T* m_ptr; // 指向堆区的指针
    int m_count; // 引用计数 此处是捆绑在一个类中

    CCounter(T* p): m_ptr(p), m_count(1) { // 指向空间 计数为1
    }

    ~CCounter() { // 回收空间 计数为0
        m_count = 0;
        delete m_ptr;
        m_ptr = nullptr;
    }

    template<class Y> // 为了让智能指针可以使用该类的私有成员，使用友元类来实现
    friend class CMyShared_ptr;
};

template<class T>
class CMyShared_ptr { // 捆绑关系 -- 空间和引用计数共用
public:
    CMyShared_ptr(T *p); // 构造 创建 Counter 类的空间，然后让里面指针指向外面申请的堆区
    CMyShared_ptr(const CMyShared_ptr& other); // 拷贝构造 Counter 类的空间已经存在 引用计数要 +1
    CMyShared_ptr& operator= (const CMyShared_ptr& other); // 重载赋值操作符 左值的引用计数要-1并判断是否为0，考虑是否回收 右值引用计数 +1
    ~CMyShared_ptr(); // 引用计数-1，判断是否为0，考虑回收

    // 返回 Counter类里的 m_count;
    int use_count() const {
        return mPtrCount->m_count;
    }
    // 返回 Counter类里的 m_ptr;
    T* get() const {
        return mPtrCount->m_ptr;
    }

private:
    CCounter<T> *mPtrCount; // 空间和引用计数 捆绑关系的指针
};

/*
shared_ptr 是线程安全的，指的是引用计数是线程安全的，但是如果两个智能指针指向同一个对空间，在不同线程使用堆空间是线程不安全的
但是引用计数 +1 -1 操作是线程安全的
除了使用锁之外，还可以使用操作系统的原子访问api 操作：
原子增：__sync_fetch_and_add(&variable, val);
原子减：__sync_fetch_and_sub(&variable, val);

在编译时使用 -march和-march编译选项启用对应的体系结构
g++ -march=native -std=c++1 xxx.cpp -o xxx
*/

template<class T>
CMyShared_ptr<T>::CMyShared_ptr(T* p) {
    mPtrCount = new CCounter<T>(p);
}

template<class T>
CMyShared_ptr<T>::CMyShared_ptr(const CMyShared_ptr& other) { // 浅拷贝
    this->mPtrCount = other.mPtrCount;
    //this->mPtrCount->m_count++;
    // 使用 linux 下的原子增操作
    __sync_fetch_and_add(& this->mPtrCount->m_count, 1);
}

template<class T>
CMyShared_ptr<T>& CMyShared_ptr<T>:: operator=(const CMyShared_ptr& other) {
    if(this != &other) {
        // 处理之前的
        //this->mPtrCount->m_count--;
        // 原子减操作
        __sync_fetch_and_sub(& this->mPtrCount->m_count, 1);
        if(this->mPtrCount->m_count == 0) {
            // 一旦引用计数归0，此时除了当前这个指针指向这个空间 没有其他指向这个空间
            delete this->mPtrCount; // 调用 Counter 的析构 将整个 Counter实例回收 
        }
        // 处理现在的
        this->mPtrCount = other.mPtrCount;
        //this->mPtrCount->m_count++;
        __sync_fetch_and_add(& this->mPtrCount->m_count, 1);
    }
    return *this;
}

template<class T>
CMyShared_ptr<T>::~CMyShared_ptr() {
    //this->mPtrCount->m_count--;
    __sync_fetch_and_sub(& this->mPtrCount->m_count, 1);
    if(this->mPtrCount->m_count == 0) {
        // 一旦引用计数归0，此时除了当前这个指针指向这个空间 没有其他指向这个空间
        delete this->mPtrCount; // 调用 Counter 的析构 将整个 Counter实例回收 
    }
}

// 模板类 迟后编译 注意与动态联编区分
// 指明类型后模板类才是完整的类型，才能完成编译

class CTest{
public:

    CTest(string s) {
        m_str = s;
        cout << "CTest::CTest(string s)" << " " << m_str << endl;
    }

    CTest() {
        cout << "CTest::CTest()" << endl;
    }
    
    CTest(CTest& other) {
        m_str = other.m_str;
        cout << "CTest::CTest(CTest& other)" <<" " << m_str <<endl;
    }

    ~CTest() {
        cout << "CTest::~CTest()" << endl;
    }

    void test() {
        cout << "CTest::" << __func__ << endl;
    }
private:
    string m_str;
};

int main() {
    {
        CMyShared_ptr<CTest> sp1(new CTest("sp1"));   
        cout << sp1.use_count() << endl;
        {
            CMyShared_ptr<CTest> sp2 = sp1; // 拷贝构造
            cout << sp1.use_count() << endl;
            cout << sp2.use_count() << endl;
        }
        cout << sp1.use_count() << endl;
        sp1.get()->test();
    }

    return 0;
}