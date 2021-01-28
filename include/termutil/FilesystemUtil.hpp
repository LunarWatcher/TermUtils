#pragma once

#include <regex>
#include <string>
// required for getenv
#include <cstdlib>
#include <iostream>

// Windows is the easiest one to detect.
// Basically, this first clause applies for everything that isn't Windows,
// which should be able to use these.
#if !defined(_WIN32) && !defined(_WIN64)
#include <pwd.h>
// Used for getuid
#include <sys/types.h>
#include <unistd.h>
#endif

#include "CompatUtil.hpp"

namespace termutil {

namespace Filesystem {

/**
 * Detect separators.
 * This is a function to enable \\ and /
 */
inline bool isSeparator(const char x) {
    return x == '/' || x == '\\';
}

/**
 * Joins paths.
 */
inline std::string joinPath(const std::string& a, const std::string& b) {
    if (isSeparator(a.back()) || isSeparator(b.front()))
        return a + b;
    return a + "/" + b;
}

/**
 * Expands a user path (AKA a path starting with ~), independently of the OS.
 * This requires several OS-specific calls (specifically between Windows and UNIX, from what I can tell.
 * There's not much of a difference in this area between for an instance Linux and Mac. Special
 * implementation requirements will be taken when we get to a point where it's needed)
 */
inline std::string expandUserPath(const std::string& inputPath) {

    // Convert all backslashes to forward slashes for processing (fuck you Windows)
    std::string rawPath = std::regex_replace(inputPath, std::regex("\\\\"), "/");

    // In order to universally support paths without doing a hacky if-check elsewhere,
    // this just returns the path itself if it isn't a home path.
    // Other paths should work themselves out
    if (rawPath.at(0) != '~')
        return rawPath;

    StdOptional<std::string> username;
    std::string remainingPath;

    // The next part of the code is used to parse off the username,
    // and grab the rest of the path. This is necessary to reconstruct
    // the path afterwards.
    std::string mod = rawPath.substr(1);
    std::vector<std::string> pathSplit; // = StrUtil::splitString(mod, "/", 1);

    // splitString but smaller so I don't have to import everything
    size_t pos = 0;
    std::string token, cache;
    pos = mod.find("/");
    token = mod.substr(0, pos);
    cache = mod;
    cache.erase(0, pos + 1);
    pathSplit.push_back(token);
    pathSplit.push_back(cache);

    //  Parse off the username
    if (rawPath.length() >= 2 && !isSeparator(rawPath.at(1))) {
        // ~username
        // ~username/path
        username = pathSplit.at(0);
        remainingPath = pathSplit.at(1);
    } else if (rawPath.find('/') != std::string::npos) {
        // The path contains a slash.
        if (pathSplit.size() == 1) {
            throw std::runtime_error("There is absolutely no reason for this to trigger, at all. If it is, hit Olivia "
                                     "with a rolled up newspaper c:");
        }

        remainingPath = pathSplit.at(1);
    }

    // We've by now found a username or not. This means we can actually expand
    // the path.

    std::string homePath = "";
#if defined(_WIN32) || defined(_WIN64)

    if (!username.has_value()) {
        auto userProfile = EnvVariable::getEnv("USERPROFILE");

        if (userProfile == "") {
            auto homeDrive = EnvVariable::getEnv("HOMEDRIVE");
            if (homeDrive == "")
                homeDrive = "";

            auto envHomePath = EnvVariable::getEnv("HOMEPATH");

            if (envHomePath == "") {
                ColorPrinter printer;
                printer << ANSIFeature::FOREGROUND << 9
                        << "Unable to find %HOMEPATH%. Specify the path explicitly instead." << ANSIFeature::CLEAR
                        << "\n";
                return "";
            }
            homePath = homeDrive + envHomePath;
        } else
            homePath = userProfile;

    } else {
        std::cerr << "This doesn't work."
                  << " Due to Windows having a very limited API for expanding user paths, and it relies on environment "
                  << "variables and assumptions, me (the developer), has decided to not implement ~user expansion on "
                     "Windows. "
                  << "I cannot easily test it, nor can I find any reassuring information for a universal pattern I can "
                     "use. "
                  << "Replace your path with an absolute path instead. An implementation for this feature may be "
                     "available in the future.\n";
        return "";
    }
    // Force forward slashes
    homePath = std::regex_replace(homePath, std::regex("\\\\"), "/");

#else
    /*
     The unixes are easier, because they should in theory share a single
     API that makes this a whole lot easier.
     The idea is checking for HOME if we're looking for the current user.
     If we can't find the home variable, fall back to a UNIX-specific^1
     function that retrieves the path, along with a few other details.
     see getpwnam(3) for more info on the functions, and passwd(5)
     for details on the struct. This code drops the HOME variable for
     various reasons.
     If a username has been supplied (~username/), getpwnam is used instead
     of getpwuid. This returns the same type of struct as getpwuid().
     The bonus with the UNIX approach is that it requires the user to exist,
     where as Windows for non-existent users with a custom specified path should
     in theory cause a bad path. This is on the user, however.
     ^1: untested on mac and other UNIX-based systems. Only verified on Linux.
    */
    struct passwd* passwdPtr = nullptr;

    if (!username.has_value()) {
        // While the home environment variable can be used,
        // getenv() is considered insecure
        // secure_getenv is only available on GNU/Linux, and not Mac.
        // Mac is still compatible with the rest of the code,
        // so no environment variables are used
        passwdPtr = getpwuid(getuid());
    } else {
        auto& name = *username;
        passwdPtr = getpwnam(name.c_str());
    }

    if (passwdPtr == nullptr && homePath == "") {
        std::cerr << "Failed to expand the user path for " << rawPath
                << ". The system seems to think you don't exist. "
                << "Please specify the path to use - don't abbreviate it with ~.\n";
        return "";
    } else if (homePath == "")
        homePath = passwdPtr->pw_dir;
#endif
    return joinPath(homePath, remainingPath);
}

} // namespace Filesystem

} // namespace termutil
