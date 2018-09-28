/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef PROCESSCONNECTOR_H
#define PROCESSCONNECTOR_H

#ifdef WIN32
 #include <Windows.h>
 #include <codecvt>
 #include <locale>
#else
#endif

#include <string>


enum PipeType{
     INPUT_PIPE,
     OUTPUT_PIPE
};


#ifndef WIN32 // POSIX compliant systems

class PosixPipe
{
    FILE* m_Pipe;
    std::string m_ProgramPath;
    std::string m_ArgsList;

 public:

    PosixPipe() : m_Pipe() {}
    ~PosixPipe() { Close(); }

    bool Open(PipeType type) 
	{
        return Open( m_ProgramPath + m_ArgsList, type );
    }

    bool Open(const std::string &cmd, PipeType type) 
	{
        if(IsOpen()) Close();
        m_Pipe = popen(cmd.c_str(), type==INPUT_PIPE ? "r" : "w");
        return m_Pipe!=nullptr;
    }

    int Close()
	{
        int ret = 0;
        if(m_Pipe){
           ret = pclose(m_Pipe);
           m_Pipe = nullptr;
        }
        m_ProgramPath.clear();
        m_ArgsList.clear();
        return ret;
    }

    bool Read(void* buffer, size_t nbytes, size_t &read)
	{
        read = fread(buffer, 1, nbytes, m_Pipe);
        return ferror(m_Pipe) ? false : true;
    }

    std::string GetError() { return std::string(strerror(errno)); }

    bool IsOpen() const { return m_Pipe!=nullptr; }

    void SetProgramPath(const std::string& progPath)
	{
        if(progPath.empty()) return;
        if(progPath.find(' ')!=std::string::npos &&
           progPath.front()!='"' && progPath.back()!='"')
           m_ProgramPath = ( "\"" + progPath + "\"" );
        else
           m_ProgramPath = ( progPath );
    }

    void AddCmdArg(const std::string& arg)
	{
        if(!arg.empty())
           m_ArgsList += " " + arg;
    }
};

#else // Windows

typedef std::wstring_convert< std::codecvt_utf8<wchar_t> > string_converter;

#ifdef UNICODE
inline std::wstring TO_STRING_T(const std::string& s)  { return string_converter().from_bytes(s); }
inline std::string  TO_STRING_T(const std::wstring &s) { return string_converter().to_bytes(s); }
#else
inline std::string TO_STRING_T(const std::string& s) { return s; }
#endif

#ifdef UNICODE
template<typename STRING_T=std::wstring>
#else
template<typename STRING_T=std::string>
#endif
class WindowsPipe
{
    enum{
        READ_ENDPOINT,
        WRITE_ENDPOINT
    };

    HANDLE m_InChannel[2];  // Our prog <--- Cmd stdout
    HANDLE m_OutChannel[2]; // Our prog ---> Cmd stdin
    HANDLE m_ErrChannel[2]; // Our prog <--> Cmd stderr

    PROCESS_INFORMATION m_ProcInfo;

    bool m_IsOpen;
    bool m_UseErrorChannel;
    STRING_T m_ProgramPath;
    STRING_T m_ArgsList;

    /// Create a new process to execute the program specified in cmd
    BOOL CreateChildProcess(const STRING_T &cmd)
    {
        DWORD dwCreationFlags = 0;
#ifdef UNICODE
        dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
#endif
        dwCreationFlags |= CREATE_NO_WINDOW;

        STARTUPINFO siStartInfo;
        BOOL bSuccess = FALSE;

        ::ZeroMemory( &m_ProcInfo, sizeof(PROCESS_INFORMATION) );
        ::ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );

        HANDLE stdin_h  = m_OutChannel[READ_ENDPOINT] != INVALID_HANDLE_VALUE ?
                          m_OutChannel[READ_ENDPOINT] :
                          GetStdHandle(STD_INPUT_HANDLE);

        HANDLE stdout_h = m_InChannel[WRITE_ENDPOINT] != INVALID_HANDLE_VALUE ?
                          m_InChannel[WRITE_ENDPOINT] :
                          GetStdHandle(STD_OUTPUT_HANDLE);

        HANDLE stderr_h = m_UseErrorChannel ?
                         (m_InChannel[WRITE_ENDPOINT] != INVALID_HANDLE_VALUE ?
                          m_ErrChannel[WRITE_ENDPOINT] :
                         (m_OutChannel[READ_ENDPOINT] != INVALID_HANDLE_VALUE ?
                          m_ErrChannel[READ_ENDPOINT] : GetStdHandle(STD_ERROR_HANDLE))) :
                          GetStdHandle(STD_ERROR_HANDLE);

