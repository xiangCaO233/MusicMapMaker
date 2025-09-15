#ifndef MMM_OPERATIONMANAGER_HPP
#define MMM_OPERATIONMANAGER_HPP

#include <mmm/OperationCommands.hpp>
#include <stack>

class OperationManager {
   public:
    void executeCommand(std::unique_ptr<OperationCommand> command) {
        if (command && command->execute()) {
            m_undo_stack.push(std::move(command));
            clearRedoStack();
        }
    }

    void undo() {
        if (m_undo_stack.empty()) return;
        auto command = std::move(m_undo_stack.top());
        m_undo_stack.pop();
        command->undo();
        m_redo_stack.push(std::move(command));
    }

    void redo() {
        if (m_redo_stack.empty()) return;
        auto command = std::move(m_redo_stack.top());
        m_redo_stack.pop();
        if (command->execute()) {
            m_undo_stack.push(std::move(command));
        }
    }

   private:
    void clearRedoStack() {
        std::stack<std::unique_ptr<OperationCommand>> empty_stack;
        m_redo_stack.swap(empty_stack);
    }

    std::stack<std::unique_ptr<OperationCommand>> m_undo_stack;
    std::stack<std::unique_ptr<OperationCommand>> m_redo_stack;
};

#endif  // MMM_OPERATIONMANAGER_HPP
