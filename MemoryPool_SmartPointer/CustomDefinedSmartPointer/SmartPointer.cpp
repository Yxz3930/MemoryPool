#include <iostream>
#include "MemoryPool_CAS.h"
#include <string>
#include <memory> // 智能指针
using namespace memory_pool_CAS;

template <class T, class... Args>
std::shared_ptr<T> make_shared_ptr_from_pool(Args &&...args)
{
    // 分配内存
    void *addr = HashBucket::allocate(sizeof(T));

    // 构造对象 placement new
    T *ptr = new (addr) T(std::forward<Args>(args)...);

    // 自定义删除器 结合内存池使用
    auto deleter = [](T *p)
    {
        p->~T();
        HashBucket::deallocate(p, sizeof(T));
    };

    // 智能指针包装 返回
    return std::shared_ptr<T>(ptr, deleter);
}

template <class T, class... Args>
std::unique_ptr<T> make_unique_ptr_from_pool(Args &&...args)
{
    // 分配内存
    void *addr = HashBucket::allocate(sizeof(T));

    // 构造对象 placement new
    T *ptr = new (addr) T(std::forward<Args>(args)...);

    // 自定义删除器 结合内存池使用
    auto deleter = [](T *p)
    {
        p->~T();
        HashBucket::deallocate(p, sizeof(T));
    };

    // 智能指针包装 返回
    return std::unique_ptr<T>(ptr, deleter);
}

struct Student
{
    std::string name;
    double score;
    Student() : name(""), score(0.0) {}
    Student(std::string _name, double _score) : name(_name), score(_score) {}
    ~Student()
    {
        name = "";
        score = 0.0;
    }
};

int main()
{
    HashBucket::initMemoryPool();

    std::shared_ptr<Student> ptr1 = make_shared_ptr_from_pool<Student>();
    std::cout << "name: " << ptr1->name << ", score: " << ptr1->score << std::endl;

    std::shared_ptr<Student> ptr2 = make_shared_ptr_from_pool<Student>("abc", 999);
    std::cout << "name: " << ptr2->name << ", score: " << ptr2->score << std::endl;

    std::cout << ptr1.use_count() << std::endl;
    std::cout << ptr2.use_count() << std::endl;

    ptr1.reset(new Student("hhh", 1));
    std::cout << "name: " << ptr1->name << ", score: " << ptr1->score << std::endl;

    ptr1 = ptr2;
    std::cout << ptr1.use_count() << std::endl;
    std::cout << ptr2.use_count() << std::endl;

    return 0;
}