        siStartInfo.cb = sizeof(STARTUPINFO);
        siStartInfo.hStdInput  = stdin_h;
        siStartInfo.hStdOutput = stdout_h;
        siStartInfo.hStdError =  stderr_h;
        siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

        bSuccess = ::CreateProcess(0,
           (LPTSTR)cmd.c_str(),   // command line
           0,          // process security attributes
           0,          // primary thread security attributes
           TRUE,          // handles are inherited
           dwCreationFlags,  // creation flags
           0,          // use parent's environment
           0,          // use parent's current directory
           &siStartInfo,  // STARTUPINFO pointer
           &m_ProcInfo);  // receives PROCESS_INFORMATION

        if (!bSuccess )
           return FALSE;
        else
        {
           // We no longer need the write handles of the pipe
           // since they've been inherited by the child process.
           if(m_InChannel[WRITE_ENDPOINT] != INVALID_HANDLE_VALUE){
              ::CloseHandle( m_InChannel[WRITE_ENDPOINT] );
              m_InChannel[WRITE_ENDPOINT] = INVALID_HANDLE_VALUE;
           }

           if(m_ErrChannel[WRITE_ENDPOINT] != INVALID_HANDLE_VALUE){
              ::CloseHandle( m_ErrChannel[WRITE_ENDPOINT] );
              m_ErrChannel[WRITE_ENDPOINT] = INVALID_HANDLE_VALUE;
           }
        }
        // Take a short nap so the program can start now
        Sleep(10);
        return bSuccess;
    }

    /// This nice function will give you a human-readable error message.
    STRING_T GiveMeAHumanReadableErrorPls()
    {
        LPVOID lpMsgBuf;
        DWORD dw = ::GetLastError();

        ::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            0,
            dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, 0 );

        size_t msgSize = ::lstrlen((LPTSTR)lpMsgBuf);

        STRING_T estr(reinterpret_cast<PTCHAR>(lpMsgBuf), msgSize);
        ::LocalFree(lpMsgBuf);
        return estr;
    }

    /// Set up all the stuffs to communicate with the process
    bool ConnectToProcess(const STRING_T& cmd, PipeType type)
    {
        HANDLE htmp;

        SECURITY_ATTRIBUTES saAttr;
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = 0;

        // Create the read pipe. The write handle must be inheritable
        // by the child process in order to be able to use it. The read
        // handle must not since it's exclusively used on this side.
        if(type == INPUT_PIPE){

            if(!::CreatePipe(&htmp, &m_InChannel[WRITE_ENDPOINT], &saAttr, 1<<20))
                return false;

            if(!::DuplicateHandle(::GetCurrentProcess(), htmp,
                                  ::GetCurrentProcess(), &m_InChannel[READ_ENDPOINT],
                                  0, FALSE, DUPLICATE_SAME_ACCESS))
                return false;

            ::CloseHandle(htmp);

            if(m_UseErrorChannel)
            {
               if(!::CreatePipe(&htmp, &m_ErrChannel[WRITE_ENDPOINT], &saAttr, 1<<20))
                  return false;

               if(!::DuplicateHandle(::GetCurrentProcess(), htmp,
                                     ::GetCurrentProcess(), &m_ErrChannel[READ_ENDPOINT],
                                     0, FALSE, DUPLICATE_SAME_ACCESS))
                  return false;

               ::CloseHandle(htmp);
            }
        }
        // Create the write pipe. (trivially implemented. But i am too lazy now...)
        else{

        }

        if(!CreateChildProcess( cmd ))
           return false;
	   
        // Connected
        return true;
    }

 public:

    WindowsPipe() :
        m_IsOpen          (false),
        m_UseErrorChannel (false)
    {
        m_InChannel[READ_ENDPOINT] = INVALID_HANDLE_VALUE;
        m_InChannel[WRITE_ENDPOINT] = INVALID_HANDLE_VALUE;
        m_OutChannel[READ_ENDPOINT] = INVALID_HANDLE_VALUE;
        m_OutChannel[WRITE_ENDPOINT] = INVALID_HANDLE_VALUE;
        m_ErrChannel[READ_ENDPOINT] = INVALID_HANDLE_VALUE;
        m_ErrChannel[WRITE_ENDPOINT] = INVALID_HANDLE_VALUE;

        ::ZeroMemory( &m_ProcInfo, sizeof(PROCESS_INFORMATION) );
    }

    ~WindowsPipe() { if(m_IsOpen) Close(); }

    /// Open an input/output pipe. The program and all its arguments must be
    /// specified upfront using the below methods.
    bool Open(PipeType type) {

        if(m_IsOpen) Close();
        m_IsOpen = ConnectToProcess( m_ProgramPath + TEXT(" ") + m_ArgsList, type );
        return m_IsOpen;
    }

    /// This method does ... take a wild guess ...
    bool Open(const std::string &cmd, PipeType type) {

        if(m_IsOpen) Close();
        m_IsOpen = ConnectToProcess( TO_STRING_T(cmd), type );
        return m_IsOpen;
    }

    /// Yup, that's correct. This does what you think it does.
    int Close(){

        int ret = 0;
        DWORD exitCode = 0;

        /// Check that the process id dead.
        if(::GetExitCodeProcess(m_ProcInfo.hProcess, &exitCode)){
           if(exitCode != STILL_ACTIVE)
              ret = exitCode;
           else{
              // The process apparently is still alive. Check for signs of life.
              if(::WaitForSingleObject(m_ProcInfo.hProcess, 50) == WAIT_OBJECT_0)
                 // The bastard is still alive. Brutally Kill it.
                 ::TerminateProcess(m_ProcInfo.hProcess, 0);
              ret = 0;
           }
        }

        ::CloseHandle( m_ProcInfo.hProcess );
        ::CloseHandle( m_ProcInfo.hThread );

        if(m_InChannel[READ_ENDPOINT] != INVALID_HANDLE_VALUE)
            ::CloseHandle( m_InChannel[READ_ENDPOINT] );

        if(m_InChannel[WRITE_ENDPOINT] != INVALID_HANDLE_VALUE)
            ::CloseHandle( m_InChannel[WRITE_ENDPOINT] );

        if(m_OutChannel[READ_ENDPOINT] != INVALID_HANDLE_VALUE)
            ::CloseHandle( m_OutChannel[READ_ENDPOINT] );

        if(m_OutChannel[WRITE_ENDPOINT] != INVALID_HANDLE_VALUE)
            ::CloseHandle( m_OutChannel[WRITE_ENDPOINT] );

        m_OutChannel[READ_ENDPOINT] = INVALID_HANDLE_VALUE;
        m_OutChannel[WRITE_ENDPOINT] = INVALID_HANDLE_VALUE;
        m_InChannel[READ_ENDPOINT] = INVALID_HANDLE_VALUE;
        m_InChannel[WRITE_ENDPOINT] = INVALID_HANDLE_VALUE;

        ::ZeroMemory( &m_ProcInfo, sizeof(PROCESS_INFORMATION) );

        m_ProgramPath.clear();
        m_ArgsList.clear();

        m_UseErrorChannel = false;
        m_IsOpen = false;
        return ret;
    }

    /// In its best days this will give you an error message that everyone can understand
    std::string GetError() { return TO_STRING_T(GiveMeAHumanReadableErrorPls()); }

    /// We have an input pipe ... we need to read something ...
    bool Read(void* buffer, size_t nbytes, size_t &read){

        DWORD bytesRead = 0;
        size_t missing = nbytes;
        read = 0;
        char* pbuf = (char*)buffer;
        // ReadFile() may return less data than requested even if there
        // is enough in the pipe line, so keep reading until the requested
        // amount is reached or all data is exhausted.
        while(missing){
            if(!::ReadFile( m_InChannel[READ_ENDPOINT],
                            pbuf, (DWORD)missing, &bytesRead, 0)){
                DWORD bytesAvail = 0;
                ::PeekNamedPipe(m_InChannel[READ_ENDPOINT], 0, 0, 0, &bytesAvail, 0);
                if(bytesAvail > 0)
                   return false;
                else break;
            }
            missing-=bytesRead;
            pbuf+=bytesRead;
        }
        read = nbytes - missing;
        return true;
    }

    /// Should you need to read the error channel (a fancy name for stderr)
    std::string ReadErr() {

        std::string estr;
        char buf[2048];
        DWORD bytesAvail = 0;
        DWORD bytesRead = 0;

        do{
            if(!::ReadFile( m_ErrChannel[READ_ENDPOINT],
                            buf, (DWORD)sizeof(buf), &bytesRead, 0)){
                ::PeekNamedPipe(m_ErrChannel[READ_ENDPOINT], 0, 0, 0, &bytesAvail, 0);
                if(bytesAvail > 0)
                   break;
            }
            estr.append(buf, bytesRead);

        }while(bytesRead);
        return estr;
    }

    bool IsOpen() const  { return m_IsOpen; }

    void SetProgramPath(const std::string& progPath){

        if(progPath.empty()) return;
        if(progPath.find(' ')!=std::string::npos &&
           progPath.front()!='"' && progPath.back()!='"')
           m_ProgramPath = TO_STRING_T( "\"" + progPath + "\"" );
        else
           m_ProgramPath = TO_STRING_T( progPath );
    }

    void AddCmdArg(const std::string& arg){

        if(!arg.empty())
           m_ArgsList += TO_STRING_T( " " + arg );
    }

    void SetUseErrorChannel(bool value) { m_UseErrorChannel = value; }

};
#endif

#endif // PROCESSCONNECTOR_H

