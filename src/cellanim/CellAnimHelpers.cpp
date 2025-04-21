#include "CellAnimHelpers.hpp"

#include <memory>

#include "../SessionManager.hpp"

#include "../command/CommandInsertArrangement.hpp"

unsigned CellAnimHelpers::duplicateArrangement(unsigned arrangementIndex) {
    SessionManager& sessionManager = SessionManager::getInstance();

    std::shared_ptr object = sessionManager.getCurrentSession()->getCurrentCellanim().object;

    sessionManager.getCurrentSession()->addCommand(
    std::make_shared<CommandInsertArrangement>(
        sessionManager.getCurrentSession()->getCurrentCellanimIndex(),
        object->arrangements.size(),
        object->arrangements.at(arrangementIndex)
    ));

    return object->arrangements.size() - 1;
}

bool CellAnimHelpers::getArrangementUnique(unsigned arrangementIndex) {
    unsigned timesUsed { 0 };

    const auto& animations =
        SessionManager::getInstance().getCurrentSession()
            ->getCurrentCellanim().object->animations;

    for (const auto& animation : animations) {
        for (const auto& key : animation.keys) {
            if (key.arrangementIndex == arrangementIndex)
                timesUsed++;

            if (timesUsed > 1)
                return false;
        }
    }

    return true;
}
