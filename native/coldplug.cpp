#include <sys/wait.h>

#include <set>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

#include "logging.h"

static const char* modprobe = "/sbin/modprobe";

static bool coldplug_done = false;

static std::set<std::string> resolve_modaliases(const std::set<std::string>& modaliases)
{
    std::vector<const char*> command = {
        modprobe,
        "-a",
        "-q",
        "-R"
    };
    for (const auto& modalias: modaliases) {
        command.push_back(modalias.c_str());
    }
    command.push_back(nullptr);
    // use fork and exec to run command
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        throw std::runtime_error("pipe() failed.");
    }
    auto pid = fork();
    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        throw std::runtime_error("fork() failed.");
    }
    if (pid == 0) {
        // child
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        execvp(command[0], const_cast<char* const*>(command.data()));
        // execvp() failed
        std::cerr << "execvp() failed." << std::endl;
        _exit(1);
    }
    // parent
    close(pipefd[1]);
    std::string output;
    char buffer[4096];
    ssize_t nread;
    while ((nread = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
        output.append(buffer, nread);
    }
    close(pipefd[0]);
    waitpid(pid, nullptr, 0); // ignore status

    std::set<std::string> modules;
    std::istringstream iss(output);
    std::string module_name;
    while (iss >> module_name) {
        if (module_name.empty()) continue;
        modules.insert(module_name);
    }
    return modules;
}

static int load_modules(const std::set<std::string>& modules)
{
    std::vector<const char*> command = {
        modprobe,
        "-a",
        "-b"
    };
    for (const auto& module: modules) {
        command.push_back(module.c_str());
    }
    command.push_back(nullptr);
    // use fork and exec to run command
    auto pid = fork();
    if (pid == -1) {
        throw std::runtime_error("fork() failed.");
    }
    if (pid == 0) {
        // child
        execvp(command[0], const_cast<char* const*>(command.data()));
        // execvp() failed
        std::cerr << "execvp() failed." << std::endl;
        _exit(1);
    }
    // parent
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status)? WEXITSTATUS(status): -1;
}

void coldplug()
{
    if (coldplug_done) {
        logging::info("coldplug() already called.");
        return;
    }
    //else
    // find files exactly named 'modalias' under /sys/devices in any level
    try {
        std::set<std::string> modaliases;
        for (const auto& entry: std::filesystem::recursive_directory_iterator("/sys/devices")) {
            if (entry.path().filename() == "modalias") {
                std::ifstream ifs(entry.path());
                if (!ifs) continue;
                //else
                std::string modalias;
                ifs >> modalias;
                // strip modalias
                modalias.erase(std::remove(modalias.begin(), modalias.end(), '\n'), modalias.end());
                modaliases.insert(modalias);
            }
        }

        auto modules = resolve_modaliases(modaliases);
        std::string msg("Loading modules: ");
        bool first = true;
        for (const auto& module: modules) {
            if (!first) msg += ", ";
            msg += module;
            first = false;
        }
        logging::info(msg);

        auto rst = load_modules(modules);
        if (rst == 0) {
            logging::info("Modules loaded.");
        } else {
            logging::warning("modpeobe returned non-zero exit status: " + std::to_string(rst));
        }

        coldplug_done = true;
        logging::info("coldplug done.");
    }
    catch (const std::exception& e) {
        logging::error("coldplug failed: " + std::string(e.what()));
    }
}
