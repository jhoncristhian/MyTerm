#include "commands.h"
#include "terminal.h"
#include "utils.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <vector>

namespace fs = std::filesystem;

void listDirectory(Terminal& term, const std::vector<std::string>& tokens) {
    std::string path = ".";
    bool long_listing = false;

    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i] == "-l") {
            long_listing = true;
        } else {
            path = tokens[i];
        }
    }
    try {
        if (long_listing) {
            for (const auto& entry : fs::directory_iterator(path)) {
                std::string filename = entry.path().filename().string();
                if (entry.is_directory()) {
                    std::cout << "drwxr-xr-x " << std::setw(10) << "" << " " << "                    ";
                    std::cout << Colors::BRIGHT_BLUE << Colors::BOLD << filename << "/" << Colors::RESET << std::endl;
                } else {
                    std::string extension = entry.path().extension().string();
                    std::string color = Colors::BRIGHT_WHITE;
                    if (extension == ".cpp" || extension == ".c" || extension == ".h") color = Colors::BRIGHT_CYAN;
                    else if (extension == ".txt") color = Colors::WHITE;
                    else if (extension == ".exe" || extension == ".sh") color = Colors::BRIGHT_GREEN;
                    else if (extension == ".md") color = Colors::BRIGHT_YELLOW;

                    std::cout << "-rw-r--r-- ";
                    if (entry.is_regular_file()) std::cout << std::right << std::setw(10) << fs::file_size(entry.path()) << " ";
                    else std::cout << std::right << std::setw(10) << "" << " ";

                    auto ftime = fs::last_write_time(entry.path());
                    auto sys_time_point = std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(ftime.time_since_epoch()));
                    std::time_t cftime = std::chrono::system_clock::to_time_t(sys_time_point);
                    std::cout << std::put_time(std::localtime(&cftime), "%b %d %H:%M") << "  ";
                    std::cout << color << filename << Colors::RESET << std::endl;
                }
            }
        } else {
            std::vector<fs::directory_entry> entries;
            size_t max_len = 0;
            for (const auto& entry : fs::directory_iterator(path)) {
                entries.push_back(entry);
                max_len = std::max(max_len, entry.path().filename().string().length());
            }

            if (entries.empty()) return;

            int term_width = getTerminalWidth();
            int col_width = max_len + 2;
            int num_cols = (term_width > 0 && col_width > 0) ? term_width / col_width : 1;
            if (num_cols == 0) num_cols = 1;

            for (size_t i = 0; i < entries.size(); ++i) {
                const auto& entry = entries[i];
                std::string filename = entry.path().filename().string();
                std::string color = Colors::BRIGHT_WHITE;
                std::string suffix = " ";

                if (entry.is_directory()) {
                    color = Colors::BRIGHT_BLUE + Colors::BOLD;
                    suffix = "/";
                } else {
                    std::string extension = entry.path().extension().string();
                    if (extension == ".cpp" || extension == ".c" || extension == ".h") color = Colors::BRIGHT_CYAN;
                    else if (extension == ".txt") color = Colors::WHITE;
                    else if (extension == ".exe" || extension == ".sh") color = Colors::BRIGHT_GREEN;
                    else if (extension == ".md") color = Colors::BRIGHT_YELLOW;
                }

                std::cout << color << std::left << std::setw(col_width) << (filename + suffix) << Colors::RESET;

                if ((i + 1) % num_cols == 0) {
                    std::cout << std::endl;
                }
            }
            if (entries.size() % num_cols != 0) {
                std::cout << std::endl;
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << Colors::RED << "Error: No se pudo acceder al directorio " << path << Colors::RESET << std::endl;
    }
}

