# MyTerm
This is a lightweight C++ terminal under active development. Despite its early stage, it runs smoothly and mimics classic Linux terminals with a minimal look. It aims for fast performance, cross-platform support, and future customization. Already stable, it serves as a solid base for improvements. Contributions are welcome!

# ejemplo de configuracion en vscode de la terminal
{
    "workbench.colorTheme": "Monokai",
    "workbench.iconTheme": "material-icon-theme",
    "editor.fontSize": 12,
    "editor.fontFamily": "'JetBrainsMono Nerd Font', Consolas, 'Courier New', monospace",
    "terminal.integrated.profiles.windows": {
        "MyTerm": {
            "path": "D:/mprojects/mySpace/cppProjects/myTerm/bin/myterm.exe"
        }
    },
    "terminal.integrated.defaultProfile.windows": "MyTerm"
}