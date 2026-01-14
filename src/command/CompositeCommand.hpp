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

    void execute() override {
        for (const auto& cmd : commands) {
            cmd->execute();
        }
    }
    void rollback() override {
        for (auto it = commands.rbegin(); it != commands.rend(); ++it) {
            (*it)->rollback();
        }
    }

private:
    std::vector<std::shared_ptr<BaseCommand>> commands;
};

#endif // COMPOSITE_COMMAND_HPP
