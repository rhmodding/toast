#ifndef POPUP_HPP
#define POPUP_HPP

class Popup {
public:
    virtual void Update() = 0;
protected:
    Popup() = default;
};

#endif