#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <iostream>

#include <memory>
#include <mutex>

#include <stdexcept>

#include "Logging.hpp"

#include "CxxDemangle.hpp"

#include "common.hpp"

template<typename T>
class Singleton {
public:
    static void createSingleton();
    static void destroySingleton();

    static T& getInstance();

    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;

    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;

protected:
    Singleton() = default;
    virtual ~Singleton() = default;

private:
    static std::unique_ptr<T> instance_;
    static std::mutex mutex_;
};

template<typename T>
std::unique_ptr<T> Singleton<T>::instance_ = nullptr;

template<typename T>
std::mutex Singleton<T>::mutex_;

template<typename T>
void Singleton<T>::createSingleton() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_) {
        throw std::runtime_error(
            "Singleton<" + CxxDemangle::Demangle(typeid(T).name()) + ">::createSingleton: Singleton instance already exists!"
        );
    }
    instance_ = std::unique_ptr<T>(new T());
}

template<typename T>
void Singleton<T>::destroySingleton() {
    std::lock_guard<std::mutex> lock(mutex_);
    instance_.reset();
}

template<typename T>
T& Singleton<T>::getInstance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
        throw std::runtime_error(
            "Singleton<" + CxxDemangle::Demangle(typeid(T).name()) + ">::getInstance: Singleton instance does not exist (anymore)!"
        );
    }
    return *instance_;
}

#endif // SINGLETON_HPP
