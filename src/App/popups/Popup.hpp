#ifndef POPUP_HPP
#define POPUP_HPP

class Popup {
    public:
        Popup();
        virtual ~Popup() = default;
        virtual void Update() = 0;
};

#endif