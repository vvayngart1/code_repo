#pragma once

namespace tw {
namespace common {
    
template<typename T>
class Singleton {
public:
    static T& instance() {
        static T _instance;
        return _instance;
    }

protected:
    Singleton() {
    }

    virtual ~Singleton() {
    }
    
private:
    struct object_creator {
      // This constructor does nothing more than ensure that instance()
      //  is called before main() begins, thus creating the static
      //  T object before multithreading race issues can come up.
      //
      object_creator() { Singleton<T>::instance(); }
      inline void do_nothing() const { }
    };
    static object_creator create_object;
    
private:
    Singleton(const Singleton&);
    Singleton& operator=(const Singleton&);
};

} // namespace common
} // namespace tw




