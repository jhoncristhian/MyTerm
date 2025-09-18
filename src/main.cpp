#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <map>
#include <csignal> // For signal handling (Ctrl+C)

#include <iomanip> // For std::setw, std::put_time
#include <ctime>   // For std::localtime, std::time_t
#ifdef _WIN32
    #include <windows.h>
    #include <io.h>
    #include <direct.h>
    #include <conio.h> // For _getch()
    #define getcwd _getcwd
    #define chdir _chdir
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <pwd.h>
#endif

namespace fs = std::filesystem;

// Códigos de colores ANSI
namespace Colors {
    const std::string RESET = "\033[0m";
    const std::string BLACK = "\033[30m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";
    
    // Colores brillantes
    const std::string BRIGHT_BLACK = "\033[90m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_GREEN = "\033[92m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_BLUE = "\033[94m";
    const std::string BRIGHT_MAGENTA = "\033[95m";
    const std::string BRIGHT_CYAN = "\033[96m";
    const std::string BRIGHT_WHITE = "\033[97m";
    
    // Estilos
    const std::string BOLD = "\033[1m";
    const std::string DIM = "\033[2m";
    const std::string UNDERLINE = "\033[4m";
}

// Helper para colores RGB
std::string rgb(int r, int g, int b) {
    return "\033[38;2;" + std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b) + "m";
}

struct Theme {
    std::string user_host;
    std::string directory;
    std::string branch;
};
class GitBashTerminal {
private:
    std::string currentPath;
    std::string userName;
    std::string previousPath; // Added to support 'cd -'
    std::string computerName;
    bool showGitBranch;

    std::map<std::string, Theme> themes;
    Theme currentTheme;

    std::vector<std::string> commandHistory;
    int historyIndex = -1;

    // Static pointer to the current instance to be used in the signal handler
    static GitBashTerminal* instance;

    // Static signal handler for Ctrl+C
    static void signalHandler(int signum) {
        if (signum == SIGINT && instance) {
            std::cout << "\n"; // Move to a new line
            // Reset cin state in case it's in a bad state after interruption
            std::cin.clear();
            instance->showPrompt(); // Show the prompt again
            std::cout.flush(); // Ensure the prompt is displayed immediately
        }
    }

public:
    GitBashTerminal() {
        instance = this; // Set the static instance pointer
        initializeTerminal();
        getCurrentPath();
        previousPath = currentPath; // Initialize previousPath
        getUserInfo();
        showGitBranch = true;
        initializeThemes();
        signal(SIGINT, signalHandler); // Register the signal handler for Ctrl+C
    }
    
    void initializeTerminal() {
        #ifdef _WIN32
            // Habilitar colores ANSI en Windows
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
            
            // Configurar UTF-8
            SetConsoleOutputCP(CP_UTF8);
            SetConsoleCP(CP_UTF8);
        #endif
    }
    
    void getCurrentPath() {
        // Using std::filesystem::current_path() for better cross-platform support
        currentPath = fs::current_path().string();
    }
    
    void getUserInfo() {
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
    
    std::string getRelativePath() {
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
            return relativePath;
        }
        return currentPath;
    }
    
    std::string getGitBranch() {
        if (!showGitBranch) return "";
        
        fs::path current_dir = fs::current_path();
        fs::path git_dir;

        // Search for .git directory in current and parent directories
        while (true) {
            if (fs::exists(current_dir / ".git")) {
                git_dir = current_dir / ".git";
                break;
            }
            if (current_dir.has_parent_path() && current_dir != current_dir.parent_path()) {
                current_dir = current_dir.parent_path();
            } else {
                break; // Reached root
            }
        }

        if (git_dir.empty()) {
            return ""; // No .git directory found
        }

        try {
            fs::path head_file_path = git_dir / "HEAD";
            std::ifstream headFile(head_file_path);
            if (headFile.is_open()) {
                std::string line;
                std::getline(headFile, line);
                headFile.close();
                if (line.find("ref: refs/heads/") == 0) {
                    std::string branch = line.substr(16);
                    return " (" + branch + ")";
                }
            }
        } catch (...) {
            // Si hay error, simplemente no mostrar la rama
        }
        return "";
    }
    
    void showPrompt() {
        std::string relativePath = getRelativePath();
        std::string gitBranch = getGitBranch();
        
        std::cout << currentTheme.user_host << userName << "@" << computerName 
                  << Colors::RESET << ":"
                  << currentTheme.directory << relativePath 
                  << currentTheme.branch << gitBranch 
                  << Colors::RESET << "$ ";
        std::cout.flush(); // Ensure prompt is displayed immediately
    }
    
