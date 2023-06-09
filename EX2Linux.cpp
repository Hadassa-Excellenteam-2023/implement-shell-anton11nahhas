//---------------------------------INCLUDE SECTIONS-----------------------------------------------
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstdlib>
#include <iterator>
#include <fcntl.h>
#include <algorithm>
#include <cstring>

//----------------------------------FUNCTIONS----------------------------------------
/**
 * This function receives a string provided by the user as input. It cleans the input
 * from unnecessary characters, creates a vector, and stores the command in the first
 * element of the vector and the arguments afterwards.
 * The function returns the vector of tokens.
 */
std::vector<std::string> tokenizeInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input); // Create a stream object to tokenize input
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    // Remove the '&' character if it exists
    if (!tokens.empty() && tokens.back() == "&") {
        tokens.pop_back();
    }

    return tokens;
}


//------------------------------------------------------------------------------------------
/**
 * This function receives a vector of strings containing tokens and creates a new vector of char*
 * to store the arguments provided by the user. It loops through the tokens vector and extracts
 * the command and arguments.
 * The function returns the vector of char* arguments.
 */
std::vector<char*> createArgs(const std::vector<std::string>& tokens) {
    std::vector<char*> args(tokens.size() + 1);
    for (size_t i = 0; i < tokens.size(); i++) {
        args[i] = new char[tokens[i].size() + 1];
        std::strcpy(args[i], tokens[i].c_str());
    }
    args[tokens.size()] = nullptr;

    return args;
}

//------------------------------------------------------------------------------------------
/**
 * This function receives a vector of char* arguments and frees the memory allocated for it.
 */
void freeArgs(std::vector<char*>& args) {
    for (char* arg : args) {
        delete[] arg;
    }
}

//------------------------------------------------------------------------------------------
/**
 * This function receives the command (first element of the arguments vector), a vector of
 * arguments, a boolean value to determine if the command must run in the background or foreground,
 * and a vector of pid_t to store background processes.
 * The function executes the command using the execvp function. It handles input and output
 * redirection if requested by the user.
 * -------- PART 2 --------
 * Re-implemented this function so it can allow the user to use redirections and pipes as well.
 * The child process now checks if the args vector consists of any of these operators '<' '>'
 * '|' and accordingly uses the required system calls to fullfil the requirements if the found operator.
 */

void executeCommand(const std::string& command, std::vector<char*>& args, bool isBackground,
    std::vector<pid_t>& backgroundProcesses) {
    try {
        pid_t pid = fork();
        if (pid == 0) {
            // CHILD
            int inputFd = -1;
            auto inputIt = std::find_if(args.begin(), args.end(),
                [](const char* arg) { return arg && std::string(arg) == "<"; });
            if (inputIt != args.end() && std::next(inputIt) != args.end() && *std::next(inputIt)) {
                inputFd = open(*std::next(inputIt), O_RDONLY);
                if (inputFd == -1) {
                    perror("Failed to open input file");
                    exit(1);
                }
                dup2(inputFd, STDIN_FILENO);
                args.erase(inputIt, std::next(inputIt, 2));
            }

            int outputFd = -1;
            auto outputIt = std::find_if(args.begin(), args.end(),
                [](const char* arg) { return arg && std::string(arg) == ">"; });
            if (outputIt != args.end() && std::next(outputIt) != args.end() && *std::next(outputIt)) {
                outputFd = open(*std::next(outputIt), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (outputFd == -1) {
                    perror("Failed to open output file");
                    exit(1);
                }
                dup2(outputFd, STDOUT_FILENO);
                args.erase(outputIt, std::next(outputIt, 2));
            }

            int pipeIdx = -1;
            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i] && std::string(args[i]) == "|") {
                    pipeIdx = i;
                    break;
                }
            }

            if (pipeIdx != -1) {
                std::vector<char*> leftArgs(args.begin(), args.begin() + pipeIdx);
                std::vector<char*> rightArgs(args.begin() + pipeIdx + 1, args.end());

                int pipefd[2];
                if (pipe(pipefd) == -1) {
                    perror("Pipe failed");
                    exit(1);
                }

                pid_t childPid = fork();
                if (childPid == 0) {
                    close(pipefd[1]);
                    dup2(pipefd[0], STDIN_FILENO);
                    close(pipefd[0]);

                    execvp(rightArgs[0], rightArgs.data());
                    perror("Command not found.");
                    exit(1);
                }
                else if (childPid > 0) {
                    close(pipefd[0]);
                    dup2(pipefd[1], STDOUT_FILENO);
                    close(pipefd[1]);

                    execvp(leftArgs[0], leftArgs.data());
                    perror("Command not found.");
                    exit(1);
                }
                else {
                    perror("Creating a process failed.");
                    exit(1);
                }
            }
            else {
                // Execute the command
                if (args.size() > 0 && args[0]) {
                    execvp(args[0], args.data());
                    perror("Command not found.");
                    exit(1);
                }
                else {
                    std::cerr << "Invalid command." << std::endl;
                    exit(1);
                }
            }
        }
        else if (pid > 0) {
            // PARENT
            if (isBackground) {
                backgroundProcesses.push_back(pid);
                std::cout << "Background process with PID " << pid << " started." << std::endl;
            }
            else {
                waitpid(pid, nullptr, 0);
            }
        }
        else {
            perror("Creating a process failed.");
            exit(1);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        exit(1);
    }
}

