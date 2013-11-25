// shell_cmds.c, Phase 7, Sac State CpE/CSc 159
// shell subroutines to perform "dir/type/print" commands
// complete the pseudo code below, prototype them into "shell_cmds.h"

#include "spede.h"
#include "types.h"
#include "sys_calls.h"
#include "op_codes.h" // operation codes: OK, STAT, etc.
#include "filesys.h"
#include "externs.h"
#include "q_mgmt.h"   // MyStrCmp(), etc.
#include "shell_cmds.h"

// buld a string: a detail line of attributes of the given file/directory
// that p points to (stat_t)
void DirLine(stat_t *p, char *line) {
	// we get back in msg.bytes two parts: stat_t type and path
	// by type-casting "p+1" pointer, we get a pointer to the path
	char *path = (char *)(p + 1);

	// build output in the line from what p points
	sprintf(line, "----  size=%5d     %s\n", p->size, path);

	if ( S_ISDIR(p->mode) ) line[0] = 'd';  // mode is directory
	if ( QBIT_ON(p->mode, S_IROTH) ) line[1] = 'r'; // mode is readable
	if ( QBIT_ON(p->mode, S_IWOTH) ) line[2] = 'w'; // mode is writable
	if ( QBIT_ON(p->mode, S_IXOTH) ) line[3] = 'x'; // mode is executable
}

// do "dir" listing, Shell() communicates with FileSys() and Stdout()
// make sure str passed has trailing 0: "dir\0" or "dir something\0"
void ShellDir(char *str, int stdout_pid, int file_sys_pid) {
	char path[NUM_BYTE], line[NUM_BYTE];
	stat_t *p;
	msg_t msg;
	int fd;
	// if str is "dir\0" assume home (root) dir "/"
	// else, assume user specified a path after first 4 letters "dir "
	if(MyStrCmp(str, "dir\0") == 1) {
		path[0] = '/';
		path[1] = '\0'; // null-terminate the path[]
	} else { // skip 1st 4 letters "dir " and get the path
		str += 4;
		MyStrCpy(path, str); // make sure str is null-terminated from Shell()
	}


	//*************************************************************************
	// write code:
	// prep msg: path[] in msg.bytes:
	//   MyMemCpy(msg.bytes, path, sizeof(path));
	// put op code STAT in msg.numbers[0]
	// send msg to FileSys, receive from FileSys, result is msg.numbers[0]
	//*************************************************************************
	MyStrCpy(msg.bytes, path);
	msg.numbers[0] = STAT;

	MsgSnd(file_sys_pid, &msg); 
	MsgRcv(&msg);

	if(msg.numbers[0] != OK) {
		sprintf(msg.bytes,"dir: error (%d) stat-ing (%s)\n", msg.numbers[0], path);
		MsgSnd(stdout_pid, &msg);
		MsgRcv(&msg);

		//*************************************************************************
		// write code:
		// send an error msg to Stdout
		// receive synchro msg from Stdout
		//*************************************************************************
		return;        // we can't continue
	}

	// By op_code STAT, file sys returns in msg.bytes a "stat_t" type,
	// take a peek, if user directed us to a file, then display info for only
	// that file; otherwise, it's a directory, needs to display each entry in
	// the dir content as if we are "listing" it
	p = (stat_t *)msg.bytes;    // p, status type pointer

	if( ! S_ISDIR(p->mode) ) // if not dir, it's a file, detail-list it
	{
		DirLine(p, line); // line is to be built and returned by subroutine
		MyStrCpy(msg.bytes, line);
		MsgSnd(stdout_pid, &msg);
		MsgRcv(&msg);  

		//*************************************************************************
		// write code:
		// prep and send msg to Stdout with line[] copied to msg.bytes
		// receive synchro msg from Stdout
		//*************************************************************************
		return;     // we can't continue
	}
	// it's a dir, we list its content (entries), one by one in a loop
	// we 1st request FileSys to open the dir, and then issue a read on
	// each entry in it in the loop

	//*************************************************************************
	// write code:
	// prep msg: put path[] in msg.bytes, put op code OPEN in msg.numbers[0]
	// send msg to FileSys, receive msg from file sys (msg.numbers[0] is result
	// code and msg.numbers[2] is the fd assigned)
	//*************************************************************************
	MyStrCpy(msg.bytes, path);
	msg.numbers[0] = OPEN;
	MsgSnd(file_sys_pid, &msg);
	MsgRcv(&msg);
	fd = msg.numbers[2];
	while(msg.numbers[0] == OK) {
		//*************************************************************************
		// write code:
		// prep msg: put op code READ in msg.numbers[0], send to FileSys,
		// receive from FileSys, what's read is in msg.bytes
		// if result not OK, break loop
		//*************************************************************************
		msg.numbers[0] = READ;
		msg.numbers[2] = fd;
		MsgSnd(file_sys_pid, &msg);
		MsgRcv(&msg);
		p = (stat_t *)msg.bytes;
		DirLine(p, line); // convert msg.bytes into a detail line
		if (msg.numbers[0] != OK)
		{
			return; // we cant go past this if the DirLine doesnt work 
		}
		MyStrCpy(msg.bytes, line);
		MsgSnd(stdout_pid,&msg);
		MsgRcv(&msg);
		//*************************************************************************
		// write code:
		// prep and send msg to Stdout to show the converted line[]
		// receive synchro msg from Stdout
		//*************************************************************************
	}

	msg.numbers[0] = CLOSE;
	msg.numbers[2] = fd;
	MsgSnd(file_sys_pid, &msg);
	MsgRcv(&msg);

	//*************************************************************************
	// write code:
	// request FileSys to close
	// if result NOT OK, display an error msg...
	//*************************************************************************
	if (msg.numbers[0] != OK)
	{
		MyStrCpy(msg.bytes, "Error: Can not close!\n\0");
		MsgSnd(stdout_pid,&msg);
		MsgRcv(&msg);
	}
}

