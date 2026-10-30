
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cerrno>

class OrbitShell
{
private:
    bool running = true;

    // Parse command into arguments.
    std::vector<std::string> parseCommand(const std::string &input)
    {
        std::vector<std::string> args;
        std::string current;

        char quote = '\0';
        bool escaping = false;
        bool tokenStarted = false;

        for (char c : input)
        {
            if (escaping)
            {
                current += c;
                escaping = false;
                tokenStarted = true;
            }
            else if (c == '\\' && quote != '\'')
            {
                escaping = true;
                tokenStarted = true;
            }
            else if (quote != '\0')
            {
                if (c == quote)
                {
                    quote = '\0';
                }
                else
                {
                    current += c;
                }
            }
            else if (c == '"' || c == '\'')
            {
                quote = c;
                tokenStarted = true;
            }
            else if (c == ' ' || c == '\t')
            {
                if (tokenStarted)
                {
                    args.push_back(current);
                    current.clear();
                    tokenStarted = false;
                }
            }
            else
            {
                current += c;
                tokenStarted = true;
            }
        }

        if (escaping)
            current += '\\';

        if (quote != '\0')
        {
            std::cerr << "orbit: unmatched quote\n";
            return {};
        }

        if (tokenStarted)
            args.push_back(current);

        return args;
    }

    // Built-in: pwd
    void pwd()
    {
        char buffer[4096];

        if (getcwd(buffer, sizeof(buffer)))
        {
            std::cout << buffer << '\n';
        }
        else
        {
            perror("orbit: pwd");
        }
    }

    // Built-in: cd
    void cd(const std::vector<std::string> &args)
    {
        std::string path;

        if (args.size() < 2 || args[1] == "~")
        {
            const char *home = getenv("HOME");

            if (!home)
            {
                std::cerr << "orbit: HOME not set\n";
                return;
            }

            path = home;
        }
        else
        {
            path = args[1];
        }

        if (chdir(path.c_str()) != 0)
        {
            perror("orbit: cd");
        }
    }

    // Built-in: exit
    void exitShell(const std::vector<std::string> &args)
    {
        if (args.size() > 2)
        {
            std::cerr << "orbit: exit: too many arguments\n";
            return;
        }

        running = false;
    }

    // Execute external Linux commands.
    void executeExternal(
        const std::vector<std::string> &args)
    {
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("orbit: fork");
            return;
        }

        if (pid == 0)
        {
            std::vector<char *> argv;

            for (const auto &arg : args)
            {
                argv.push_back(
                    const_cast<char *>(arg.c_str()));
            }

            argv.push_back(nullptr);

            execvp(argv[0], argv.data());

            perror("orbit: command");
            _exit(127);
        }

        int status;

        while (waitpid(pid, &status, 0) == -1)
        {
            if (errno != EINTR)
            {
                perror("orbit: waitpid");
                return;
            }
        }
    }

    void execute(const std::vector<std::string> &args)
    {
        if (args.empty())
            return;

        const std::string &command = args[0];

        if (command == "pwd")
        {
            pwd();
        }
        else if (command == "cd")
        {
            cd(args);
        }
        else if (command == "exit")
        {
            exitShell(args);
        }
        else
        {
            executeExternal(args);
        }
    }

public:
    void run()
    {
        std::string input;

        std::cout << "Welcome to Orbit Shell\n";

        while (running)
        {
            std::cout << "orbit:" << std::filesystem::current_path() << " $ " << std::flush;

            if (!std::getline(std::cin, input))
            {
                break;
            }

            auto args = parseCommand(input);
            execute(args);
        }

        std::cout << "Orbit Shell terminated.\n";
    }
};

int main()
{
    OrbitShell shell;
    shell.run();

    return 0;
}
