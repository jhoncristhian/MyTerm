#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#endif

// Códigos de colores ANSI
namespace Colors {
    extern const std::string RESET;
    extern const std::string RED;
    extern const std::string GREEN;
    extern const std::string YELLOW;
    extern const std::string BLUE;
    extern const std::string MAGENTA;
    extern const std::string CYAN;
    extern const std::string WHITE;
    extern const std::string BRIGHT_BLACK;
    extern const std::string BRIGHT_RED;
    extern const std::string BRIGHT_GREEN;
    extern const std::string BRIGHT_YELLOW;
    extern const std::string BRIGHT_BLUE;
    extern const std::string BRIGHT_MAGENTA;
    extern const std::string BRIGHT_CYAN;
    extern const std::string BRIGHT_WHITE;
    extern const std::string BOLD;
    extern const std::string DIM;
    extern const std::string UNDERLINE;
}

// Helper para colores RGB
std::string rgb(int r, int g, int b);

// Helper para convertir a minúsculas
std::string toLower(std::string s);

// Helper para obtener el ancho de la terminal
int getTerminalWidth();

// Helper para inicializar la terminal (colores, UTF-8)
void initializeTerminal();


#endif // UTILS_H