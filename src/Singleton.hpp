#ifndef SINGLETON_HPP
#define SINGLETON_HPP

template<typename T>
class Singleton {
public:
    static T& getInstance() {
        static T instance; // Static instance created once
        return instance;
    }

    Singleton(const Singleton&) = delete; // Delete copy constructor
    Singleton& operator=(const Singleton&) = delete; // Delete assignment operator

protected:
    Singleton() {} // Protected constructor to prevent instantiation
};

#endif // SINGLETON_HPP
