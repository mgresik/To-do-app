#pragma once

#include "task.hpp"
#include <vector>
#include <string>
#include <memory>
#include <mutex>

class TaskManager {
public:
    explicit TaskManager(const std::string& data_file);
    ~TaskManager();

    // Copy denied (descriptor)
    TaskManager(const TaskManager&) = delete;
    TaskManager& operator=(const TaskManager&) = delete;

    bool load();
    bool save() const;

    int add_task(const std::string& description);
    bool delete_task(int id);
    bool mark_done(int id);
    const std::vector<Task>& tasks() const { return tasks_; }
    Task* find_task(int id);

    const std::string& data_file() const { return data_file_; }

    // File block (UNIX API)
    bool lock_file();
    void unlock_file();

private:
    std::string data_file_;
    int fd_;                    // decriptor
    std::vector<Task> tasks_;
    mutable std::mutex mutex_;

    int next_id_ = 1;
};