#ifndef COMPOSITE_COMMAND_HPP
#define COMPOSITE_COMMAND_HPP

#include "BaseCommand.hpp"

#include <vector>

#include <memory>

class CompositeCommand : public BaseCommand {
public:
    void addCommand(std::shared_ptr<BaseCommand> command) {
        commands.push_back(command);
    }

    void Execute() override {
        for (const auto& cmd : commands) {
            cmd->Execute();
        }
    }
    void Rollback() override {
        for (auto it = commands.rbegin(); it != commands.rend(); ++it) {
            (*it)->Rollback();
        }
    }

private:
    std::vector<std::shared_ptr<BaseCommand>> commands;
};

#endif // COMPOSITE_COMMAND_HPP
