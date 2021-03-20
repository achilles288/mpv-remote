#include "cmd_rsp.h"

#include <stdio.h>
#include <stdlib.h> // calloc, realloc & free

#ifdef _WIN32
#include <windows.h>

#define BUFSIZE 1000 

typedef struct {
    /* child process's STDIN is the user input or data entered into the child process - READ */
    void* in_pipe_read;
    /* child process's STDIN is the user input or data entered into the child process - WRITE */
    void* in_pipe_write;
    /* child process's STDOUT is the program output or data that child process returns - READ */
    void* out_pipe_read;
    /* child process's STDOUT is the program output or data that child process returns - WRITE */
    void* out_pipe_write;
}IO_PIPES;

// Private prototypes
static int CreateChildProcess(const char* cmd, IO_PIPES* io);
static int CreateChildProcessNoStdOut(const char* cmd, IO_PIPES* io);
static int ReadFromPipe(char** rsp, unsigned int size, IO_PIPES* io);
static char* ReSizeBuffer(char** str, unsigned int size);
static void SetupSecurityAttributes(SECURITY_ATTRIBUTES* saAttr);
static void SetupStartUpInfo(STARTUPINFO* siStartInfo, IO_PIPES* io);
static int SetupChildIoPipes(IO_PIPES* io, SECURITY_ATTRIBUTES* saAttr);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Prototype:      int __declspec(dllexport) cmd_rsp(char *command, char **chunk, size_t chunk_size)  
//
//  Description:    Executes any command that can be executed in a Windows cmd prompt and returns
//                  the response via auto-resizing buffer.
//
//  Inputs:         const char *command - string containing complete command to be sent
//                  char **chunk - initialized pointer to char array to return results
//                  size_t chunk_size - Initial memory size in bytes of char **chunk.
//
//  Return:         0 for success
//                 -1 for failure
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int __declspec(dllexport) cmd_rsp(const char* command, char** chunk, unsigned int chunk_size)
{
    SECURITY_ATTRIBUTES saAttr;

    /// All commands that enter here must contain (and start with) the substring: "cmd.exe /c 
    /// /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// char cmd[] = ("cmd.exe /c \"dir /s\"");  /// KEEP this comment until format used for things like
                                                 /// directory command (i.e. two parts of syntax) is captured
    /// /////////////////////////////////////////////////////////////////////////////////////////////////////////

    const char rqdStr[] = { "cmd.exe /c " };
    int len = (int)strlen(command);
    char* Command = NULL;
    int status = 0;

    Command = calloc(len + sizeof(rqdStr), 1);
    if (!Command) return -1;
    strcat(Command, rqdStr);
    strcat(Command, command);

    SetupSecurityAttributes(&saAttr);

    IO_PIPES io;

    if (SetupChildIoPipes(&io, &saAttr) < 0) return -1;

    //eg: CreateChildProcess("adb");
    if (CreateChildProcess(Command, &io) == 0)
    {
        // Read from pipe that is the standard output for child process. 
        ReadFromPipe(chunk, chunk_size, &io);
        status = 0;
    }
    else
    {
        status = -1;
    }
    free(Command);

    return status;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Prototype:      int __declspec(dllexport) cmd_no_rsp(char *command)  
//
//  Description:    Variation of cmd_rsp that does not wait for a response.  This is useful for
//                  executables or processes that run and exit without ever sending a response to stdout,
//                  causing cmd_rsp to hang during the call to the read() function.
//
//  Inputs:         const char *command - string containing complete command to be sent
//
//  Return:         0 for success
//                 -1 for failure
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int __declspec(dllexport) cmd_no_rsp(const char* command)
{
    /// All commands that enter here must contain (and start with) the substring: "cmd.exe /c 
    /// /////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// char cmd[] = ("cmd.exe /c \"dir /s\"");  /// KEEP this comment until format used for things like
                                                 /// directory command (i.e. two parts of syntax) is captured
    /// /////////////////////////////////////////////////////////////////////////////////////////////////////////

    SECURITY_ATTRIBUTES saAttr;
    const char rqdStr[] = { "cmd.exe /c " };
    int len = (int)strlen(command);
    char* Command = NULL;
    int status = 0;

    Command = calloc(len + sizeof(rqdStr), 1);
    if (!Command) return -1;
    strcat(Command, rqdStr);
    strcat(Command, command);

    SetupSecurityAttributes(&saAttr);

    IO_PIPES io;

    if (SetupChildIoPipes(&io, &saAttr) < 0) return -1;

    status = CreateChildProcessNoStdOut(Command, &io);

    free(Command);

    return status;
}

static int SetupChildIoPipes(IO_PIPES* io, SECURITY_ATTRIBUTES* saAttr)
{
    //child process's STDOUT is the program output or data that child process returns
    // Create a pipe for the child process's STDOUT. 
    if (!CreatePipe(&io->out_pipe_read, &io->out_pipe_write, saAttr, 0))
    {
        return -1;
    }

    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if (!SetHandleInformation(io->out_pipe_read, HANDLE_FLAG_INHERIT, 0))
    {
        return -1;
    }

    //child process's STDIN is the user input or data entered into the child process
    // Create a pipe for the child process's STDIN. 
    if (!CreatePipe(&io->in_pipe_read, &io->in_pipe_write, saAttr, 0))
    {
        return -1;
    }

    // Ensure the write handle to the pipe for STDIN is not inherited. 
    if (!SetHandleInformation(io->in_pipe_write, HANDLE_FLAG_INHERIT, 0))
    {
        return -1;
    }
    return 0;
}


// Create a child process that uses the previously created pipes for STDIN and STDOUT.
static int CreateChildProcess(const char* cmd, IO_PIPES* io)
{
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure. 
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    SetupStartUpInfo(&siStartInfo, io);

    // Create the child process. 
    bSuccess = CreateProcess(NULL,
        cmd,                // command line 
        NULL,               // process security attributes 
        NULL,               // primary thread security attributes 
        TRUE,               // handles are inherited 
        CREATE_NO_WINDOW,   // creation flags            
        //CREATE_NEW_CONSOLE,   // creation flags            
        NULL,               // use parent's environment 
        NULL,               // use parent's current directory 
        &siStartInfo,       // STARTUPINFO pointer 
        &piProcInfo);       // receives PROCESS_INFORMATION 

    // If an error occurs, exit the application. 
    if (!bSuccess)
    {
        return -1;
    }
    else
    {
        // Close handles to the child process and its primary thread.
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(io->out_pipe_write);
    }
    return 0;
}


// Create a child process that uses the previously created pipes for STDIN and STDOUT.
static int CreateChildProcessNoStdOut(const char* cmd, IO_PIPES* io)
{
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;
    BOOL bSuccess = FALSE;

    // Set up members of the PROCESS_INFORMATION structure. 
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    SetupStartUpInfo(&siStartInfo, io);

    // Create the child process. 
    bSuccess = CreateProcess(NULL,
        cmd,                // command line 
        NULL,               // process security attributes 
        NULL,               // primary thread security attributes 
        TRUE,               // handles are inherited 
        CREATE_NO_WINDOW,   // creation flags            
        //CREATE_NEW_CONSOLE,   // creation flags            
        NULL,               // use parent's environment 
        NULL,               // use parent's current directory 
        &siStartInfo,       // STARTUPINFO pointer 
        &piProcInfo);       // receives PROCESS_INFORMATION 

    // If an error occurs, exit the application. 
    if (!bSuccess)
    {
        return -1;
    }
    else
    {
        // Close handles to the child process and its primary thread.
        CloseHandle(piProcInfo.hProcess);
        CloseHandle(piProcInfo.hThread);
        CloseHandle(io->out_pipe_write);
    }
    return 0;
}

// Read output from the child process's pipe for STDOUT
// Grow the buffer as needed
// Stop when there is no more data. 
static int ReadFromPipe(char** rsp, unsigned int size, IO_PIPES* io)
{
    COMMTIMEOUTS ct;
    int size_recv = 0;
    unsigned int total_size = 0;
    unsigned long dwRead;
    BOOL bSuccess = TRUE;
    char* accum;
    char* tmp1 = NULL;
    char* tmp2 = NULL;


    //Set timeouts for stream
    ct.ReadIntervalTimeout = 0;
    ct.ReadTotalTimeoutMultiplier = 0;
    ct.ReadTotalTimeoutConstant = 10;
    ct.WriteTotalTimeoutConstant = 0;
    ct.WriteTotalTimeoutMultiplier = 0;
    SetCommTimeouts(io->out_pipe_read, &ct);


    //This accumulates each read into one buffer, 
    //and copies back into rsp before leaving
    accum = (char*)calloc(1, sizeof(char)); //grow buf as needed
    if (!accum) return -1;
    memset(*rsp, 0, size);

    do
    {
        //Reads stream from child stdout 
        bSuccess = ReadFile(io->out_pipe_read, *rsp, size - 1, &dwRead, NULL);
        if (!bSuccess || dwRead == 0)
        {
            free(accum);
            return 0;//successful - reading is done
        }

        (*rsp)[dwRead] = 0;
        size_recv = (int)strlen(*rsp);


        if (size_recv == 0)
        {
            //should not get here for streaming
            (*rsp)[total_size] = 0;
            return total_size;
        }
        else
        {
            //New Chunk:
            (*rsp)[size_recv] = 0;
            //capture increased byte count
            total_size += size_recv + 1;
            //increase size of accumulator
            tmp1 = ReSizeBuffer(&accum, total_size);
            if (!tmp1)
            {
                free(accum);
                strcpy(*rsp, "");
                return -1;
            }
            accum = tmp1;
            strcat(accum, *rsp);
            if (total_size > (size - 1))
            {   //need to grow buffer
                tmp2 = ReSizeBuffer(&(*rsp), total_size + 1);
                if (!tmp2)
                {
                    free(*rsp);
                    return -1;
                }
                *rsp = tmp2;
            }
            strcpy(*rsp, accum);//refresh rsp
        }


    } while (1);
}

// return '*str' after number of bytes realloc'ed to 'size'
static char* ReSizeBuffer(char** str, unsigned int size)
{
    char* tmp = NULL;

    if (!(*str)) return NULL;

    if (size == 0)
    {
        free(*str);
        return NULL;
    }

    tmp = (char*)realloc((char*)(*str), size);
    if (!tmp)
    {
        free(*str);
        return NULL;
    }
    *str = tmp;

    return *str;
}

static void SetupSecurityAttributes(SECURITY_ATTRIBUTES* saAttr)
{
    // Set the bInheritHandle flag so pipe handles are inherited.
    saAttr->nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr->bInheritHandle = TRUE;
    saAttr->lpSecurityDescriptor = NULL;
}

static void SetupStartUpInfo(STARTUPINFO* siStartInfo, IO_PIPES* io)
{
    siStartInfo->cb = sizeof(STARTUPINFO);
    siStartInfo->hStdError = io->out_pipe_write;
    siStartInfo->hStdOutput = io->out_pipe_write;
    siStartInfo->hStdInput = io->in_pipe_read;
    siStartInfo->dwFlags |= STARTF_USESTDHANDLES;
}




#else

int cmd_rsp(const char* command, char** chunk, unsigned int chunk_size) {
    FILE *fp = popen(command, "r");
    if(fp == NULL)
        return -1;
    fgets(*chunk, chunk_size, fp);
    fclose(fp);
    return 0;
}

int cmd_no_rsp(const char* command) {
    FILE* fp = popen(command, "r");
    if(fp == NULL)
        return -1;
    fclose(fp);
    return 0;
}

#endif