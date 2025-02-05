#ifndef TECHNICAL_H
#define TECHNICAL_H

#ifndef NULL
#define NULL \0
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif
// MAX_PATH including NULL character
#define MAX_PATH_INC MAX_PATH + 1

// this relates to tec_process
// stderr should use same size
#define TEC_STDOUT_BUFFER_SIZE 1024

// most significant digit in 1 byte char
#define TEC_MOST_SIG_BIT 128
#define TEC_2ND_MOST_SIG_BIT 64
#define TEC_3RD_MOST_SIG_BIT 32
#define TEC_4TH_MOST_SIG_BIT 16

//this allows for colours when using printf
//e.g. printf("%sError:%s This went wrong!\n", TEC_PRINT_RED, TEC_PRINT_NORMAL);
//only on Linux, no effect on windows
#ifdef __linux
	#define TEC_PRINT_NORMAL "\x1B[0m"
	#define TEC_PRINT_RED "\x1B[1;31m"
	#define TEC_PRINT_GREEN "\x1B[1;32m"
	#define TEC_PRINT_YELLOW "\x1B[1;33m"
	#define TEC_PRINT_BLUE "\x1B[1;34m"
	#define TEC_PRINT_PURPLE "\x1B[1;35m"
	#define TEC_PRINT_CYAN "\x1B[1;36m"
	#define TEC_PRINT_WHITE "\x1B[1;37m"

	#define TEC_PRINT_NORMAL_BOLD "\x1B[0m"
	#define TEC_PRINT_RED_DARK "\x1B[0;31m"
	#define TEC_PRINT_GREEN_DARK "\x1B[0;32m"
	#define TEC_PRINT_YELLOW_DARK "\x1B[0;33m"
	#define TEC_PRINT_BLUE_DARK "\x1B[0;34m"
	#define TEC_PRINT_PURPLE_DARK "\x1B[0;35m"
	#define TEC_PRINT_CYAN_DARK "\x1B[0;36m"
	#define TEC_PRINT_WHITE_DARK "\x1B[0;37m"
#else
	#define TEC_PRINT_NORMAL ""
	#define TEC_PRINT_RED ""
	#define TEC_PRINT_GREEN ""
	#define TEC_PRINT_YELLOW ""
	#define TEC_PRINT_BLUE ""
	#define TEC_PRINT_PURPLE ""
	#define TEC_PRINT_CYAN ""
	#define TEC_PRINT_WHITE ""

	#define TEC_PRINT_NORMAL_DARK ""
	#define TEC_PRINT_RED_DARK ""
	#define TEC_PRINT_GREEN_DARK ""
	#define TEC_PRINT_YELLOW_DARK ""
	#define TEC_PRINT_BLUE_DARK ""
	#define TEC_PRINT_PURPLE_DARK ""
	#define TEC_PRINT_CYAN_DARK ""
	#define TEC_PRINT_WHITE_DARK ""
#endif

#ifdef __linux
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <pthread.h>
	#include <sys/uio.h>
	#include <fcntl.h>
#endif
#ifdef __WIN32

	#include <windows.h>
	#include <shlwapi.h>

#endif
#include <unistd.h>

typedef struct _tec_process tec_process;

struct _tec_process{

	char buf_stdout[TEC_STDOUT_BUFFER_SIZE];
	char buf_stderr[TEC_STDOUT_BUFFER_SIZE];

	//stdout callback for process
	void (*tp_output_cb)(void *tp, void *dp);
	void *dp;	//data_pointer, argument 
	//stderr callback for process
	void (*tp_output_error_cb)(void *tp, void *dp);
	void *dp_error;

#ifdef __linux

#define PARENT_READ_PIPE 0
#define PARENT_WRITE_PIPE 1
#define PARENT_READ_ERROR_PIPE 2
#define READ_FD  0
#define WRITE_FD 1

	pid_t child;

	int pipes[3][2];

#endif

#ifdef __WIN32

