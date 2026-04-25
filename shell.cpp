#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <fstream>
#include <cstdlib>
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>

using namespace std;

struct Job {
    pid_t pid;
    string command;
    bool running;

    Job(pid_t _pid, const string& _cmd, bool _run) : pid(_pid), command(_cmd), running(_run) {}
};

vector<Job> jobs;
vector<string> history;

pid_t current_pid = -1;

void handle_signal(int sig) {
    if (current_pid > 0) {
        kill(current_pid, SIGINT);
    } else {
        cout << "\nmysh> ";
        cout.flush();
    }
}

void load_history(const string& filename) {
    ifstream infile(filename);
    string line;
    while (getline(infile, line)) {
        if (!line.empty()) {
            add_history(line.c_str());
            history.push_back(line);
        }
    }
}

void save_history(const string& filename) {
    ofstream outfile(filename);
    for (const auto& cmd : history) {
        outfile << cmd << endl;
    }
}


void shellLoop();


char* command_generator(const char* text, int state);
char** my_completion(const char* text, int start, int end);

const char* builtin_cmds[] = {
    "cd", "pwd", "exit", "clear", "history", "jobs", "kill", "fg", "bg", nullptr
};

char* command_generator(const char* text, int state) {
    static DIR* dir;
    static struct dirent* entry;
    static int len;

    if (state == 0) {
        dir = opendir(".");
        len = strlen(text);
    }
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry -> d_name, text, len) == 0) {
            return strdup(entry -> d_name);
        }
    }

    closedir(dir);
    return NULL;
}

char** custom_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_generator);
}

int main() {
    cout << "Welcome to our custom shell!" << endl;

    string home = getenv("HOME");
    string histfile = home + "/.mysh_history";

    load_history(histfile);

    signal(SIGINT, handle_signal);
    rl_attempted_completion_function = custom_completion;

    shellLoop();

    save_history(histfile);
    return 0;
}

char** my_completion(const char* text, int start, int end) {
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, command_generator);
}

