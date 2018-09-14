/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef COMMAND_H
#define COMMAND_H

#include <vector>
#include <string>
#include <set>
#include <boost/lexical_cast.hpp>


/// Custom exception for illformed cmd lines
class bad_cmd_line_exception : public std::logic_error {
 public: explicit bad_cmd_line_exception(const std::string& msg) :
                  std::logic_error(msg) {}
};

/// Command-specific exception
class bad_cmd_exception : public std::logic_error {
 public: explicit bad_cmd_exception(const std::string& msg) :
                  std::logic_error(msg) {}
};

/// Abstract Command.
class Command
{
protected:
    std::set<std::string>    m_SupportedArgs;
    std::vector<std::string> m_Args;
    std::string              m_Usage;

    /// Get the value for the given argument. Arguments are supposed
    /// to be in the form <arg_id> <arg_value> (i.e. -x value)
    /// Return false if the argument was not found (throwing is not
    /// a good idea as arguments may be optional, so let it to the command
    /// to decide). Throws if the cmmand is ill formed (i.e. no value found).
    template<typename T>
    bool GetArgValue(const std::string& arg, T& val)
    {
        std::vector<std::string>::const_iterator it =
        std::find(m_Args.begin(), m_Args.end(), arg);
        if(it != m_Args.end()){
           if(++it != m_Args.end())
              val = boost::lexical_cast<T>(*it);
           else
              PrintUsageAndThrow("Invalid command line");
           return true;
        }
        return false;
    }

    /// Check whether the given argument was specified
    bool ArgExists(const std::string& arg)
    {
        return std::find(m_Args.begin(), m_Args.end(), arg) != m_Args.end();
    }

    /// Validate the arguments against the list of supported arguments
    /// for this command
    bool ValidArgs()
    {
        std::vector<std::string>::const_iterator it = m_Args.begin();
        for(; it!=m_Args.end(); ++it)
            if((*it).substr(0,1) == "-" &&
               m_SupportedArgs.find(*it) == m_SupportedArgs.end())
               return false;
        return true;
    }

    void PrintUsageAndThrow(const std::string& emsg)
	{
        throw bad_cmd_exception(emsg + m_Usage);
    }

public:
    void SetArgs(const std::vector<std::string>& args) { m_Args = args; }
    virtual void Execute() = 0;
    virtual ~Command(){}
};

/// A dummy command
class NullCommand : public Command{
public: void Execute() {}
};


#endif
