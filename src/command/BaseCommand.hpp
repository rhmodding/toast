#ifndef BASE_COMMAND_HPP
#define BASE_COMMAND_HPP

class BaseCommand {
public:
    virtual void Execute() {}
    virtual void Rollback() {}

    virtual ~BaseCommand() {}
};

#endif // BASE_COMMAND_HPP