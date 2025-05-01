## Instructions for setting up the linters

### Cpp linter
- First, you need to install the latest version of clang separately using ```sudo apt install clang-format```
- You can format the entire project immediately, but it is recommended to format files in which changes have been made using ```clang-format -i name_file.cpp```
    - -i — edits the in-place file (without this flag, the output will be to the console)
    - --style=Google — you can specify the style (by default, it searches for .clang-format in the project)
- Checking if the code is formatted ```clang-format name_file.cpp | diff -u name_file.cpp -```
    - If the output is empty, the code has already been formatted.
    - If there are differences, the code requires formatting.
- You can create a configuration **.clang-format** using ```clang-format -style=Google -dump-config > .clang-format```. But this is not recommended because it already exists in the root of the project and it can be changed to suit your needs.

### Go linter
- Formatting the file ```gofmt -w file_name.go ``` (`-w' = write) or formatting the entire directory ```gofmt -w .```. It is not recommended to format the entire project at once.
- Checking if the code is formatted:
    - List of unformatted files ```gofmt -l .```
    - Show differences (diff) ```gofmt -d file_name.go```
