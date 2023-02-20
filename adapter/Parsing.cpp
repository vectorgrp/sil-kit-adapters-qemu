// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "Parsing.hpp"
#include <iostream>
#include <algorithm>

void adapters::print_help(bool userRequested)
{
    std::cout
        << "Usage (defaults in curly braces if you omit the switch):" << std::endl
        << "SilKitAdapterQemuChardev [--name <participant's name{ChardevAdapter}>]\n"
           "  [--registry-uri silkit://<host{localhost}>:<port{8501}>]\n"
           "  [--log <Trace|Debug|Warn|{Info}|Error|Critical|off>]\n"
           //left for future unification. Print that both are "optional" but add a line about at least of them to be present.
           //"   --socket-to-ethernet <host>:<port>,network=<network's name>[~<controller's name>]\n"
           " [[--socket-to-chardev\n"
           "     <host>:<port>,\n"
           "    [<namespace>::]<inbound topic name>[~<subscriber's name>]\n"
           "       [[,<label key>:<optional label value>\n"
           "        |,<label key>=<mandatory label value>\n"
           "       ]],\n"
           "    [<namespace>::]<outbound topic name>[~<publisher's name>]\n"
           "       [[,<label key>:<optional label value>\n"
           "        |,<label key>=<mandatory label value>\n"
           "       ]]\n"
           " ]]\n"
           "\n"
           "There needs to be at least one --socket-to-chardev. Each socket must be unique.\n";
    std::cout << "\n"
                 "Example:\n"
                 "SilKitAdapterQemuChardev --name ChardevAdapter2 "
                 "--socket-to-chardev localhost:12345,"
                 "Namespace::inboundQemu,VirtualNetwork=Default,"
                 "outboundQemu,Namespace:Namespace,VirtualNetwork:Default\n";
    if (!userRequested)
        std::cout << "\n"
                     "Pass --help to get this message.\n";
};

char** adapters::findArg(int argc, char** argv, const std::string& argument, char** args)
{
    auto found = std::find_if(args, argv + argc, [argument](const char* arg) -> bool {
        return argument == arg;
    });
    if (found < argv + argc)
    {
        return found;
    }
    return NULL;
};
char** adapters::findArgOf(int argc, char** argv, const std::string& argument, char** args)
{
    auto found = findArg(argc, argv, argument, args);
    if (found != NULL && found + 1 < argv + argc)
    {
        return found + 1;
    }
    return NULL;
};
std::string adapters::getArgDefault(int argc, char** argv, const std::string& argument, const std::string& defaultValue)
{
    auto found = findArgOf(argc, argv, argument, argv);
    if (found != NULL)
        return *(found);
    else
        return defaultValue;
};
