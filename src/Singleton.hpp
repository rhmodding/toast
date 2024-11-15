#ifndef SINGLETON_HPP
#define SINGLETON_HPP

template<typename T>
class Singleton {
public:
    static T& getInstance() {
        // Static instance created once
        static T instance;

        return instance;
    }

    Singleton(const Singleton&) = delete;            // Delete copy constructor
    Singleton(Singleton&&) = delete;                 // Delete move constructor
    Singleton& operator=(const Singleton&) = delete; // Delete copy assignment
    Singleton& operator=(Singleton&&) = delete;      // Delete move assignment

protected:
    Singleton() = default;           // Protected default constructor
    virtual ~Singleton() = default;  // Protected virtual destructor
};

#endif // SINGLETON_HPP
