//
//  main.m
//  user-namespace-execd
//
//  Created by Conor Hughes on 2/19/14.
//
//

#include <stdio.h>

#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <err.h>
#include <sysexits.h>

#include "user-namespace-exec-constants.h"
#include "une_daemonServer.h"

static void ipc_loop(void *context)
{
	kern_return_t kr = KERN_SUCCESS;
	typedef union {
		union __RequestUnion__unedaemonserver_ipc_subsystem req;
		union __ReplyUnion__unedaemonserver_ipc_subsystem rep;
	} MAX_MSG_BUFFER;
	
	mach_port_t port = (mach_port_t) ((uintptr_t) context);
	
	do {
		kr = mach_msg_server_once(ipc_server,
								  sizeof(MAX_MSG_BUFFER)+ MAX_TRAILER_SIZE,
								  port,
								  0);
		if (kr == MACH_RCV_INTERRUPTED) {
			kr = KERN_SUCCESS;
		}
	} while (kr == KERN_SUCCESS);
	
	errx(EX_OSERR, "mach_msg_server_once() failed: %d: %s", kr, mach_error_string(kr));
}

kern_return_t unedaemonserver_get_agent(mach_port_t sp, uint32_t session_type, uint32_t *out_pid)
{
	fprintf(stderr, "HI MOM %d\n", session_type);
	*out_pid = 42;
	return KERN_SUCCESS;
}

int main(int argc, const char * argv[])
{
	mach_port_t client_mp = MACH_PORT_NULL;
	
	kern_return_t kr = bootstrap_check_in(bootstrap_port, USER_EXEC_CLIENT_CONNECTION_NAME, &client_mp);
	if(KERN_SUCCESS != kr) {
		errx(EX_UNAVAILABLE, "bootstrap_check_in() failed: %d: %s", kr, mach_error_string(kr));
	}
	
	ipc_loop((void*)(uintptr_t)client_mp);
	
    return 0;
}
