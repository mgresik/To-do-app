#include "task_manager.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <csignal>
#include <cstring>
#include <unistd.h>
#include <getopt.h>

// Глобальный указатель на менеджер для обработчика сигнала
static TaskManager* g_manager = nullptr;
static bool g_running = true;

// Обработчик сигнала SIGINT (Ctrl+C)
void signal_handler(int signum) {
    if (signum == SIGINT) {
        std::cout << "\nПолучен SIGINT, сохраняем задачи..." << std::endl;
        if (g_manager) {
            g_manager->save();
        }
        g_running = false;
    }
}

void print_help() {
    std::cout << "Todo Tracker - консольный менеджер задач\n"
              << "Использование:\n"
              << "  todo [опции] [команда] [аргументы]\n"
              << "Команды (если не указаны, запускается интерактивный режим):\n"
              << "  add <описание>        - добавить задачу\n"
              << "  list                  - показать все задачи\n"
              << "  done <id>             - отметить задачу выполненной\n"
              << "  remove <id>           - удалить задачу\n"
              << "  help                  - показать эту справку\n"
              << "Опции:\n"
              << "  -f <файл>             - файл данных (по умолчанию ~/.todo.txt)\n"
              << "  -h                    - показать справку\n";
}

// Разбор командной строки (неинтерактивный режим)
bool execute_command(TaskManager& manager, const std::vector<std::string>& args) {
    if (args.empty()) return false; // интерактивный

    const std::string& cmd = args[0];
    if (cmd == "add" && args.size() >= 2) {
        std::string desc = args[1];
        for (size_t i = 2; i < args.size(); ++i) desc += " " + args[i];
        int id = manager.add_task(desc);
        manager.save();
        std::cout << "Задача добавлена с ID " << id << std::endl;
        return true;
    } else if (cmd == "list") {
        const auto& tasks = manager.tasks();
        if (tasks.empty()) {
            std::cout << "Нет задач." << std::endl;
        } else {
            for (const auto& t : tasks) {
                std::cout << "[" << t.id() << "] " << (t.done() ? "[x]" : "[ ]")
                          << " " << t.description() << std::endl;
            }
        }
        return true;
    } else if (cmd == "done" && args.size() == 2) {
        int id = std::stoi(args[1]);
        if (manager.mark_done(id)) {
            manager.save();
            std::cout << "Задача " << id << " отмечена выполненной." << std::endl;
        } else {
            std::cout << "Задача с ID " << id << " не найдена." << std::endl;
        }
        return true;
    } else if (cmd == "remove" && args.size() == 2) {
        int id = std::stoi(args[1]);
        if (manager.delete_task(id)) {
            manager.save();
            std::cout << "Задача " << id << " удалена." << std::endl;
        } else {
            std::cout << "Задача с ID " << id << " не найдена." << std::endl;
        }
        return true;
    } else if (cmd == "help") {
        print_help();
        return true;
    } else {
        std::cerr << "Неизвестная команда или недостаточно аргументов. Используйте 'help'." << std::endl;
        return false;
    }
}

int main(int argc, char* argv[]) {
    // Установка обработчика сигнала SIGINT
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // перезапускать прерванные системные вызовы
    if (sigaction(SIGINT, &sa, nullptr) == -1) {
        std::cerr << "Не удалось установить обработчик сигнала: " << strerror(errno) << std::endl;
        return 1;
    }

    // Парсинг опций командной строки
    std::string data_file = std::string(getenv("HOME")) + "/.todo.txt";
    int opt;
    while ((opt = getopt(argc, argv, "f:h")) != -1) {
        switch (opt) {
            case 'f':
                data_file = optarg;
                break;
            case 'h':
                print_help();
                return 0;
            default:
                print_help();
                return 1;
        }
    }

    // Оставшиеся аргументы — команда и её параметры
    std::vector<std::string> args;
    for (int i = optind; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }

    // Создаём менеджер задач
    TaskManager manager(data_file);
    g_manager = &manager;

    // Пытаемся захватить блокировку файла (чтобы избежать одновременного запуска)
    if (!manager.lock_file()) {
        std::cerr << "Не удалось получить блокировку файла. Возможно, уже запущен экземпляр." << std::endl;
        return 1;
    }

    // Загружаем данные
    if (!manager.load()) {
        std::cerr << "Ошибка загрузки данных. Продолжаем с пустым списком." << std::endl;
    }

    // Если есть команда — выполняем и выходим
    if (!args.empty()) {
        bool ok = execute_command(manager, args);
        manager.unlock_file();
        return ok ? 0 : 1;
    }

    // Интерактивный режим
    std::cout << "Todo Tracker. Введите команду (help для справки, quit для выхода)" << std::endl;
    std::string line;
    while (g_running) {
        std::cout << "> ";
        std::getline(std::cin, line);
        if (line.empty()) continue;

        // Разбиваем на слова
        std::istringstream iss(line);
        std::vector<std::string> tokens;
        std::string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        if (tokens.empty()) continue;

        const std::string& cmd = tokens[0];
        if (cmd == "quit" || cmd == "exit") {
            manager.save();
            break;
        } else if (cmd == "help") {
            print_help();
        } else if (cmd == "add" && tokens.size() >= 2) {
            std::string desc = tokens[1];
            for (size_t i = 2; i < tokens.size(); ++i) desc += " " + tokens[i];
            int id = manager.add_task(desc);
            manager.save();
            std::cout << "Задача добавлена с ID " << id << std::endl;
        } else if (cmd == "list") {
            const auto& tasks = manager.tasks();
            if (tasks.empty()) {
                std::cout << "Нет задач." << std::endl;
            } else {
                for (const auto& t : tasks) {
                    std::cout << "[" << t.id() << "] " << (t.done() ? "[x]" : "[ ]")
                              << " " << t.description() << std::endl;
                }
            }
        } else if (cmd == "done" && tokens.size() == 2) {
            try {
                int id = std::stoi(tokens[1]);
                if (manager.mark_done(id)) {
                    manager.save();
                    std::cout << "Задача " << id << " отмечена выполненной." << std::endl;
                } else {
                    std::cout << "Задача с ID " << id << " не найдена." << std::endl;
                }
            } catch (...) {
                std::cout << "Некорректный ID." << std::endl;
            }
        } else if (cmd == "remove" && tokens.size() == 2) {
            try {
                int id = std::stoi(tokens[1]);
                if (manager.delete_task(id)) {
                    manager.save();
                    std::cout << "Задача " << id << " удалена." << std::endl;
                } else {
                    std::cout << "Задача с ID " << id << " не найдена." << std::endl;
                }
            } catch (...) {
                std::cout << "Некорректный ID." << std::endl;
            }
        } else {
            std::cout << "Неизвестная команда. Введите 'help'." << std::endl;
        }
    }

    manager.unlock_file();
    return 0;
}