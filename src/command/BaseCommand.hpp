#ifndef BASE_COMMAND_HPP
#define BASE_COMMAND_HPP

class BaseCommand {
public:
    virtual ~BaseCommand() {}

    virtual void execute() {}
    virtual void rollback() {}
};

#endif // BASE_COMMAND_HPP