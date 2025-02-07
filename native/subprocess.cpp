#include <unistd.h>
#include <sys/wait.h>

#include "subprocess.h"
#include "logging.h"
#include "formatter.h"

int run_subprocess(std::vector<std::string> cmdline)
{
    logging::debug(std::format("Running command: {}", cmdline));
    auto pid = fork();
    if (pid == -1) {
        return -1;
    }
    if (pid == 0) {
        std::vector<char*> argv;
        for (const auto& arg: cmdline) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        execvp(argv[0], argv.data());
        _exit(1);
    }
    //else
    int status;
    waitpid(pid, &status, 0);
    auto rst = WIFEXITED(status)? WEXITSTATUS(status) : -1;
    logging::debug(std::format("Command exited with status: {}", rst));
    return rst;
}