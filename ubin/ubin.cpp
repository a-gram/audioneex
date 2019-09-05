/*
   Copyright (c) 2014, Audioneex.com.
   Copyright (c) 2014, Alberto Gramaglia.
	
   This source code is part of the Audioneex software package and is
   subject to the terms and conditions stated in the accompanying license.
   Please refer to the license document provided with the package
   for more information.
	
*/

#include <iostream>
#include <vector>
#include <algorithm>

#include "ubin.h"


void PrintUsage(){
    std::cout << "\nUSAGE: ubin <command> [command_options]\n"<<std::endl;
}


int main(int argc, char** argv)
{
    try
    {
        CmdLineParser cmdLine;
        cmdLine.Parse(argv, argc);
        Command* cmd = cmdLine.GetCommand();
        cmd->Execute();

        std::cout << "Done" << std::endl;
    }
    // Command line exceptions
    catch(const bad_cmd_line_exception &ex){
        std::cout << "ERROR: " << ex.what() << std::endl;
        PrintUsage();
        return -1;
    }
    // Command-specific exceptions
    catch(const bad_cmd_exception &ex){
        std::cout << "ERROR: " << ex.what() << std::endl;
        return -1;
    }
    // Any other exception
    catch(const std::exception &ex){
        std::cout << "ERROR: " << ex.what() << std::endl;
        return -1;
    }

    return 0;
}

