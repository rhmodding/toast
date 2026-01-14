#ifndef BASE_WINDOW_HPP
#define BASE_WINDOW_HPP

class BaseWindow {
public:
    virtual void update() = 0;

    virtual void setOpen(bool open) {}

    virtual ~BaseWindow() {}
};

#endif // BASE_WINDOW_HPP
