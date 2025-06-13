#ifndef COMMANDMODIFYANIMATIONNAME_HPP
#define COMMANDMODIFYANIMATIONNAME_HPP

#include "BaseCommand.hpp"

#include <string_view>

#include "manager/SessionManager.hpp"

class CommandModifyAnimationName : public BaseCommand {
public:
    // Constructor: Modify animation name from cellanimIndex, animationIndex and name.
    CommandModifyAnimationName(
        unsigned cellanimIndex, unsigned animationIndex,
        std::string_view name
    ) :
        mCellAnimIndex(cellanimIndex), mAnimationIndex(animationIndex),
        mNewName(name)
    {
        mOldName = getAnimation().name;
    }
    ~CommandModifyAnimationName() = default;

    void Execute() override {
        getAnimation().name = mNewName;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

    void Rollback() override {
        getAnimation().name = mOldName;

        SessionManager::getInstance().setCurrentSessionModified(true);
    }

private:
    unsigned mCellAnimIndex;
    unsigned mAnimationIndex;

    std::string mOldName;
    std::string mNewName;

    CellAnim::Animation& getAnimation() {
        return SessionManager::getInstance().getCurrentSession()
            ->cellanims.at(mCellAnimIndex).object
            ->getAnimation(mAnimationIndex);
    }
};

#endif // COMMANDMODIFYANIMATIONNAME_HPP