void changeDirectory(Terminal& term, const std::string& path) {
    std::string newPath = path;
    
    if (path == "~") {
        #ifdef _WIN32
            char* home = getenv("USERPROFILE");
        #else
            char* home = getenv("HOME");
        #endif
        newPath = home ? home : "/";
    } else if (path == "-") {
        if (!term.getPreviousPath().empty()) {
            newPath = term.getPreviousPath();
        } else {
            std::cout << Colors::RED << "Error: No hay directorio anterior para volver." << Colors::RESET << std::endl;
            return;
        }
    }
    
    try {
        fs::path targetPath = newPath;
        if (!targetPath.is_absolute()) {
            targetPath = fs::current_path() / targetPath;
        }

        if (fs::is_directory(targetPath)) {
            term.setPreviousPath(term.getCurrentPath());
            fs::current_path(targetPath);
            term.setCurrentPath(fs::current_path().string());
        } else {
            std::cout << Colors::RED << "Error: El directorio '" << path << "' no existe o no es un directorio." << Colors::RESET << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << Colors::RED << "Error al cambiar de directorio a '" << path << "': " << e.what() << Colors::RESET << std::endl;
    }
}

void makeDirectory(const std::string& name) {
    try {
        if (fs::create_directory(name)) {
            std::cout << Colors::BRIGHT_GREEN << "Directorio creado: " << name << Colors::RESET << std::endl;
        } else {
            std::cout << Colors::YELLOW << "Advertencia: El directorio '" << name << "' ya existe." << Colors::RESET << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << Colors::RED << "Error al crear el directorio '" << name << "': " << e.what() << Colors::RESET << std::endl;
    }
}

void removeDirectory(const std::string& name) {
    try {
        if (fs::remove_all(name) > 0) {
            std::cout << Colors::BRIGHT_GREEN << "Directorio eliminado: " << name << Colors::RESET << std::endl;
        } else {
            std::cout << Colors::YELLOW << "Advertencia: El directorio '" << name << "' no existe." << Colors::RESET << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << Colors::RED << "Error al eliminar el directorio '" << name << "': " << e.what() << Colors::RESET << std::endl;
    }
}

void createFile(const std::string& name) {
    std::ofstream file(name);
    if (file.is_open()) {
        file.close();
        std::cout << Colors::BRIGHT_GREEN << "Archivo creado: " << name << Colors::RESET << std::endl;
    } else {
        std::cout << Colors::RED << "Error: No se pudo crear el archivo " << name << Colors::RESET << std::endl;
    }
}

void removeFile(const std::string& name) {
    try {
        if (fs::remove(name)) {
            std::cout << Colors::BRIGHT_GREEN << "Archivo eliminado: " << name << Colors::RESET << std::endl;
        } else {
            std::cout << Colors::YELLOW << "Advertencia: El archivo '" << name << "' no existe." << Colors::RESET << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cout << Colors::RED << "Error al eliminar el archivo '" << name << "': " << e.what() << Colors::RESET << std::endl;
    }
}

void showFileContent(const std::string& name) {
    std::ifstream file(name);
    if (file.is_open()) {
        std::cout << Colors::BRIGHT_CYAN << "Contenido de " << name << ":" << Colors::RESET << std::endl;
        std::cout << Colors::BRIGHT_BLACK << "-------------------------------------" << Colors::RESET << std::endl;
        std::string line;
        int lineNum = 1;
        while (std::getline(file, line)) {
            std::cout << Colors::BRIGHT_BLACK << std::setw(3) << lineNum++ << " | " << Colors::RESET << line << std::endl;
        }
        std::cout << Colors::BRIGHT_BLACK << "-------------------------------------" << Colors::RESET << std::endl;
        file.close();
    } else {
        std::cout << Colors::RED << "Error: No se pudo leer el archivo " << name << Colors::RESET << std::endl;
    }
}

void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void executeGitCommand(const std::vector<std::string>& tokens) {
    std::string gitCmd = "git";
    for (size_t i = 1; i < tokens.size(); ++i) {
        gitCmd += " " + tokens[i];
    }
    system(gitCmd.c_str());
}

void changeTheme(Terminal& term, const std::string& themeName) {
    const auto& themes = term.getThemes();
    if (themes.count(themeName)) {
        term.setCurrentTheme(themes.at(themeName));
        std::cout << Colors::BRIGHT_GREEN << "Tema cambiado a: " << themeName << Colors::RESET << std::endl;
    } else {
        std::cout << Colors::RED << "Error: El tema '" << themeName << "' no existe." << Colors::RESET << std::endl;
    }
}