void ShellType(char *str, int stdout_pid, int file_sys_pid) {
	char path[NUM_BYTE];
	stat_t *p;
	msg_t msg;

	//*************************************************************************
	// write code:
	// if str is "type\0" display "Usage: type <filename>\n\0"
	//    return;                  // can't continue
	//
	// skip 1st 5 characters in str ("type ") to get the rest (str)
	// copy str to msg.bytes
	// ask FileSys to STAT this
	// if result NOT OK, display an error msg...
	//    return;         // we can't continue
	//
	// By op_code STAT, file sys returns in msg.bytes a "stat_t" type,
	// take a peek, if user directed us to a file, then display info for only
	// that file; otherwise, it's a directory, display an error msg to user.
	//
	// p = (stat_t *)msg.bytes;    // p, status type pointer
	// if( S_ISDIR(p->mode) ) {    // if dir, can't do it
	//    display an error msg...
	//    return;                  // we can't continue
	//*************************************************************************
	if(MyStrCmp("type\0", str))
	{
		path[1] = '\0'; // null-terminate
		path[0] = '/'; // start for file path
	}
	else // if not the type, get the path
	{
		str = str + 5;
		MyStrCpy(path, str); 
	}
	MyStrCpy(msg.bytes, path);
	msg.numbers[0] = STAT;

	MsgSnd(file_sys_pid, &msg);
	MsgRcv(&msg);
	// If the resutl is not ok:
	if (msg.numbers[0] != OK)
	{
		MyStrCpy(msg.bytes,"Error: Can not read file!\n\0");
		MsgSnd(stdout_pid, &msg);
		MsgRcv(&msg);
		return; // cant continue 
	}

	p = (stat_t *)msg.bytes; 
	//*************************************************************************
	// write code:
	// request FileSys to OPEN (prep msg, send FileSys)
	// then start READ loop (until NOT OK)
	while(msg.numbers[0] == OK) {          // so long OK
		//    request FileSys to READ (prep msg, send FileSys)
		//    receive back in msg, text in msg.bytes
		//
		//    if(msg.numbers[0] != OK) break;
		//
		//    send it to Stdout to show it to terminal
		//    receive synchro msg back
		//*************************************************************************
		if (S_ISDIR(p->mode))
		{
			MyStrCpy(msg.bytes,"Error: Cant read a dir!\n\0");
			MsgSnd(stdout_pid, &msg);
			MsgRcv(&msg);
			return;
		}else{
			MyStrCpy(msg.bytes, path);
			msg.numbers[0] = OPEN;
			MsgSnd(file_sys_pid, &msg);
			MsgRcv(&msg);

			msg.numbers[0] = READ;
			MsgSnd(file_sys_pid, &msg);
			MsgRcv(&msg);

			while(msg.numbers[0] != END_OF_FILE)
			{
				MsgSnd(stdout_pid, &msg);
				MsgRcv(&msg);

				msg.numbers[0] = READ;
				MsgSnd(file_sys_pid, &msg);
				MsgRcv(&msg);
			}
			//Reached end of file

			MyStrCpy(msg.bytes,"\n\0");
			MsgSnd(stdout_pid, &msg);
			MsgRcv(&msg);

			msg.numbers[0] = CLOSE;
			MsgSnd(file_sys_pid, &msg);
			MsgRcv(&msg);
			return;
		}
	}
	//*************************************************************************
	// write code:
	// request FileSys to close
	// if result NOT OK, display an error msg...
	//*************************************************************************
	if (msg.numbers[0] != OK)
	{
		MyStrCpy(msg.bytes,"Error: Can not read file!\n\0");
		MsgSnd(stdout_pid, &msg);
		MsgRcv(&msg);
		return; // cant continue 
	}
}