//------------------------------------------------------------------------------------------
/**
 * This function receives a string representing the input provided by the user and checks if
 * the user wants to run the command in the background. It does so by checking if the user
 * provided '&' at the end of the input. If '&', the function returns true indicating that
 * the command must run in the background; otherwise, it returns false.
 */
bool isBackgroundCommand(const std::string& input) {
    return (!input.empty() && input.back() == '&');
}

//------------------------------------------------------------------------------------------
/**
 * This function receives a vector of process IDs consisting of every process running in
 * the background. The function displays the ID and status of each background process
 * using signals.
 */
void myjobs(const std::vector<pid_t>& backgroundProcesses) {
    std::cout << "Background processes:" << std::endl;
    std::cout << "PID\tExit Status\tStatus" << std::endl;

    // Check the status of each background process
    for (pid_t pid : backgroundProcesses) {
        int status;
        int result = waitpid(pid, &status, WNOHANG);
        if (result != -1) {
            if (WIFEXITED(status)) {
                std::cout << pid << "\t" << WEXITSTATUS(status) << "\t\tExited" << std::endl;
            }
            else if (WIFSIGNALED(status)) {
                std::cout << pid << "\t" << WTERMSIG(status) << "\t\tTerminated" << std::endl;
            }
            else if (WIFSTOPPED(status)) {
                std::cout << pid << "\t\t\t\tStopped" << std::endl;
            }
            else if (WIFCONTINUED(status)) {
                std::cout << pid << "\t\t\t\tRunning" << std::endl;
            }
        }
    }
}

//------------------------------------------------------------------------------------------

/**
 * The main function handles the input received from the user and calls the necessary
 * functions to implement a basic shell.
 */
int main() {
    std::string input;
    bool isBg = false;
    std::vector<pid_t> backgroundProcesses;

    while (true) {
        std::cout << "Enter the command and arguments (or 'exit' to quit): ";
        std::getline(std::cin, input);

        if (input == "exit") {
            break;
        }

        isBg = isBackgroundCommand(input);

        if (input == "myjobs") {
            myjobs(backgroundProcesses);
            continue;
        }

        std::vector<std::string> tokens = tokenizeInput(input);

        if (tokens.empty()) {
            std::cerr << "No command entered." << std::endl;
            continue;
        }

        std::vector<char*> args = createArgs(tokens);

        executeCommand(tokens[0], args, isBg, backgroundProcesses);

        freeArgs(args);
    }

    return 0;
}