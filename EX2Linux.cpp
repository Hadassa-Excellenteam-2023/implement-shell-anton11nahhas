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

//----------------------------------FUNCTIONS----------------------------------------
/**
 * this function recieves a string provided by the user which is the input, the function 
 * cleans the input from unnecessary charachters, creates a vector and stores the 
 * command in the first cell of the vector and the arguments afterwards.
 * the function returns the vector of tokens
*/
std::vector<std::string> tokenizeInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input); // create a stream object to tokenise input
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

//------------------------------------------------------------------------------------------
/**
 * this function recieves a vector of strings full of tokens, create a new vector of char*
 * to store the arguments the user provided. loops through the tokens vector and extracts
 * the command and the arguments
 * the function returns the vector of char* of arguments.
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
 * this function recieves vector of char* and frees the memory allocated for it
*/
void freeArgs(std::vector<char*>& args) {
    for (char* arg : args) {
        delete[] arg;
    }
}

//------------------------------------------------------------------------------------------
/**
 * this function recieves the first cell of the vector char* which is the command, a vector of 
 * arguments which represent the arguments, a boolean value to see if the command must run on 
 * bg or fg, and a vector of pid_t whihc will store all processes ran in bg. the function's 
 * goal is to execute the command, the execvp uses PATHS to search for the command and executes it
 * if the path is not found an error is displayed to user, it first creates a process:
 * CHILD PROCESS: uses the execvp function to execute the command with the following arguments 
 * PARENT PROCESS: checks first if the process must run in bg or fg, if fg, it waits for the 
 *                  child process to finish, if bg, it does not need to wait the process keeps
 *                  running on bg, and appends the process to the vector of bgprocesses.
 * the function as well handles errors if any failures occur.
*/
void executeCommand(const std::string& command, std::vector<char*>& args, bool isBackground,
                    std::vector<pid_t>& backgroundProcesses) {
    pid_t pid = fork();
    if(pid == 0){
        //CHILD
        if (execvp(command.c_str(), args.data()) == -1) {
            perror("Command not found.");
            exit(1);
        }
    }
    else if(pid > 0){
        //PARENT
        if (isBackground) {
            backgroundProcesses.push_back(pid); 
        } else {
            waitpid(pid, nullptr, 0);
        }
    }
    else{
        perror("Creating a process failed.");
        exit(1);
    }
}

//------------------------------------------------------------------------------------------
/**
 * this function receives a string which represents the input the user provided, and checks 
 * if the user wants to run the command on bg, it does so by checking if the user provided 
 * & at the end of the input, if so it returns true, meaning the command must run in bg, 
 * and false otherwise.
*/
bool isBackgroundCommand(const std::string& input) {
    return input.back() == '&';
}

//------------------------------------------------------------------------------------------
/**
 * this function recieves a vector of process IDs, consisting of every vector running on 
 * background. the function displays the id of the process running on bg, as well as 
 * the status. it uses signals to retrieve this type of data.
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
            } else if (WIFSIGNALED(status)) {
                std::cout << pid << "\t" << WTERMSIG(status) << "\t\tTerminated" << std::endl;
            } else if (WIFSTOPPED(status)) {
                std::cout << pid << "\t\t\t\tStopped" << std::endl;
            } else if (WIFCONTINUED(status)) {
                std::cout << pid << "\t\t\t\tRunning" << std::endl;
            }
        }
    }
}

//------------------------------------------------------------------------------------------

/**
 * the main function that handles the input recieved from the user and then calls the 
 * necessary functions that implements a dummy shell.
*/
int main() {
    std::string input; // string to store the input from the user
    bool isBg = false; // boolean varibale to indicated bg commands
    std::vector<pid_t> backgroundProcesses; // Vector to store background process IDs

    while (true) {
        std::cout << "Enter the command and arguments (or 'exit' to quit): ";
        std::getline(std::cin, input);

        if (input == "exit") {
            break; // Break the loop if "exit" is entered
        }

        isBg = isBackgroundCommand(input); // call function to see if command must run in bg

        if (input == "myjobs") {
            myjobs(backgroundProcesses);
            continue; // Continue to the next iteration of the loop
        }

        std::vector<std::string> tokens = tokenizeInput(input); // vector for tokens

        if (tokens.empty()) {
            std::cerr << "No command entered." << std::endl;
            continue; // Continue to the next iteration of the loop
        }

        std::vector<char*> args = createArgs(tokens); // call function to prepare args

        executeCommand(tokens[0], args, isBg, backgroundProcesses);

        freeArgs(args); // free args
    }

    return 0;
}