	HANDLE child_std_in_read;
	HANDLE child_std_in_write;
	HANDLE child_std_out_read;
	HANDLE child_std_out_write;
	HANDLE child_std_err_read;
	HANDLE child_std_err_write;

	HANDLE process_info;

#endif

};

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
//for tec_error
#include "stdarg.h"
//

/******************************************************************************
	string related functions
******************************************************************************/

/*tec_string_equal:
	compares two strings
	returns 1 if they are equal
	returns 0 otherwise
	will also return 1 if they are both empty strings (either because they are NULL pointers or there is no data in the string)
	this function is more intuitive than strcmp() as this latter function returns 0 when strings are equal
*/
int			tec_string_equal(char *str1, char *str2);

/*tec_string_length:
	returns the length of a string in bytes
	check tec_string_utf8_length() to know the number of codepoints
	unlike strlen, this function does not segfault when you give it a NULL pointer
	instead it returns zero because, well, you gave it an empty string ;-)
*/
int			tec_string_length(char *str);

/*tec_string_copy:
tec_string_copy_with_ellipsis:
	safer alternative to strcpy or even strncpy
	int size is the size of the destination
	these functions guarantee that at most size - 1 bytes will be written to destination plus a NULL character
	this prevents buffer overflows
	this guarantees you get a NULL terminated string in destination (unlike strncpy)
	this function will not cut off the copying in the middle of a UTF8 codepoint when out of space
	ellipsis is those "..." added at end of string when space is insufficient
	for best performance, use tec_string_copy
	returns 1 if all was copied right
	returns 2 if source could not be fully copied
	returns 0 in case of error
	naturally, to use ellipsis, destination needs to be 4 bytes or longer

	these functions assume char *source is a correctly formatted UTF8 string
*/
int			tec_string_copy(char *destination, char *source, int size);
int			tec_string_copy_with_ellipsis(char *destination, char *source, int size);

/*tec_string_concatenate:
tec_string_cat:
	concatenates two strings
	will guarantee a NULL pointer at end of destination even if source is too large to fit
	this function will not cut off the copying in the middle of a UTF8 codepoint when out of space
	returns 0 in case of error
	returns 1 if everything went right
	returns 2 if source was too large
*/
int			tec_string_concatenate(char *destination, char *source, int size);
#define tec_string_cat(X, Y, Z) tec_string_concatenate(X, Y, Z)

/*tec_string_split:

	splits a string where char c does occur
	num will give you the number of times char c did occur in the string
	num is also index of last string in returned char**
	CAUTION: there is one more string than occurences of char c, this might include empty strings

	the string str will be modified so make sure to make a copy if needed

	note that some character pointers may be NULL pointers
	if 2 or more characters c are right next to each other

	free the returned char ** when done

	returns NULL in case of error
*/
char**		tec_string_split(char *str, char c, int *num);

/*tec_string_split_for_args:

	this does _almost_ the same thing as tec_string_split(str, ' ', &num);

	It will split the string like arguments on a command line:
		> cmd arg1 arg2 arg3

	The difference is that it will not split the string at the space when inside quotes
		> cmd arg1 "arg 2" arg3

*/
char**		tec_string_split_for_args(char *str, int *num);

/*tec_string_find_char:
	returns index of first instance of a character in a string
	returns -1 if not found
*/
int			tec_string_find_char(char *s, char to_find);

/*tec_string_find_str:
	look for an instance of to_find in string
	returns index in string where to_find was found
	returns -1 when nothing was found
*/
int			tec_string_find_str(char *string, char *to_find);

/*tec_string_is_plain_ascii:
	this function tests whether the string contains only old ascii (7 bit) characters
	if so, returns 1
	if it contains utf-8 or extended ascii, it returns zero
	all other cases, returns zero
	this function also checks if there are non-printable ascii characters (smaller than 32)
	if so, returns 0
	this function also checks if string contains ascii(127), if so returns 0
*/
int			tec_string_is_plain_ascii(char *text);

