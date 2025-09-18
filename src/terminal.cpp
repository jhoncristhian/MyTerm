#include "terminal.h"
#include "commands.h"
#include "ui.h"
#include "utils.h"

#include <iostream>
#include <sstream>
#include <filesystem>
#include <fstream>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <direct.h>
    #include <conio.h>
    #define getcwd _getcwd
    #define chdir _chdir
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
    #include <termios.h> // Para el modo no canónico en Linux
#endif

namespace fs = std::filesystem;

// Initialize the static member
Terminal* Terminal::instance = nullptr;

Terminal::Terminal() {
    instance = this;
    initializeTerminal();
    currentPath = fs::current_path().string();
    previousPath = currentPath;
    getUserInfo();
    showGitBranch = true;
    initializeThemes();
    signal(SIGINT, signalHandler);
}

void Terminal::signalHandler(int signum) {
    if (signum == SIGINT && instance) {
        std::cout << "\n";
        std::cin.clear();
        instance->showPrompt();
        std::cout.flush();
    }
}

void Terminal::getUserInfo() {
    #ifdef _WIN32
        char* user = getenv("USERNAME");
        char* computer = getenv("COMPUTERNAME");
        userName = user ? user : "Usuario";
        computerName = computer ? computer : "PC";
    #else
        struct passwd *pw = getpwuid(getuid());
        userName = pw ? pw->pw_name : "usuario";
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        computerName = hostname;
    #endif
}

std::string Terminal::getRelativePath() {
    std::string homePath;
    #ifdef _WIN32
        char* home = getenv("USERPROFILE");
        homePath = home ? home : "";
    #else
        char* home = getenv("HOME");
        homePath = home ? home : "";
    #endif
    
    if (!homePath.empty() && currentPath.find(homePath) == 0) {
        std::string relativePath = "~" + currentPath.substr(homePath.length());
        // On Windows, replace backslashes with forward slashes for consistency
        std::replace(relativePath.begin(), relativePath.end(), '\\', '/');
        return relativePath;
    }
    std::string displayPath = currentPath;
    std::replace(displayPath.begin(), displayPath.end(), '\\', '/');
    return displayPath;
}

std::string Terminal::getGitBranch() {
    if (!showGitBranch) return "";
    
    fs::path current_dir = fs::current_path();
    fs::path git_dir;

    while (true) {
        if (fs::exists(current_dir / ".git")) {
            git_dir = current_dir / ".git";
            break;
        }
        if (current_dir.has_parent_path() && current_dir != current_dir.parent_path()) {
            current_dir = current_dir.parent_path();
        } else {
            break;
        }
    }

    if (git_dir.empty()) return "";

    std::string branch_name;
    try {
        fs::path head_file_path = git_dir / "HEAD";
        std::ifstream headFile(head_file_path);
        if (headFile.is_open()) {
            std::string line;
            std::getline(headFile, line);
            headFile.close();
            if (line.rfind("ref: refs/heads/", 0) == 0) {
                branch_name = line.substr(16);
            }
        }
    } catch (...) {}

    if (branch_name.empty()) return ""; // Si no se encontró rama (o no es un repo git), no mostrar nada

    std::string state;
    if (fs::exists(git_dir / "rebase-merge") || fs::exists(git_dir / "rebase-apply")) {
        state = " | REBASE";
    } else if (fs::exists(git_dir / "MERGE_HEAD")) {
        state = " | MERGE";
    }

    return " (" + branch_name + state + ")";
}

void Terminal::showPrompt() {
    ::showPrompt(*this, getRelativePath(), getGitBranch());
}

std::vector<std::string> Terminal::splitCommand(const std::string& command) {
    std::vector<std::string> tokens;
    std::string current_token;
    bool in_quotes = false;

    for (char c : command) {
        if (c == ' ' && !in_quotes) {
            if (!current_token.empty()) {
                tokens.push_back(current_token);
                current_token.clear();
            }
        } else if (c == '"') {
            in_quotes = !in_quotes;
        } else {
            current_token += c;
        }
    }

    if (!current_token.empty()) {
        tokens.push_back(current_token);
    }

    return tokens;
}

