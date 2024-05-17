#include "AppState.hpp"

#include "ConfigManager.hpp"

void AppState::UpdateTheme() {
    this->darkTheme = ConfigManager::getInstance().config.darkTheme;

    if (this->darkTheme) {
        ImGui::StyleColorsDark();
        this->windowClearColor = Common::RGBAtoImVec4(24, 24, 24, 255);
    }
    else {
        ImGui::StyleColorsLight();
        this->windowClearColor = Common::RGBAtoImVec4(248, 248, 248, 255);
    }
}

void AppState::UpdateUpdateRate() {
    this->updateRate = 1000. / ConfigManager::getInstance().config.updateRate;
}

void AppState::PlayerState::updateSetFrameCount() {
    GET_ANIMATABLE;

    this->frameCount = static_cast<uint16_t>(
        globalAnimatable->getCurrentAnimation()->keys.size()
    );
}

void AppState::PlayerState::updateCurrentFrame() {
    GET_ANIMATABLE;
    GET_APP_STATE;

    if (this->currentFrame >= this->frameCount)
        this->currentFrame = this->frameCount - 1;

    globalAnimatable->setAnimationKeyFromIndex(this->currentFrame);

    RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

    if (appState.selectedPart >= arrangementPtr->parts.size())
        appState.selectedPart = static_cast<int16_t>(arrangementPtr->parts.size() - 1);

    this->holdFramesLeft = 0;
}

void AppState::PlayerState::ResetTimer() {
    this->timeLeft = 1 / (float)frameRate;
    this->previous = std::chrono::steady_clock::now();
}

void AppState::PlayerState::ToggleAnimating(bool animating) {
    GET_ANIMATABLE;

    this->playing = animating;
    globalAnimatable->setAnimating(animating);
}

uint16_t AppState::PlayerState::getTotalPseudoFrames() {
    uint16_t result{ 0 };

    GET_ANIMATABLE;

    for (auto key : globalAnimatable->getCurrentAnimation()->keys)
        result += key.holdFrames;

    return result;
}

uint16_t AppState::PlayerState::getCurrentPseudoFrames() {
    if (!this->playing && this->currentFrame == 0 && this->holdFramesLeft == 0)
        return 0;

    uint16_t result{ 0 };

    GET_ANIMATABLE;

    for (uint16_t keyIndex = 0; keyIndex < currentFrame; keyIndex++)
        result += globalAnimatable->getCurrentAnimation()->keys.at(keyIndex).holdFrames;

    result += globalAnimatable->getCurrentKey()->holdFrames - this->holdFramesLeft;

    return result;
}

float AppState::PlayerState::getAnimationProgression() {
    return this->getCurrentPseudoFrames() / static_cast<float>(this->getTotalPseudoFrames());
}

void AppState::PlayerState::Update() {
    GET_ANIMATABLE;

    if (this->playing && globalAnimatable) {
        auto now = std::chrono::steady_clock::now();
        auto duration = now - this->previous;
        float delta = std::chrono::duration_cast<std::chrono::duration<float>>(duration).count();
        this->previous = now;

        while (delta >= this->timeLeft) {
            globalAnimatable->Update();
            delta -= this->timeLeft;

            this->timeLeft = 1 / (float)frameRate;

            if (!globalAnimatable->getAnimating() && this->loopEnabled) {
                globalAnimatable->setAnimationFromIndex(globalAnimatable->getCurrentAnimationIndex());
                globalAnimatable->setAnimating(true);
            }

            this->playing = globalAnimatable->getAnimating();
            this->currentFrame = globalAnimatable->getCurrentKeyIndex();
            this->holdFramesLeft = globalAnimatable->getHoldFramesLeft();

            GET_APP_STATE;

            RvlCellAnim::Arrangement* arrangementPtr = globalAnimatable->getCurrentArrangement();

            if (appState.selectedPart >= arrangementPtr->parts.size())
                appState.selectedPart = static_cast<int16_t>(arrangementPtr->parts.size() - 1);
        }

        this->timeLeft -= delta;
    }
}