    std::vector<std::string> splitCommand(const std::string& command) {
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
    
    void executeCommand(const std::vector<std::string>& tokens, const std::string& originalCommand) {
        if (tokens.empty()) return;
        
        std::string cmd = tokens[0];
        
        if (cmd == "help") {
            showHelp();
        }
        else if (cmd == "ls" || cmd == "dir") {
            listDirectory(tokens); // Pass all tokens to handle options like -l
        }
        else if (cmd == "cd") {
            changeDirectory(tokens.size() > 1 ? tokens[1] : "~");
        }
        else if (cmd == "pwd") {
            std::cout << Colors::BRIGHT_BLUE << currentPath << Colors::RESET << std::endl;
        }
        else if (cmd == "mkdir") {
            if (tokens.size() > 1) {
                makeDirectory(tokens[1]);
            } else {
                std::cout << Colors::RED << "Error: Especifique el nombre del directorio" << Colors::RESET << std::endl;
            }
        }
        else if (cmd == "rmdir") {
            if (tokens.size() > 1) {
                removeDirectory(tokens[1]);
            } else {
                std::cout << Colors::RED << "Error: Especifique el nombre del directorio" << Colors::RESET << std::endl;
            }
        }
        else if (cmd == "touch") {
            if (tokens.size() > 1) {
                createFile(tokens[1]);
            } else {
                std::cout << Colors::RED << "Error: Especifique el nombre del archivo" << Colors::RESET << std::endl;
            }
        }
        else if (cmd == "rm") {
            if (tokens.size() > 1) {
                removeFile(tokens[1]);
            } else {
                std::cout << Colors::RED << "Error: Especifique el nombre del archivo" << Colors::RESET << std::endl;
            }
        }
        else if (cmd == "cat") {
            if (tokens.size() > 1) {
                showFileContent(tokens[1]);
            } else {
                std::cout << Colors::RED << "Error: Especifique el nombre del archivo" << Colors::RESET << std::endl;
            }
        }
        else if (cmd == "clear" || cmd == "cls") {
            clearScreen();
        }
        else if (cmd == "git") {
            executeGitCommand(tokens);
        }
        else if (cmd == "exit" || cmd == "quit") {
            std::cout << Colors::BRIGHT_CYAN << "Hasta luego!" << Colors::RESET << std::endl;
            exit(0);
        }
        else if (cmd == "theme") {
            if (tokens.size() > 1) {
                changeTheme(tokens[1]);
            } else {
                showThemes();
            }
        }
        else {
            // Intentar ejecutar como comando del sistema
            // Pasamos el comando original sin procesar a system() para preservar las comillas
            
            // Ignore SIGINT while the external command is running
            signal(SIGINT, SIG_IGN); 
            system(originalCommand.c_str());
            // Restore our custom signal handler
            signal(SIGINT, signalHandler);
        }
    }
    
    void showHelp() {
        std::cout << Colors::BRIGHT_CYAN << "====================================================================" << Colors::RESET << std::endl;
        std::cout << Colors::BOLD << Colors::BRIGHT_WHITE << "                        COMANDOS DISPONIBLES                        " << Colors::RESET << std::endl;
        std::cout << Colors::BRIGHT_CYAN << "====================================================================" << Colors::RESET << std::endl;
        
        std::vector<std::pair<std::string, std::string>> commands = {
            {"help", "Muestra esta ayuda"},
            {"ls/dir [ruta]", "Lista archivos y directorios"},
            {"cd [ruta]", "Cambia de directorio"},
            {"pwd", "Muestra el directorio actual"},
            {"mkdir <nombre>", "Crea un directorio"},
            {"rmdir <nombre>", "Elimina un directorio"},
            {"touch <archivo>", "Crea un archivo vacio"},
            {"rm <archivo>", "Elimina un archivo"},
            {"cat <archivo>", "Muestra el contenido de un archivo"},
            {"clear", "Limpia la pantalla"},
            {"git <comando>", "Ejecuta comandos de Git"},
            {"theme [nombre]", "Cambia el tema de colores"},
            {"exit/quit", "Salir del terminal"}
        };
        
        for (const auto& cmd : commands) {
            std::cout << Colors::BRIGHT_GREEN << std::left << std::setw(20) << cmd.first
                      << Colors::BRIGHT_WHITE << " | " << cmd.second << Colors::RESET << std::endl;
        }
        
        std::cout << Colors::BRIGHT_CYAN << "====================================================================" << Colors::RESET << std::endl;
    }
    
    void listDirectory(const std::vector<std::string>& tokens) {
        std::string path = ".";
        bool long_listing = false;

        for (size_t i = 1; i < tokens.size(); ++i) {
            if (tokens[i] == "-l") {
                long_listing = true;
            } else {
                path = tokens[i]; // Assume the first non-option is the path
            }
        }
        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                std::string filename = entry.path().filename().string();

                if (entry.is_directory()) {
                    if (long_listing) {
                        std::cout << "drwxr-xr-x "
                                  << std::setw(10) << "" << " "
                                  << "                    "; // Placeholder for date
                    }
                    std::cout << Colors::BRIGHT_BLUE << Colors::BOLD << filename << "/" << Colors::RESET << std::endl;
                } else {
                    std::string extension = entry.path().extension().string();
                    
                    std::string color = Colors::BRIGHT_WHITE;
                    
                    if (extension == ".cpp" || extension == ".c" || extension == ".h") {
                        color = Colors::BRIGHT_CYAN;
                    } else if (extension == ".txt") {
                        color = Colors::WHITE;
                    } else if (extension == ".exe" || extension == ".sh") {
                        color = Colors::BRIGHT_GREEN;
                    } else if (extension == ".md") {
                        color = Colors::BRIGHT_YELLOW;
                    }
                    
                    if (long_listing) {
                        // Simulated permissions. Getting real permissions is complex and platform-specific.
                        std::cout << "-rw-r--r-- ";

                        // Size
                        if (entry.is_regular_file()) {
                            std::cout << std::right << std::setw(10) << fs::file_size(entry.path()) << " ";
                        } else {
                            std::cout << std::right << std::setw(10) << "" << " ";
                        }

                        // Last modified time
                        auto ftime = fs::last_write_time(entry.path());
                        auto sys_time_point = std::chrono::system_clock::time_point(
                            std::chrono::duration_cast<std::chrono::system_clock::duration>(ftime.time_since_epoch())
                        );
                        std::time_t cftime = std::chrono::system_clock::to_time_t(sys_time_point);
                        std::cout << std::put_time(std::localtime(&cftime), "%b %d %H:%M") << "  ";

                        std::cout << color << filename << Colors::RESET << std::endl;
                    } else {
                        std::cout << color << filename << Colors::RESET << std::endl;
                    }
                }
            }
        } catch (const fs::filesystem_error& e) {
            std::cout << Colors::RED << "Error: No se pudo acceder al directorio " << path << Colors::RESET << std::endl;
        }
    }
    
    void changeDirectory(const std::string& path) {
        std::string newPath = path;
        
        if (path == "~") {
            #ifdef _WIN32
                char* home = getenv("USERPROFILE");
            #else
                char* home = getenv("HOME");
            #endif
            newPath = home ? home : "/";
        } else if (path == "-") { // Implement 'cd -'
            if (!previousPath.empty()) {
                newPath = previousPath;
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
                previousPath = currentPath; // Store current path before changing
                fs::current_path(targetPath); // Use std::filesystem to change directory
                getCurrentPath(); // Update currentPath member
            } else {
                std::cout << Colors::RED << "Error: El directorio '" << path << "' no existe o no es un directorio." << Colors::RESET << std::endl;
            }
        } catch (const fs::filesystem_error& e) {
            std::cout << Colors::RED << "Error al cambiar de directorio a '" << path << "': " << e.what() << Colors::RESET << std::endl;
        } catch (const std::exception& e) {
            std::cout << Colors::RED << "Error inesperado al cambiar de directorio: " << e.what() << Colors::RESET << std::endl;
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
            if (fs::remove_all(name) > 0) { // remove_all returns number of elements removed
                std::cout << Colors::BRIGHT_GREEN << "Directorio eliminado: " << name << Colors::RESET << std::endl;
            } else {
                std::cout << Colors::YELLOW << "Advertencia: El directorio '" << name << "' no existe o está vacío." << Colors::RESET << std::endl;
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
            if (fs::remove(name)) { // fs::remove returns true if file was removed, false otherwise
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
                std::cout << Colors::BRIGHT_BLACK << std::setw(3) << lineNum << " | " 
                          << Colors::RESET << line << std::endl;
                lineNum++;
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
        // No need to re-initialize ANSI colors after clearing screen, they persist.
    }
    
    void executeGitCommand(const std::vector<std::string>& tokens) {
        std::string gitCmd = "git";
        for (size_t i = 1; i < tokens.size(); ++i) {
            gitCmd += " " + tokens[i];
        }
        
        system(gitCmd.c_str());
    }
    
    void initializeThemes() {
        // Default theme
        themes["default"] = {rgb(46, 204, 113), rgb(52, 152, 219), rgb(241, 196, 15)}; // Verde, Azul, Amarillo
        
        // 1. Dracula
        themes["dracula"] = {rgb(189, 147, 249), rgb(139, 233, 253), rgb(80, 250, 123)}; // Morado, Cian, Verde
        // 2. Nord
        themes["nord"] = {rgb(143, 188, 187), rgb(129, 161, 193), rgb(191, 97, 106)}; // Turquesa, Azul-gris, Rojo
        // 3. Solarized
        themes["solarized"] = {rgb(38, 139, 210), rgb(133, 153, 0), rgb(211, 54, 130)}; // Azul, Verde-oliva, Magenta
        // 4. Gruvbox
        themes["gruvbox"] = {rgb(250, 189, 47), rgb(146, 131, 116), rgb(215, 95, 0)}; // Amarillo, Gris-cafe, Naranja
        // 5. Monokai
        themes["monokai"] = {rgb(249, 38, 114), rgb(166, 226, 46), rgb(102, 217, 239)}; // Rosa, Verde-lima, Cian

        // Set default theme
        currentTheme = themes["default"];
    }

    void changeTheme(const std::string& theme) {
        if (themes.count(theme)) {
            currentTheme = themes[theme];
            std::cout << Colors::BRIGHT_GREEN << "Tema cambiado a: " << theme << Colors::RESET << std::endl;
        } else {
            std::cout << Colors::RED << "Error: El tema '" << theme << "' no existe." << Colors::RESET << std::endl;
        }
    }
    
    void showThemes() {
        std::cout << Colors::BRIGHT_CYAN << "Temas disponibles:" << Colors::RESET << std::endl;
        for (const auto& pair : themes) {
            std::cout << Colors::BRIGHT_WHITE << "* " << pair.first << Colors::RESET << std::endl;
        }
    }
    
    void handleTabCompletion(std::string& line, size_t& cursorPos) {
        // Find the start of the word to complete
        size_t word_start = line.rfind(' ', cursorPos - 1);
        if (word_start == std::string::npos) {
            word_start = 0;
        } else {
            word_start++; // Start after the space
        }

        std::string to_complete = line.substr(word_start, cursorPos - word_start);
        std::vector<std::string> matches;

        try {
            for (const auto& entry : fs::directory_iterator(".")) {
                std::string filename = entry.path().filename().string();
                if (filename.rfind(to_complete, 0) == 0) { // Check if filename starts with to_complete
                    matches.push_back(filename);
                }
            }
        } catch (...) { /* Ignore errors during completion */ }

        if (matches.size() == 1) {
            std::string completion = matches[0];
            if (fs::is_directory(completion)) {
                #ifdef _WIN32
                    completion += "\\";
                #else
                    completion += "/";
                #endif
            }
            line.replace(word_start, cursorPos - word_start, completion);
            cursorPos = word_start + completion.length();

            // Redraw the line
            std::cout << "\r" << std::string(80, ' '); // Clear line generously
            std::cout << "\r";
            showPrompt();
            std::cout << line;

        } else if (matches.size() > 1) {
            // Show all matches
            std::cout << std::endl;
            for (const auto& match : matches) {
                if (fs::is_directory(match)) {
                    std::cout << Colors::BRIGHT_BLUE << match << "/  " << Colors::RESET;
                } else {
                    std::cout << match << "  ";
                }
            }
            std::cout << std::endl;

            // Redraw the *full* prompt and the current line to ensure a clean state
            showPrompt();
            std::cout << line;
        }
    }
    // Advanced line reading with history support
    std::string getLineAdvanced() {
        std::string line;
        size_t cursorPos = 0;
        char c;

        #ifdef _WIN32
            while ((c = _getch()) != '\r') { // Enter key
                if (c == '\b') { // Backspace
                    if (cursorPos > 0) {
                        cursorPos--;
                        line.erase(cursorPos, 1);
                        // Redraw the line from the cursor position
                        std::cout << "\r"; // Go to beginning of line
                        showPrompt();
                        std::cout << line << " " << "\b"; // Redraw, clear last char, move cursor back
                        // Move cursor back to the correct position
                        for(size_t i = 0; i < line.length() - cursorPos; ++i) std::cout << "\b";
                    }
                } else if (c == 0 || c == -32 || c == 224) { // Arrow keys prefix
                    c = _getch();
                    if (c == 72) { // Up arrow
                        if (historyIndex > 0) {
                            historyIndex--;
                        } else if (!commandHistory.empty()) {
                            historyIndex = commandHistory.size() - 1;
                        }

                        if (historyIndex != -1) {
                            // Clear the current line from the cursor position
                            std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r"; // Clear generously
                            showPrompt();
                            line = commandHistory[historyIndex];
                            std::cout << line;
                            cursorPos = line.length();
                        }
                    } else if (c == 80) { // Down arrow
                        if (historyIndex != -1 && historyIndex < (int)commandHistory.size() - 1) {
                            historyIndex++;
                            // Clear the current line from the cursor position
                            std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r"; // Clear generously
                            showPrompt();
                            line = commandHistory[historyIndex];
                            std::cout << line;
                            cursorPos = line.length();
                        } else {
                            historyIndex = -1;
                            // Clear the current line from the cursor position
                            std::cout << "\r" << std::string(line.length() + 80, ' ') << "\r"; // Clear generously
                            showPrompt();
                            line = "";
                            cursorPos = 0;
                        }
                    } else if (c == 75) { // Left arrow
                        if (cursorPos > 0) {
                            cursorPos--;
                            std::cout << "\b";
                        }
                    } else if (c == 77) { // Right arrow
                        if (cursorPos < line.length()) {
                            cursorPos++;
                            // Use ANSI code to move cursor forward
                            std::cout << "\033[C";
                        }
                    } // Other special keys can be handled here if needed
                } else if (c == '\t') { // Tab key
                    handleTabCompletion(line, cursorPos);
                    // After completion, the line is redrawn, but we need to reposition the cursor
                    // on the console without moving the internal cursorPos. This is now handled inside handleTabCompletion
                    // but we still need to move the cursor to the right place.
                    std::cout << "\r";
                    showPrompt();
                    std::cout << line;
                    for(size_t i = 0; i < line.length() - cursorPos; ++i) std::cout << "\b";
                } else if (c >= 0 && c < 32) {
                    // Ignore other control characters (like Ctrl+X, Ctrl+Z, etc.)
                    // We already handle '\b' (backspace) and '\r' (enter)
                    continue;
                } else {
                    // Insert character at cursor position
                    line.insert(cursorPos, 1, c);
                    cursorPos++;
                    // Print the new character and the rest of the line
                    std::cout << c << line.substr(cursorPos);
                    // Move the cursor back to the correct position
                    if (cursorPos < line.length()) {
                        std::cout << "\033[" << (line.length() - cursorPos) << "D";
                    }
                }
            }
            std::cout << std::endl;
        #else // Linux/macOS
            // A proper implementation on Linux requires non-canonical mode (termios)
            // which is more complex. std::getline is used as a fallback.
            if (!std::getline(std::cin, line)) {
                // Handle EOF (Ctrl+D)
                std::cout << "exit" << std::endl;
                return "exit";
            }
        #endif

        return line;
    }

    void run() {
        std::string input;
        
        while (true) {
            showPrompt();
            historyIndex = -1; // Reset history navigation for new command

            #ifdef _WIN32
                input = getLineAdvanced();
            #else
                // On Linux, std::getline doesn't support raw key presses easily.
                // For history, a library like linenoise or readline is recommended.
                // We'll stick to getline for basic functionality.
                if (!std::getline(std::cin, input)) {
                    // Handle EOF (Ctrl+D on a new line)
                    std::cout << "exit" << std::endl;
                    break;
                }
            #endif

            // Quitar espacios en blanco al inicio y final
            input.erase(0, input.find_first_not_of(" \t"));
            input.erase(input.find_last_not_of(" \t") + 1);
            
            if (!input.empty()) {
                // Add to history
                if (commandHistory.empty() || commandHistory.back() != input) {
                    commandHistory.push_back(input);
                }

                std::vector<std::string> tokens = splitCommand(input);

                // Check for exit command after adding to history, so it's recallable
                if (!tokens.empty() && (tokens[0] == "exit" || tokens[0] == "quit")) {
                    std::cout << Colors::BRIGHT_CYAN << "Hasta luego!" << Colors::RESET << std::endl;
                    break;
                }

                executeCommand(tokens, input);
            } else {
                // If the user just presses Enter on an empty line, std::getline might be empty.
                // The Ctrl+C handler already prints a new prompt, but an empty line
                // from the user should also result in a new prompt without extra newlines.
                // The current loop structure already handles this correctly by showing a new prompt.
            }
        }
    }
};

// Initialize the static member
GitBashTerminal* GitBashTerminal::instance = nullptr;

int main() {
    try {
        GitBashTerminal terminal;
        terminal.run();
    } catch (const std::exception& e) {
        std::cout << Colors::RED << "Error fatal: " << e.what() << Colors::RESET << std::endl;
        return 1;
    }
    
    return 0;
}