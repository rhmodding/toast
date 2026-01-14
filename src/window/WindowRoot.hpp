#ifndef WINDOW_ROOT_HPP
#define WINDOW_ROOT_HPP

#include "BaseWindow.hpp"

#include <list>

#include <memory>

#include <type_traits>

#include <imgui.h>

class WindowRoot : public BaseWindow {
private:
    struct SubWindowOptions {
        const char *name;
        bool showOnlyWithSession;
        bool showInAppMenu;
    };

    class ISubWindow {
    public:
        virtual ~ISubWindow() = default;
        virtual void update() = 0;
        virtual void setNext(ISubWindow *next) = 0;
        virtual ISubWindow *getNext() const = 0;
    };

    template <typename T = BaseWindow>
    class SubWindow : public ISubWindow {
    public:
        SubWindow(SubWindowOptions options) {
            static_assert(std::is_base_of<BaseWindow, T>::value, "T must be derived from BaseWindow");
            mInst = std::make_unique<T>();
            mMyOptions = std::move(options);
        }
        ~SubWindow() {
            destroy();
        }

        void update() override;

        void destroy() { mInst.reset(); }

        const SubWindowOptions &getOptions() const { return mMyOptions; }

        void openOption() {
            if (ImGui::MenuItem(mMyOptions.name, "", nullptr)) {
                mInst->setOpen(true);
            }
        }
        void toggleShyOption() {
            if (ImGui::MenuItem(mMyOptions.name, "", !mIsShy)) {
                mIsShy ^= true;
            }
        }

        SubWindow *getNext() const override { return static_cast<SubWindow *>(mNext); }
        void setNext(ISubWindow *next) override { mNext = next; }

    private:
        bool mIsShy { false };
        std::unique_ptr<T> mInst;
        SubWindowOptions mMyOptions;

        ISubWindow *mNext { nullptr };
    };

public:
    WindowRoot();
    ~WindowRoot();

    void update() override;

private:
    template <typename T>
    void registerWindow(SubWindowOptions options);

    void doMenubar();

private:
    SubWindow<> *mWindowHead { nullptr };
    SubWindow<> *mWindowTail { nullptr };
};

#endif // WINDOW_ROOT_HPP
