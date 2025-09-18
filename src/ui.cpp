#include "ui.h"
#include "terminal.h"
#include "utils.h"

#include <iostream>
#include <vector>
#include <iomanip>

void showHelp() {
    std::cout << Colors::BRIGHT_CYAN << "====================================================================" << Colors::RESET << std::endl;
    std::cout << Colors::BOLD << Colors::BRIGHT_WHITE << "                        COMANDOS DISPONIBLES                        " << Colors::RESET << std::endl;
    std::cout << Colors::BRIGHT_CYAN << "====================================================================" << Colors::RESET << std::endl;
    
    std::vector<std::pair<std::string, std::string>> commands = {
        {"help", "Muestra esta ayuda"},
        {"ls/dir [-l] [ruta]", "Lista archivos y directorios"},
        {"cd [ruta|~|-]", "Cambia de directorio"},
        {"pwd", "Muestra el directorio actual"},
        {"mkdir <nombre>", "Crea un directorio"},
        {"rmdir <nombre>", "Elimina un directorio"},
        {"touch <archivo>", "Crea un archivo vacio"},
        {"rm <archivo>", "Elimina un archivo"},
        {"cat <archivo>", "Muestra el contenido de un archivo"},
        {"clear/cls", "Limpia la pantalla"},
        {"git <comando>", "Ejecuta comandos de Git"},
        {"theme [nombre]", "Cambia o lista los temas de colores"},
        {"exit/quit", "Salir del terminal"}
    };
    
    for (const auto& cmd : commands) {
        std::cout << Colors::BRIGHT_GREEN << std::left << std::setw(25) << cmd.first
                  << Colors::BRIGHT_WHITE << " | " << cmd.second << Colors::RESET << std::endl;
    }
    
    std::cout << Colors::BRIGHT_CYAN << "====================================================================" << Colors::RESET << std::endl;
}

void showThemes(Terminal& term) {
    std::cout << Colors::BRIGHT_CYAN << "Temas disponibles:" << Colors::RESET << std::endl;
    for (const auto& pair : term.getThemes()) {
        std::cout << Colors::BRIGHT_WHITE << "* " << pair.first << Colors::RESET << std::endl;
    }
}

void showPrompt(Terminal& term, const std::string& relativePath, const std::string& gitBranch) {
    const auto& theme = term.getCurrentTheme();
    std::cout << theme.user_host << term.getUserName() << "@" << term.getComputerName() << Colors::RESET << ":"
              << theme.directory << relativePath << theme.branch << gitBranch << Colors::RESET << "$ ";
    std::cout.flush();
}