//*************************************************************************
// write code:
// Other shell commands...
//*************************************************************************

void ShellHelp(int stdout_pid){
	msg_t msg;
	char print_help[] = "\n====================Walls OS=====================\n";
	MyStrCpy(msg.bytes, print_help);
	MsgSnd(stdout_pid, &msg);
	MsgRcv(&msg);

	//print buffer small, spliting to multiple messages
	MyStrCpy(print_help, "\tHelp\n\twho:\tDisplays our OS team name\n\n\tbye:\tQuits the shell\n");
	MyStrCpy(msg.bytes, print_help);
	MsgSnd(stdout_pid,&msg);
	MsgRcv(&msg);

	//Buffer still small, getting segfult, moving spliting message
	MyStrCpy(print_help, "\tprint <filename>:\tprints specified file\n");
	MyStrCpy(msg.bytes, print_help);
	MsgSnd(stdout_pid,&msg);
	MsgRcv(&msg);

	MyStrCpy(print_help,  "\tdir [path]:\tlists the content of the directory.\n\ttype <filename>:\tPrints the file\n");  
	MyStrCpy(msg.bytes, print_help);
	MsgSnd(stdout_pid,&msg);
	MsgRcv(&msg);

	MyStrCpy(print_help,  "\n====================Walls OS=====================\n");  
	MyStrCpy(msg.bytes, print_help);
	MsgSnd(stdout_pid,&msg);
	MsgRcv(&msg);

}


void ShellWho(int stdout_pid){
	// The who commands :) 
	msg_t msg;
	char dr_who[] = "Walls OS! !Windows\n\t\t all right reserved || Fall 2013\n\0";
	MyStrCpy(msg.bytes, dr_who);
	MsgSnd(stdout_pid, &msg);
	MsgRcv(&msg);
}

void ShellPrint(char *str,int printd_pid,int file_sys_pid)
{
	char path[NUM_BYTE];
	stat_t *p;
	msg_t msg;

	if(MyStrCmp("print\0", str))
	{
		path[0] = '/';
		path[1] = '\0'; // null-terminate the path[]
	}
	else // skip 1st 5 letters "type " and get the file name
	{
		str += 6;
		MyStrCpy(path, str); // make sure str is null-terminated from Shell()
	}

	MyStrCpy(msg.bytes, path);
	msg.numbers[0] = STAT;

	MsgSnd(file_sys_pid, &msg);
	MsgRcv(&msg);

	if(msg.numbers[0] != OK)
	{
		MyStrCpy(msg.bytes,"Error: File not found\n\0");
		MsgSnd(printd_pid, &msg);
		return;
	}

	p = (stat_t *)msg.bytes;    

	if(S_ISDIR(p->mode) )  // if its not a directory Just like the type
	{
		MyStrCpy(msg.bytes,"file not found or unreadable\n\0");
		MsgSnd(printd_pid, &msg);
		return;
	}
	else
	{

		MyStrCpy(msg.bytes, path);

		msg.numbers[0] = OPEN;
		MsgSnd(file_sys_pid, &msg);
		MsgRcv(&msg);

		msg.numbers[0] = READ;
		MsgSnd(file_sys_pid, &msg);
		MsgRcv(&msg);

		while(msg.numbers[0] != END_OF_FILE)
		{
			MsgSnd(printd_pid, &msg);

			msg.numbers[0] = READ;
			MsgSnd(file_sys_pid, &msg);
			MsgRcv(&msg);
		}

		MyStrCpy(msg.bytes,"\n\0");
		MsgSnd(printd_pid, &msg);

		msg.numbers[0] = CLOSE;
		MsgSnd(file_sys_pid, &msg);
		MsgRcv(&msg);
		return;
	}
}

