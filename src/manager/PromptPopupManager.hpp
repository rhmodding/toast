#ifndef PROMPT_POPUP_MANAGER_HPP
#define PROMPT_POPUP_MANAGER_HPP

#include "Singleton.hpp"

#include <cstdint>

#include <string>

#include <functional>

#include <vector>

#include "Logging.hpp"

// TODO: move out
class PromptPopup {
public:
    enum Response {
        RESPONSE_NULL = 0,
        RESPONSE_OK = (1 << 0),
        RESPONSE_YES = (1 << 1),
        RESPONSE_CANCEL = (1 << 2),
        RESPONSE_NO = (1 << 3),
        RESPONSE_RETRY = (1 << 4),
        RESPONSE_ABORT = (1 << 5),
    };
    typedef uint32_t ResponseFlags;

public:
    uint32_t id;

    ResponseFlags availableResponseFlags { RESPONSE_OK };
    Response defaultResponse { RESPONSE_OK };

    std::string title;
    std::string message;

    bool hasInputField { false };
    std::string currentInputText;

    // HEY ! IMPLEMENT ME! TODO
    bool hasComboBox { false };
    std::vector<std::string> comboBoxOptions;
    int selectedComboIndex { 0 };

    // - inputText will be null if not applicable
    // - comboText will be null if not applicable
    std::function<void(Response res, const std::string* inputText)> callback;

    bool hasEnded { false };

public:
    void Show();

private:
    void DoCallback(Response selectedRes) {
        if (callback) {
            callback(selectedRes, hasInputField ? &currentInputText : nullptr);
        }
    }

public: // Building methods
    PromptPopup& WithResponses(ResponseFlags flags, Response defaultSelect = RESPONSE_NULL) {
        availableResponseFlags = flags;
        defaultResponse = defaultSelect;
        return *this;
    }

    PromptPopup& WithInput(std::string defaultInput = {}) {
        hasInputField = true;
        currentInputText = std::move(defaultInput);
        return *this;
    }

    PromptPopup& WithCallback(std::function<void(Response, const std::string* input)> cb) {
        callback = std::move(cb);
        return *this;
    }
};

class PromptPopupManager : public Singleton<PromptPopupManager> {
    friend class Singleton<PromptPopupManager>;

private:
    PromptPopupManager() = default;
public:
    ~PromptPopupManager() = default;

public:
    [[nodiscard]] static PromptPopup CreatePrompt(std::string title, std::string message) {
        PromptPopup popup;
        popup.id = sNextId++;
        popup.title = std::move(title);
        popup.message = std::move(message);
        return popup;
    }

    void Queue(PromptPopup popup) {
        mQueuedPopups.push_back(std::move(popup));
    }
    
    void Update();

private:
    void FlushQueue() {
        if (!mQueuedPopups.empty()) {
            mActivePopups.insert(
                mActivePopups.end(),
                std::make_move_iterator(mQueuedPopups.begin()),
                std::make_move_iterator(mQueuedPopups.end())
            );
            mQueuedPopups.clear();
        }
    }

private:
    static uint32_t sNextId;

    std::vector<PromptPopup> mActivePopups;
    std::vector<PromptPopup> mQueuedPopups;
};

#endif // PROMPT_POPUP_MANAGER_HPP