void Terminal::executeCommand(const std::vector<std::string>& tokens, const std::string& originalCommand) {
    if (tokens.empty()) return;
    
    std::string cmd = tokens[0];
    
    if (cmd == "help") showHelp();
    else if (cmd == "ls" || cmd == "dir") listDirectory(*this, tokens);
    else if (cmd == "cd") changeDirectory(*this, tokens.size() > 1 ? tokens[1] : "~");
    else if (cmd == "pwd") std::cout << Colors::BRIGHT_BLUE << currentPath << Colors::RESET << std::endl;
    else if (cmd == "mkdir") {
        if (tokens.size() > 1) makeDirectory(tokens[1]);
        else std::cout << Colors::RED << "Error: Especifique el nombre del directorio" << Colors::RESET << std::endl;
    }
    else if (cmd == "rmdir") {
        if (tokens.size() > 1) removeDirectory(tokens[1]);
        else std::cout << Colors::RED << "Error: Especifique el nombre del directorio" << Colors::RESET << std::endl;
    }
    else if (cmd == "touch") {
        if (tokens.size() > 1) createFile(tokens[1]);
        else std::cout << Colors::RED << "Error: Especifique el nombre del archivo" << Colors::RESET << std::endl;
    }
    else if (cmd == "rm") {
        if (tokens.size() > 1) removeFile(tokens[1]);
        else std::cout << Colors::RED << "Error: Especifique el nombre del archivo" << Colors::RESET << std::endl;
    }
    else if (cmd == "cat") {
        if (tokens.size() > 1) showFileContent(tokens[1]);
        else std::cout << Colors::RED << "Error: Especifique el nombre del archivo" << Colors::RESET << std::endl;
    }
    else if (cmd == "clear" || cmd == "cls") clearScreen();
    else if (cmd == "git") executeGitCommand(tokens);
    else if (cmd == "theme") {
        if (tokens.size() > 1) changeTheme(*this, tokens[1]);
        else ::showThemes(*this);
    }
    else {
        signal(SIGINT, SIG_IGN); 
        system(originalCommand.c_str());
        signal(SIGINT, signalHandler);
    }
}

void Terminal::initializeThemes() {
    themes["default"] = {rgb(46, 204, 113), rgb(52, 152, 219), rgb(241, 196, 15)};
    themes["dracula"] = {rgb(189, 147, 249), rgb(139, 233, 253), rgb(80, 250, 123)};
    themes["nord"] = {rgb(143, 188, 187), rgb(129, 161, 193), rgb(191, 97, 106)};
    themes["solarized"] = {rgb(38, 139, 210), rgb(133, 153, 0), rgb(211, 54, 130)};
    themes["gruvbox"] = {rgb(250, 189, 47), rgb(146, 131, 116), rgb(215, 95, 0)};
    themes["monokai"] = {rgb(249, 38, 114), rgb(166, 226, 46), rgb(102, 217, 239)};
    currentTheme = themes["default"];
}

void Terminal::handleTabCompletion(std::string& line, size_t& cursorPos) {
    size_t word_start = line.rfind(' ', cursorPos > 0 ? cursorPos - 1 : 0);
    if (word_start == std::string::npos) word_start = 0;
    else word_start++;

    std::string to_complete_orig = line.substr(word_start, cursorPos - word_start);
    std::string to_complete_lower = toLower(to_complete_orig);
    std::vector<std::string> matches;

    try {
        for (const auto& entry : fs::directory_iterator(".")) {
            std::string filename = entry.path().filename().string();
            if (toLower(filename).rfind(to_complete_lower, 0) == 0) {
                matches.push_back(filename);
            }
        }
    } catch (...) {}

    if (matches.size() == 1) {
        std::string completion = matches[0];
        if (fs::is_directory(completion)) {
            #ifdef _WIN32
                completion += "\\";
            #else
                completion += "/";
            #endif
        }
        line.replace(word_start, to_complete_orig.length(), completion);
        cursorPos = word_start + completion.length();

        std::cout << "\r";
        showPrompt();
        std::cout << line;

    } else if (matches.size() > 1) {
        std::cout << std::endl;
        for (const auto& match : matches) {
            if (fs::is_directory(match)) {
                std::cout << Colors::BRIGHT_BLUE << match << "/  " << Colors::RESET;
            } else {
                std::cout << match << "  ";
            }
        }
        std::cout << std::endl;
        showPrompt();
        std::cout << line;
    }
}

