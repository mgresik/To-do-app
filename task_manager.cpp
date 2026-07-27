#include "task_manager.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <cstring>
#include <cerrno>

TaskManager::TaskManager(const std::string& data_file)
    : data_file_(data_file), fd_(-1) 
{
    fd_ = open(data_file_.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd_ == -1) {
        std::cerr << "Не удалось открыть файл данных: " << strerror(errno) << std::endl;
    }
}

TaskManager::~TaskManager() {
    if (fd_ != -1) {
        close(fd_);
    }
}

bool TaskManager::lock_file() {
    if (fd_ == -1) return false;
    // flock - блокировка всего файла
    if (flock(fd_, LOCK_EX | LOCK_NB) == -1) {
        std::cerr << "Не удалось захватить блокировку файла: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

void TaskManager::unlock_file() {
    if (fd_ != -1) {
        flock(fd_, LOCK_UN);
    }
}

bool TaskManager::load() {
    if (fd_ == -1) return false;

    // Сбрасываем позицию в начало
    lseek(fd_, 0, SEEK_SET);

    // Читаем весь файл через read (UNIX API)
    char buffer[4096];
    std::string content;
    ssize_t bytes;
    while ((bytes = read(fd_, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes] = '\0';
        content += buffer;
    }
    if (bytes == -1) {
        std::cerr << "Ошибка чтения файла: " << strerror(errno) << std::endl;
        return false;
    }

    tasks_.clear();
    if (content.empty()) {
        next_id_ = 1;
        return true;
    }

    // Парсим построчно
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        try {
            Task task = Task::deserialize(line);
            tasks_.push_back(task);
            if (task.id() >= next_id_) next_id_ = task.id() + 1;
        } catch (const std::exception& e) {
            std::cerr << "Ошибка парсинга строки: " << line << " (" << e.what() << ")" << std::endl;
        }
    }
    return true;
}

bool TaskManager::save() const {
    if (fd_ == -1) return false;

    // Строим содержимое
    std::string content;
    for (const auto& task : tasks_) {
        content += task.serialize() + '\n';
    }

    // Записываем через write (UNIX API)
    // Усекаем файл до нуля
    if (ftruncate(fd_, 0) == -1) {
        std::cerr << "Ошибка усечения файла: " << strerror(errno) << std::endl;
        return false;
    }
    lseek(fd_, 0, SEEK_SET);

    const char* data = content.c_str();
    size_t total = content.size();
    ssize_t written = 0;
    while (written < static_cast<ssize_t>(total)) {
        ssize_t result = write(fd_, data + written, total - written);
        if (result == -1) {
            std::cerr << "Ошибка записи файла: " << strerror(errno) << std::endl;
            return false;
        }
        written += result;
    }

    // Принудительная синхронизация (fsync)
    if (fsync(fd_) == -1) {
        std::cerr << "Ошибка fsync: " << strerror(errno) << std::endl;
        // Не фатально
    }
    return true;
}

int TaskManager::add_task(const std::string& description) {
    int id = next_id_++;
    tasks_.emplace_back(id, description);
    return id;
}

bool TaskManager::delete_task(int id) {
    auto it = std::remove_if(tasks_.begin(), tasks_.end(),
                             [id](const Task& t) { return t.id() == id; });
    if (it == tasks_.end()) return false;
    tasks_.erase(it, tasks_.end());
    return true;
}

bool TaskManager::mark_done(int id) {
    Task* task = find_task(id);
    if (!task) return false;
    task->set_done(true);
    return true;
}

Task* TaskManager::find_task(int id) {
    auto it = std::find_if(tasks_.begin(), tasks_.end(),
                           [id](const Task& t) { return t.id() == id; });
    return it != tasks_.end() ? &(*it) : nullptr;
}