/*tec_string_shift:
	removes an ascii char or unicode codepoint at front of string
	assumes a valid utf8 string
*/
void		tec_string_shift(char *str);

/*tec_string_utf8:
	encodes a unicode codepoint into a utf8 encoded string
	uint32_t codepoint: the codepoint to be encoded into utf8
	char *str: the string where the encoded codepoint will be written to
	you need to ensure that str has at least 5 bytes
	4 bytes is the max length of a utf8 encoding plus NULL character
	returns number of bytes written (excluding NULL character)
*/
int			tec_string_utf8(uint32_t codepoint, char *str);

/*tec_string_to_int:
	converts a string to an integer
	string may not contain anything in front of number except '-' or '+'
	does not safeguard against overflow
*/
int			tec_string_to_int(char *s);

/*tec_string_hex_to_int:
	takes a string containing a hexadecimal number and converts it to a uint32
	string may have leading white space
	string may or may not start with "0x"
	this function will handle up to 0xffffffff or 4,294,967,295
*/
uint32_t	tec_string_hex_to_int(char *str);

/*tec_string_from_int:
	takes an integer and converts it to a string
	writes the result into char* buffer
	returns the number of bytes written (including NULL character)
	returns -1 upon failure
*/
int			tec_string_from_int(int32_t i, char *buffer, int32_t buffer_size);

/*tec_string_to_double:
	ensure char *string points to a floating point number
	white space shall be skipped
	returns a double
*/
double		tec_string_to_double(char *string, int *error);

/*tec_char_is_white_space:
	returns 1 if c is a white space character (e.g. space)
	returns 0 otherwise
	assumes 7 bit ascii character
	there are more white space characters within unicode
	they are not so commonly used and could not all be considered in just an 8 bit character
*/
int			tec_char_is_white_space(char c);

/*tec_buf_begins:
	returns 1 if the buffer begins with string str
	returns 0 in all other cases, including errors and str being longer than buffer
*/
int			tec_buf_begins(char *buffer, char *str);

/*tec_buf_find_str:
	returns index in buffer where string to_find begins
	returns -1 when not found
	returns -1 when string is empty or buffer points to NULL
*/
int			tec_buf_find_str(char *buffer, int buffer_size, char *to_find);

/*tec_buf_cmp:
	compares 2 buffers with each other from 'begin' bytes in until and including 'end'
	do ensure the buffers are long enough
	returns 1 if they are equal
	returns 0 in all other cases (including any or both are NULL pointers)
*/
int			tec_buf_cmp(char *buf1, char *buf2, int begin, int end);

/******************************************************************************
	file/path related functions
******************************************************************************/

/*tec_file_exists:
	returns 1 if file exists
	returns 0 in all other cases ( doesn't exist or error )
	assumes a 7bit ASCII string
*/
int			tec_file_exists(char *path);

/*tec_file_get_size:
	returns the size of the file in bytes
	returns -1 in case of error e.g. file does not exist
*/
int64_t		tec_file_get_size(char *path);

/*tec_file_rename(char *old_name, char *new_name):
	returns zero in case of success
	returns non-zero in case of failure
*/
#define tec_file_rename(X, Y) rename(X, Y)

/*tec_file_copy:
	returns non-zero on success
	returns zero on failure
*/
int			tec_file_copy(char *source, char *dest, int fail_if_file_exists);

/*tec_file_get_contents:
	this will return the raw contents of a file in a NULL terminated buffer
	returns NULL in case of error (file does not exist, no read permission, etc.)
	free the returned buffer
*/
char*		tec_file_get_contents(char *path);

/*tec_file_delete(char *file_path):
tec_file_remove(char *file_path):
	deletes a file
	file_path should be an ASCII string with maximum length of MAX_PATH
	returns zero on success
	returns non-zero on failure
*/
#ifdef __linux
#define tec_file_delete(X) unlink(X)
#define tec_file_remove(X) unlink(X)
#endif
#ifdef __WIN32
#define tec_file_delete(X) !DeleteFile(X)
#define tec_file_remove(X) !DeleteFile(X)
#endif

