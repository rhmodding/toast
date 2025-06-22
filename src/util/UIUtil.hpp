#ifndef UI_UTIL_HPP
#define UI_UTIL_HPP

#include <string>

#include <functional>

#include <imgui.h>

namespace UIUtil {

namespace Widget {

template <typename T>
void ValueEditor(
    const char* label, T& currentValue,
    std::function<T()> getOriginal,
    std::function<void(const T& oldValue, const T& newValue)> setFinal,
    std::function<bool(const char* label, T* value)> widgetCall
) {
    static T oldValue;
    T tempValue = currentValue;

    if (widgetCall(label, &tempValue))
        currentValue = tempValue;

    if (ImGui::IsItemActivated())
        oldValue = getOriginal();
    if (ImGui::IsItemDeactivated())
        setFinal(oldValue, tempValue);
}

bool StdStringTextInput(const char* label, std::string& str);

} // namespace Widget

} // namespace UIUtil

#endif // UI_UTIL_HPP
