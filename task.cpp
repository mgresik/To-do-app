#include "task.hpp"
#include <sstream>
#include <iomanip>

Task::Task(int id, std::string description, bool done,
           std::optional<std::chrono::system_clock::time_point> created)
    : id_(id), description_(std::move(description)), done_(done),
      created_(created.value_or(std::chrono::system_clock::now())) {}

std::string Task::serialize() const {
    std::ostringstream oss;
    auto time_t = std::chrono::system_clock::to_time_t(created_);
    oss << id_ << '|' << done_ << '|' << time_t << '|' << description_;
    return oss.str();
}

Task Task::deserialize(const std::string& line) {
    std::istringstream iss(line);
    std::string token;
    int id;
    bool done;
    std::time_t time_t;
    std::string desc;

    std::getline(iss, token, '|');
    id = std::stoi(token);
    std::getline(iss, token, '|');
    done = (token == "1");
    std::getline(iss, token, '|');
    time_t = std::stoll(token);
    std::getline(iss, desc);  // остаток строки — описание

    auto created = std::chrono::system_clock::from_time_t(time_t);
    return Task(id, desc, done, created);
}