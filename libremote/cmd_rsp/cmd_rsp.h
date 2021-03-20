#ifndef __CMD_RSP_H__
#define __CMD_RSP_H__

#ifndef CMD_RSP_EXPORT
#ifdef _WIN32
#define CMD_RSP_EXPORT __declspec(dllimport)
#else
#define CMD_RSP_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Prototype:      int int __declspec(dllexport) cmd_rsp(char *command, char **chunk, size_t size)  
//
//  Description:    Executes any command that can be executed in a Windows cmd prompt and returns
//                  the response via auto-resizing buffer. 
///                 Note: this function will hang for executables or processes that run and exit 
///                 without ever writing to stdout.  
///                 The hang occurs during the call to the read() function.
//
//  Inputs:         const char *command - string containing complete command to be sent
//                  char **chunk - initialized pointer to char array to return results
//                  size_t size - Initial memory size in bytes char **chunk was initialized to.
//
//  Return:         0 for success
//                 -1 for failure
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int CMD_RSP_EXPORT cmd_rsp(const char *command, char **chunk, unsigned int chunk_size);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Prototype:      int int __declspec(dllexport) cmd_no_rsp(char *command)  
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
int CMD_RSP_EXPORT cmd_no_rsp(const char *command);


#ifdef __cplusplus
}
#endif

#endif