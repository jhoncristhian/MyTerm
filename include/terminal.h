#ifndef TERMINAL_H
#define TERMINAL_H

#include <string>
#include <vector>
#include <map>
#include <csignal>

struct Theme {
    std::string user_host;
    std::string directory;
    std::string branch;
};

class Terminal {
private:
    std::string currentPath;
    std::string userName;
    std::string previousPath;
    std::string computerName;
    bool showGitBranch;

    std::map<std::string, Theme> themes;
    Theme currentTheme;

    std::vector<std::string> commandHistory;
    int historyIndex = -1;

    static Terminal* instance;
    static void signalHandler(int signum);

    void getUserInfo();
    std::string getRelativePath();
    std::string getGitBranch();
    
    void initializeThemes();

    std::vector<std::string> splitCommand(const std::string& command);
    void executeCommand(const std::vector<std::string>& tokens, const std::string& originalCommand);

    void handleTabCompletion(std::string& line, size_t& cursorPos);
    std::string getLineAdvanced();

public:
    Terminal();
    void run();

    // Public accessors needed by other components
    const std::string& getCurrentPath() const { return currentPath; }
    const std::string& getUserName() const { return userName; }
    const std::string& getComputerName() const { return computerName; }
    const Theme& getCurrentTheme() const { return currentTheme; }

    // Public mutators
    void setCurrentPath(const std::string& path) { currentPath = path; }
    void setPreviousPath(const std::string& path) { previousPath = path; }
    void setCurrentTheme(const Theme& theme) { currentTheme = theme; }
    const std::string& getPreviousPath() const { return previousPath; }
    const std::map<std::string, Theme>& getThemes() const { return themes; }

    void showPrompt();
};

#endif // TERMINAL_H