void shellLoop() {
    string line;

    while (true) {
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        cout << "[" << cwd << "]$ ";
        char* input = readline("mysh> ");
        if (!input) {
            cout << "\n";
            break;
        }
        line = input;
        if (!line.empty()) {
            add_history(line.c_str());
            if (line != "history") {
                history.push_back(line);
            }
        }
        free(input);

        if (!line.empty() && line != "history") {
            history.push_back(line);
        }

        if (cin.eof()) {
            cout << "\n";
            break;
        }
        
        if (line.empty()) continue;

        stringstream ss(line);
        string token;
        vector<string> tokens;
        while (ss >> token) {
            tokens.push_back(token);
        }

        bool has_pipe = false;
        vector<string> left_tokens, right_tokens;

        for (size_t i = 0; i < tokens.size(); i++) {
            if (tokens[i] == "|") {
                has_pipe = true;
                left_tokens.assign(tokens.begin(), tokens.begin() + i);
                right_tokens.assign(tokens.begin() + i + 1, tokens.end());
                break;
            }
        }

        bool run_in_background = false;
        if (!tokens.empty() && tokens.back() == "&") {
            run_in_background = true;
            tokens.pop_back();
        }

        if (tokens[0] == "exit") break;

        if (tokens[0] == "history") {
            for (size_t i = 0; i < history.size(); ++i) {
                cout << i + 1 << " " << history[i] << endl;
            }
            continue;
        }

        if (tokens[0] == "clear") {
            system("clear");
            continue;
        }

        if (tokens[0] == "cd") {
            const char* path = tokens.size() > 1 ? tokens[1].c_str() : getenv("HOME");
            if (chdir(path) != 0) {
                perror("cd failed");
            }
            continue;
        }

        if (tokens[0] == "pwd") {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                cout << cwd << endl;
            else    
                perror("pwd failed");
            continue;
        }

        if (tokens[0] == "jobs") {
            for (size_t i = 0; i < jobs.size(); i++) {
                cout << "[" << i + 1 << "] " << (jobs[i].running ? "Running" : "Stopped") << " " << jobs[i].command << " (PID: " << jobs[i].pid << ")\n";
            }
            continue;
        }

        if (tokens[0] == "kill" && tokens.size() == 2) {
            int job_id = stoi(tokens[1]) - 1;
            if (job_id >= 0 && job_id < jobs.size()) {
                kill(jobs[job_id].pid, SIGKILL);
            } else {
                cout << "Invalid job ID\n";
            }
            continue;
        }

        if (tokens[0] == "fg" && tokens.size() == 2) {
            int job_id = stoi(tokens[1]) - 1;
            if (job_id >= 0 && job_id < jobs.size()) {
                pid_t pid = jobs[job_id].pid;
                kill(pid, SIGCONT);
                waitpid(pid, NULL, 0);
                jobs[job_id].running = false;
            } else {
                cout << "Invalid job ID\n";
            }

            continue;
        }

        if (tokens[0] == "bg" && tokens.size() == 2) {
            int job_id = stoi(tokens[1]) - 1;
            if (job_id >= 0 && job_id < jobs.size()) {
                pid_t pid = jobs[job_id].pid;
                kill(pid, SIGCONT);
                jobs[job_id].running = true;
                cout << "Resumed job [" << job_id + 1 << "] in background\n";
            } else {
                cout << "Invalid job ID\n";
            }
            continue;
        }

        if (has_pipe) {
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe failed");
                continue;
            }
            
            pid_t pid1 = fork();
            if (pid1 == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);
                
                vector<char*> args1;
                for (const auto& arg: left_tokens) {
                    char* cstr = new char[arg.size() + 1];
                    strcpy(cstr, arg.c_str());
                    args1.push_back(cstr);
                }
                args1.push_back(nullptr);
                
                execvp(args1[0], args1.data());
                perror("exec failed");
                exit(1);
            }
            
            pid_t pid2 = fork();
            if (pid2 == 0) {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                
                vector<char*> args2;
                for (const auto& arg: right_tokens) {
                    char* cstr = new char[arg.size() + 1];
                    strcpy(cstr, arg.c_str());
                    args2.push_back(cstr);
                }
                args2.push_back(nullptr);
                
                execvp(args2[0], args2.data());
                perror("exec failed");
                exit(1);
            }

            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            continue;
        }

        int in_fd = -1, out_fd = -1;
        bool has_input_redir = false, has_output_redir = false;

        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i] == "<" && i + 1 < tokens.size()) {
                has_input_redir = true;
                in_fd = open(tokens[i + 1].c_str(), O_RDONLY);
                if (in_fd < 0) perror("Input redirection failed");
                tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
                i--;
            } else if (tokens[i] == ">" && i + 1 < tokens.size()) {
                has_output_redir = true;
                out_fd = open(tokens[i + 1].c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
                if (out_fd < 0) perror("Output redirection failed");
                tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
                i--;
            }
        }

        vector<char*> args;
        for (const auto& arg: tokens) {
            char* cstr = new char[arg.size() + 1];
            strcpy(cstr, arg.c_str());
            args.push_back(cstr);
        }
        args.push_back(nullptr);


        pid_t pid = fork();
        current_pid = pid;
        if (pid == 0) {
            if (has_input_redir) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (has_output_redir) {
                dup2(out_fd, STDOUT_FILENO);
                close(out_fd);
            }
            if (has_input_redir) close(in_fd);
            if (has_output_redir) close(out_fd);
            execvp(args[0], args.data());
            perror("exec failed");
            exit(1);
        } else if (pid > 0) {
            current_pid = pid;
            if (!run_in_background) {
                waitpid(pid, NULL, 0);
            } else {
                jobs.push_back(Job(pid, line, true));
                cout << "Started background process with PID: " << pid << endl;
            }
            current_pid = -1;
        } else {
            perror("fork failed");
        }
        current_pid = -1;

        for (char* arg: args) delete[] arg;
    }
}