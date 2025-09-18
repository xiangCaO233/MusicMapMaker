#ifndef MMM_OPERATIONMANAGER_HPP
#define MMM_OPERATIONMANAGER_HPP

#include <mmm/OperationCommands.hpp>
#include <mmm/map/editor/MMapEditEvent.hpp>
#include <stack>
#include <tool/ThreadSafeQueue.hpp>

class OperationManager {
   public:
    explicit OperationManager(ThreadSafeQueue<MMapEditEvent>& editEventQueue)
        : editEventQ(editEventQueue) {}

    void executeCommand(std::unique_ptr<OperationCommand> command) {
        if (command && command->execute(editEventQ)) {
            m_undo_stack.push(std::move(command));
            clearRedoStack();
        }
    }

    void undo() {
        if (m_undo_stack.empty()) return;
        auto command = std::move(m_undo_stack.top());
        m_undo_stack.pop();
        command->undo(editEventQ);
        m_redo_stack.push(std::move(command));
    }

    void redo() {
        if (m_redo_stack.empty()) return;
        auto command = std::move(m_redo_stack.top());
        m_redo_stack.pop();
        if (command->execute(editEventQ)) {
            m_undo_stack.push(std::move(command));
        }
    }

   private:
    // 操作事件队列
    ThreadSafeQueue<MMapEditEvent>& editEventQ;

    // 清空重做栈
    void clearRedoStack() {
        std::stack<std::unique_ptr<OperationCommand>> empty_stack;
        m_redo_stack.swap(empty_stack);
    }

    std::stack<std::unique_ptr<OperationCommand>> m_undo_stack;
    std::stack<std::unique_ptr<OperationCommand>> m_redo_stack;
};

#endif  // MMM_OPERATIONMANAGER_HPP
