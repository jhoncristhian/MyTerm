#ifndef COMMANDS_H
#define COMMANDS_H

#include <vector>
#include <string>

class Terminal; // Forward declaration

void listDirectory(Terminal& term, const std::vector<std::string>& tokens);
void changeDirectory(Terminal& term, const std::string& path);
void makeDirectory(const std::string& name);
void removeDirectory(const std::string& name);
void createFile(const std::string& name);
void removeFile(const std::string& name);
void showFileContent(const std::string& name);
void clearScreen();
void executeGitCommand(const std::vector<std::string>& tokens);
void changeTheme(Terminal& term, const std::string& themeName);


#endif // COMMANDS_H