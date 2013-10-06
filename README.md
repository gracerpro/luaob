#Name
luaob -- obfuscator of Lua code.

#Decsription
Obfuscator for Lua code. Target code is WoW's Lua and may be read the *.toc file of Addon, but if included files are *.lua.

#Features
* [always] Remove comments
* [always] Remove duplicated chars (' ', '\n', '\t')
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
luaob [-t toc_file_name] [-gef global_exclude_functions_file_name] [-a FILE] [-dir DIRECTORY]
[-opt-one_file] [-between_lines] [-opt-add_false_code] [-opt-add_false_comment] [-opt-const_float]
[-opt-const_int] [-opt-const_string] [-opt-global_function] [-opt-local_function] [-opt-local_vars_args]

#Options
* *-t tocFileName* (The file name of *.toc file)
* *-gef globalExcludeFunctionsFileName* (The file name of file, included global function names which must be exclude from obfuscating)
* *-a FILE* (*.lua file name)
* -dir DIRECTORY (The search directory, must be one!)
* -between_lines (count of \n between a lines in one obfuscated file)
* -opt-one_file (Create one obfuscated file)
* -opt-add_false_code (If this option is sets then will be add false code, default false)
* -opt-add_false_comment (If this option is sets then will be add false comment, default false)
* -opt-const_float (If this option is sets then will be obfuscates float numbers, default false)
* -opt-const_int (If this option is sets then will be obfuscates int numbers, default false)
* -opt-const_string (If this option is sets then will be obfuscates strings, default false)
* -opt-global_function (If this option is sets then will be obfuscate global function names, default false)
* -opt-local_function (If this option is sets then will be obfuscate local function names, default false)
* -opt-local_vars_args (If this option is sets then will be obfuscate local variables and arguments, default false)


#Version
luaob 1.0

#Compare
[https://github.com/mlnlover11/XFuscator]

[http://luasrcdiet.luaforge.net/]

[http://www.lualearners.org/forum/3869]

