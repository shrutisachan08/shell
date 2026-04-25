# 🐚MySH - A Custom UNIX-Style Shell in C++
**MySH** is a minimalist yet feature-rich shell built from scratch in C++. It mimics core behavior of UNIX shells, supporting command execution, pipes, I/O redirection, job control, history, and tab completion.

> This project was created to deepen understanding of OS-level process control, signals, file descriptors, and user interaction with command-line interfaces. It offers a hands-on implementation of features found in modern shells like Bashor Zsh, without external dependencies beyond standard libraries and GNU Readline.

---

## ✨ Features
- Run external programs (e.g. `ls`, `cat`, `grep`)
- Built-in commands: `cd`, `pwd`, `clear`, `exit`, `history`, `jobs`, `fg`, `bg`, `kill`
- Persistent history across sessions (`~/.mysh_history`)
- ⛓️ Piping: `command1 | command2`
- Input/Output redirection: `command > out.txt`, `command < in.txt`
- Background process execution: `command &`
- Job control: list jobs, bring to foreground or background
- Tab completion for built-in commands and file names

---

## 🚀 Getting Started 

### 📦 Prerequisites

Ensure you have:

- **C++ compiler**
- **GNU Readline library** for input and history:

```bash
    sudo apt install libreadline-dev
```

### 🔧 Getting Started
- 1. Clone the repository:
    ```bash
        git clone https://github.com/akshitamishra13/custom_shell
        cd mysh
    ```
- 2. Compile the shell:
    ```bash
        g++ shell.cpp -o mysh -lreadline
    ```
- 3. Start the shell:
    ```bash
        ./mysh
    ```

### 📷Example Usage
```bash
    mysh> pwd
    /home/user

    mysh> ls -l | grep ".cpp" > cpp_files.txt

    mysh> ./long_running_task & 
    Started background process with PID: 4321

    mysh> jobs
    [1] Running ./long_running_task (PID: 4321)

    mysh> fg 1
```

### 💡Tab Completion
Press `Tab` to auto-complete:

- Built-in commands like `cd`, `exit`, `history`, `jobs`, etc.
- Files and directories in the current working directory

### 🗃️ Project Structure 
```bash 
    custom_shell/
    ├── README.md
    ├── custom_shell
    ├── my_shell
    ├── out.txt
    └── shell.cpp
```