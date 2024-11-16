#include "CellanimHelpers.hpp"

#include "../SessionManager.hpp"

#include "../command/CommandInsertArrangement.hpp"

unsigned CellanimHelpers::DuplicateArrangement(unsigned arrangementIndex) {
    GET_SESSION_MANAGER;

    auto object = sessionManager.getCurrentSession()->getCellanimObject();

    sessionManager.getCurrentSession()->executeCommand(
    std::make_shared<CommandInsertArrangement>(
        sessionManager.getCurrentSession()->currentCellanim,
        object->arrangements.size(),
        object->arrangements.at(arrangementIndex)
    ));

    return object->arrangements.size() - 1;
}

bool CellanimHelpers::getArrangementUnique(unsigned arrangementIndex) {
    unsigned timesUsed { 0 };

    for (const auto& animation : SessionManager::getInstance().getCurrentSession()->getCellanimObject()->animations) {
        for (const auto& key : animation.keys) {
            if (key.arrangementIndex == arrangementIndex)
                timesUsed++;

            if (timesUsed > 1) {
                return false;
            }
        }
    }

    return true;
}