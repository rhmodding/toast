#ifndef BASECOMMAND_HPP
#define BASECOMMAND_HPP

class BaseCommand {
public:
    virtual void Execute() {};

    virtual void Rollback() {};

    virtual ~BaseCommand() {}
};

#endif // BASECOMMAND_HPP