#Name
luaob -- obfuscator of Lua code.

#Decsription
Obfuscator for Lua code. Target code is WoW's Lua and may be read the *.toc file of Addon, but if included files are *.lua.

#Features
* [always] Remove comments
* [always] Remove extra whitespace
* [always] Remove extra new line symbols and replace him to semicolon
* Replace global function names
* Replace local variable names and formal parameter names
* Replace string value to complex expression (escapes codes)
* Add false comment

TODO:
* Replace integer number to complex expression
* Replace float number to complex expression
* Add false code

#Synopsis
luaob [-t tocFileName] [-gef globalExcludeFunctionsFileName] [-a FILE] [-dir DIRECTORY]

#Options
* -t tocFileName (The file name of *.toc file)
* -gef globalExcludeFunctionsFileName (The file name of file, included global function names which must be exclude from obfuscating)
* -a FILE (*.lua file name)
* -dir DIRECTORY (The search directory, must be one!)

#Version
luaob 1.0

#Compare
[https://github.com/mlnlover11/XFuscator]
[http://luasrcdiet.luaforge.net/]
[http://www.lualearners.org/forum/3869]