/*tec_dir_make:
	char *path is the path of the directory to be created
	make sure every directory along the way already exists
	only the final directory gets created
	the maximum length of path should be 248 (i.e. MAX_PATH - 12)
	this includes the final '/' or '\\'
	this is to still allow a proper filename

	to ensure compatibility with as many systems as possible
	all paths should only contain 7-bit ascii
	paths should not contain spaces

	returns non-zero on success
	returns 0 on failure (including if dir already exists)
*/
int			tec_dir_make(char *path);

/*tec_dir_delete(char *directory_path):
tec_dir_remove(char *directory_path):
	deletes an empty directory
	fails when directory is not empty
	directory_path should be an ASCII string with maximum length of MAX_PATH
	returns zero on success
	returns non-zero on failure
*/
#ifdef __linux
#define tec_dir_delete(X) rmdir(X)
#define tec_dir_remove(X) rmdir(X)
#endif
#ifdef __WIN32
#define tec_dir_delete(X) !RemoveDirectoryA(X)
#define tec_dir_remove(X) !RemoveDirectoryA(X)
#endif

/*tec_dir_parent:
	it changes char *path to that of the parent directory
	returns 1 on success
	returns 0 on failure
	on failure, char *path is unchanged
	this function assumes char *path is a valid path
*/
int			tec_dir_parent(char *path);

/*tec_dir_sub:
	does concatenate sub_dir to path
	will insert forward slash or backslash as required
	returns 1 in case of success
	returns 0 in case of error
*/
int			tec_dir_sub(char *path, char *sub_dir, int path_size);

/*tec_path_get_filename:
	takes a string containing a full path and changes it to just the filename
	(whatever comes after last '/' or '\\' )
	does nothing if char *path is already filename
	will modify given character string
	make a copy if needed
	returns 1 on success
	returns 0 on failure
		could happen because path is empty
		or path contains no filename
*/
int			tec_path_get_filename(char *path);

/*tec_path_is_dir:
tec_path_is_directory:
	returns 1 is char *path is a directory
	returns 0 in all other cases
*/
int			tec_path_is_directory(char *path);
#define tec_path_is_dir(X) tec_path_is_directory(X)

/******************************************************************************
	system related functions
******************************************************************************/

/*tec_sys_launch_exe:
	note for linux users:
		running this command does not involve bash shell searching through /usr/bin
		and similar directories to find your application,
		thus make sure to include full path to your program
	note for windows
		for similar reasons, full path needs to be included
	command should not be longer than 1024 characters (including terminating NULL)
*/
int			tec_sys_launch_exe(char *command);

/*tec_sys_launch_process:
	launches a process with redirected input and output
	char *cmd: the command to be executed
	command should not be longer than 1024 characters (including terminating NULL)
	void (*func_out)(): callback for the output (stdout)
	void (*func_error)(): callback for the output (stderr)

	func_out and func_error should have prototypes:
	void func_out(tec_process *tp, void *data);
	void func_error(tec_process *tp, void *data);

tec_process_input(tec_process *tp, char *input);
	pass on input to the process launched with tec_sys_launch_process

	free the struct returned by tec_sys_launch_process once the process is no longer running
*/
tec_process*	tec_sys_launch_process(char *cmd, void (*func_out)(), void *data_out, void (*func_error)(), void *data_error);
void			tec_sys_process_input(tec_process *tp, char *text);

/*tec_error:
	prints an error string to STDERR

	supports %d, %c and %s, just like in printf
	that's all it supports right now
	so, no %g or other formats like %2d

	TODO(GI):
	1. expands with more formatting options

*/
#define tec_error(...) t_error(__FILE__, __LINE__, __VA_ARGS__);

void*		t_sys_launch_process_output(void *tp);
void*		t_sys_launch_process_error(void *tp);
void		t_error(char *file, int line, char *error_msg, ...);

#include "technical.c"

#endif
