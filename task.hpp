#pragma once

#include <string>
#include <chrono>
#include <optional>

class Task {
public:
    Task(int id, std::string description, bool done = false,
         std::optional<std::chrono::system_clock::time_point> created = std::nullopt);

    int id() const { return id_; }
    const std::string& description() const { return description_; }
    bool done() const { return done_; }
    std::chrono::system_clock::time_point created() const { return created_; }

    void set_done(bool done) { done_ = done; }
    void set_description(const std::string& desc) { description_ = desc; }

    // Сериализация в строку (для записи в файл)
    std::string serialize() const;
    // Десериализация из строки
    static Task deserialize(const std::string& line);

private:
    int id_;
    std::string description_;
    bool done_;
    std::chrono::system_clock::time_point created_;
};