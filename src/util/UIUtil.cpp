#include "UIUtil.hpp"

#include <imgui_internal.h>

#include <stdexcept>

bool UIUtil::Widget::StdStringTextInput(const char* label, std::string& str) {
    constexpr ImGuiTextFlags flags = ImGuiInputTextFlags_CallbackResize;

    return ImGui::InputText(label, str.data(), str.capacity() + 1, flags,
        [](ImGuiInputTextCallbackData* data) -> int {
            std::string* str = reinterpret_cast<std::string*>(data->UserData);
            if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
                if (data->Buf != str->data()) {
                    throw std::runtime_error("UIUtil::Widget::StdStringTextInput: data->Buf is not equal to str->data() ..");
                }

                if (str->size() != data->BufTextLen) {
                    str->resize(data->BufTextLen);
                    data->Buf = str->data();
                }
            }

            return 0;
        }, &str
    );
}
