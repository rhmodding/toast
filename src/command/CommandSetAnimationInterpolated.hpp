#ifndef COMMANDSETANIMATIONINTERPOLATED_HPP
#define COMMANDSETANIMATIONINTERPOLATED_HPP

#include "BaseCommand.hpp"

#include "../SessionManager.hpp"

class CommandSetAnimationInterpolated : public BaseCommand {
public:
    // Constructor: Set animation interpolation from cellanimIndex, animationIndex and isInterpolated.
    CommandSetAnimationInterpolated(
        unsigned cellanimIndex, unsigned animationIndex,
        bool isInterpolated
    ) :
        cellanimIndex(cellanimIndex), animationIndex(animationIndex),
        newIsInterpolated(isInterpolated)
    {
        this->oldIsInterpolated = this->getAnimation().isInterpolated;
    }
    ~CommandSetAnimationInterpolated() = default;

    void Execute() override {
        this->getAnimation().isInterpolated = this->newIsInterpolated;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        this->getAnimation().isInterpolated = this->oldIsInterpolated;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned cellanimIndex;
    unsigned animationIndex;

    bool oldIsInterpolated, newIsInterpolated;

    CellAnim::Animation& getAnimation() {
        return SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(cellanimIndex).object
            ->animations.at(animationIndex);
    }
};

#endif // COMMANDSETANIMATIONINTERPOLATED_HPP