std::string Terminal::getLineAdvanced() {
    std::string line;
    size_t cursorPos = 0;
    char c;

    #ifdef _WIN32
        while ((c = _getch()) != '\r') {
            if (c == '\b') {
                if (cursorPos > 0) {
                    cursorPos--;
                    line.erase(cursorPos, 1);
                    std::cout << "\r";
                    showPrompt();
                    std::cout << line << " " << "\b";
                    for(size_t i = 0; i < line.length() - cursorPos; ++i) std::cout << "\b";
                }
            } else if (c == 0 || c == -32 || c == 224) {
                c = _getch();
                if (c == 72) { // Up
                    if (historyIndex > 0) historyIndex--;
                    else if (!commandHistory.empty()) historyIndex = commandHistory.size() - 1;

                    if (historyIndex != -1) {
                        std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r";
                        showPrompt();
                        line = commandHistory[historyIndex];
                        std::cout << line;
                        cursorPos = line.length();
                    }
                } else if (c == 80) { // Down
                    if (historyIndex != -1 && historyIndex < (int)commandHistory.size() - 1) {
                        historyIndex++;
                        std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r";
                        showPrompt();
                        line = commandHistory[historyIndex];
                        std::cout << line;
                        cursorPos = line.length();
                    } else {
                        historyIndex = -1;
                        std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r";
                        showPrompt();
                        line = "";
                        cursorPos = 0;
                    }
                } else if (c == 75 && cursorPos > 0) { // Left
                    cursorPos--;
                    std::cout << "\b";
                } else if (c == 77 && cursorPos < line.length()) { // Right
                    cursorPos++;
                    std::cout << "\033[C";
                }
            } else if (c == '\t') {
                handleTabCompletion(line, cursorPos);
                std::cout << "\r";
                showPrompt();
                std::cout << line;
                for(size_t i = 0; i < line.length() - cursorPos; ++i) std::cout << "\b";
            } else if (c >= 0 && c < 32) {
                continue;
            } else {
                line.insert(cursorPos, 1, c);
                cursorPos++;
                std::cout << c << line.substr(cursorPos);
                if (cursorPos < line.length()) {
                    std::cout << "\033[" << (line.length() - cursorPos) << "D";
                }
            }
        }
        std::cout << std::endl;
    #else
        // Implementación para Linux/macOS usando termios para modo no canónico
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO); // Desactivar modo canónico y eco
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        while ((c = getchar()) != '\n') { // Enter key
            if (c == 127 || c == '\b') { // Backspace (127 en la mayoría de terminales Linux)
                if (cursorPos > 0) {
                    cursorPos--;
                    line.erase(cursorPos, 1);
                    std::cout << "\r";
                    showPrompt();
                    std::cout << line << " " << "\b";
                    for(size_t i = 0; i < line.length() - cursorPos; ++i) std::cout << "\b";
                }
            } else if (c == '\t') { // Tab
                handleTabCompletion(line, cursorPos);
                std::cout << "\r";
                showPrompt();
                std::cout << line;
                for(size_t i = 0; i < line.length() - cursorPos; ++i) std::cout << "\b";
            } else if (c == '\033') { // Secuencia de escape para flechas
                getchar(); // Consumir '['
                switch(getchar()) {
                    case 'A': // Up
                        if (historyIndex > 0) historyIndex--;
                        else if (!commandHistory.empty()) historyIndex = commandHistory.size() - 1;
                        if (historyIndex != -1) {
                            std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r";
                            showPrompt();
                            line = commandHistory[historyIndex];
                            std::cout << line;
                            cursorPos = line.length();
                        }
                        break;
                    case 'B': // Down
                        if (historyIndex != -1 && historyIndex < (int)commandHistory.size() - 1) {
                            historyIndex++;
                            std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r";
                            showPrompt();
                            line = commandHistory[historyIndex];
                            std::cout << line;
                            cursorPos = line.length();
                        } else {
                            historyIndex = -1;
                            std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r";
                            showPrompt();
                            line = "";
                            cursorPos = 0;
                        }
                        break;
                    case 'C': if (cursorPos < line.length()) { cursorPos++; std::cout << "\033[C"; } break; // Right
                    case 'D': if (cursorPos > 0) { cursorPos--; std::cout << "\b"; } break; // Left
                }
            } else if (c >= 0 && c < 32) {
                continue;
            } else {
                line.insert(cursorPos, 1, c);
                cursorPos++;
                std::cout << c << line.substr(cursorPos);
                if (cursorPos < line.length()) {
                    std::cout << "\033[" << (line.length() - cursorPos) << "D";
                }
            }
        }
        std::cout << std::endl;
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt); // Restaurar la configuración de la terminal
    #endif
    return line;
}

void Terminal::run() {
    std::string input;
    while (true) {
        showPrompt();
        historyIndex = -1;
        input = getLineAdvanced();

        input.erase(0, input.find_first_not_of(" \t"));
        input.erase(input.find_last_not_of(" \t") + 1);
        
        if (!input.empty()) {
            if (commandHistory.empty() || commandHistory.back() != input) {
                commandHistory.push_back(input);
            }

            std::vector<std::string> tokens = splitCommand(input);
            if (!tokens.empty() && (tokens[0] == "exit" || tokens[0] == "quit")) {
                std::cout << Colors::BRIGHT_CYAN << "Hasta luego!" << Colors::RESET << std::endl;
                break;
            }
            executeCommand(tokens, input);
        }
    }
}