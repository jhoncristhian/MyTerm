#ifndef UI_H
#define UI_H

#include <string>

class Terminal; // Forward declaration

void showHelp();
void showThemes(Terminal& term);
void showPrompt(Terminal& term, const std::string& relativePath, const std::string& gitBranch);

#endif // UI_H