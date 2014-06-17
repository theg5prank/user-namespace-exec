//
//  main.m
//  user-namespace-exec-agent
//
//  Created by Conor Hughes on 2/19/14.
//
//

#include <stdlib.h>
#include <err.h>
#include <sysexits.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>

#include "user-namespace-exec-constants.h"
#include "une_agent.h"
#include "notifyServer.h"

#define WAIT_TIME 25000

mach_port_t notification_port = MACH_PORT_NULL;
uint32_t session_id = ~0;

void register_with_daemon(void)
{
	mach_port_t server_port = MACH_PORT_NULL;
	mach_port_t prev_notification_port = MACH_PORT_NULL;
	mach_port_t death_port = MACH_PORT_NULL;
	
	kern_return_t kr = bootstrap_look_up(bootstrap_port, USER_EXEC_SERVICE_NAME, &server_port);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't look up service: %s", mach_error_string(kr));
	}
	
	kr = unedaemonagent_register(server_port, session_id, bootstrap_port, MACH_MSG_TYPE_COPY_SEND, &death_port, WAIT_TIME);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't register bootstrap port: %s", mach_error_string(kr));
	} else if (death_port == MACH_PORT_NULL || death_port == MACH_PORT_DEAD) {
		errx(EX_SOFTWARE, "Got a bogus death port 0x%08x", death_port);
	}
	
	mach_port_deallocate(mach_task_self(), server_port);
	
	kr = mach_port_request_notification(mach_task_self(), death_port, MACH_NOTIFY_DEAD_NAME, 0, notification_port, MACH_MSG_TYPE_MAKE_SEND_ONCE, &prev_notification_port);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't request dead name notification: %s", mach_error_string(kr));
	} else if (prev_notification_port != MACH_PORT_NULL) {
		errx(EX_OSERR, "Previous notification port for new port was %u?", prev_notification_port);
	}
}

kern_return_t do_mach_notify_dead_name(mach_port_t notify, mach_port_name_t name)
{
	kern_return_t kr = KERN_SUCCESS;
	warnx("Daemon died. Reconnecting...");
	
	kr = mach_port_deallocate(mach_task_self(), name);
	
	if (kr != KERN_SUCCESS) {
		warnx("Could not deallocate supposed dead name: %s", mach_error_string(kr));
	}

	register_with_daemon();

	return KERN_SUCCESS;
}

int main(int argc, const char * argv[])
{
	if (argc != 2) {
		errx(EX_USAGE, "usage: %s <session>", getprogname());
	}

	const char *session = argv[1];
	
	if (!strcasecmp(session, "System")) {
		session_id = USER_EXEC_SESSION_TYPE_SYSTEM;
	} else if (!strcasecmp(session, "Aqua")) {
		session_id = USER_EXEC_SESSION_TYPE_AQUA;
	} else if (!strcasecmp(session, "Background")) {
		session_id = USER_EXEC_SESSION_TYPE_BACKGROUND;
	} else if (!strcasecmp(session, "LoginWindow")) {
		session_id = USER_EXEC_SESSION_TYPE_LOGINWINDOW;
	} else if (!strcasecmp(session, "StandardIO")) {
		session_id = USER_EXEC_SESSION_TYPE_STANDARDIO;
	} else {
		errx(EX_USAGE, "invalid session type %s", session);
	}

	kern_return_t kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &notification_port);
	
	if (kr != KERN_SUCCESS) {
		errx(EX_SOFTWARE, "Couldn't allocate mach port: %s", mach_error_string(kr));
	}
	
	register_with_daemon();
	
	typedef union {
		union __RequestUnion__do_notify_subsystem req;
		union __ReplyUnion__do_notify_subsystem rep;
	} MAX_MSG_BUFFER;
	
	do {
		kr = mach_msg_server_once(notify_server,
								  sizeof(MAX_MSG_BUFFER)+ MAX_TRAILER_SIZE,
								  notification_port,
								  0);
		if (kr == MACH_RCV_INTERRUPTED) {
			kr = KERN_SUCCESS;
		}
	} while (kr == KERN_SUCCESS);

	errx(EXIT_FAILURE, "mach_msg_server_once failed: %s", mach_error_string(kr));
}

kern_return_t do_mach_notify_port_deleted(mach_port_t notify, mach_port_name_t name)
{ errx(EX_OSERR, "Did not expect name to be made available rather than dead!?"); }

kern_return_t do_mach_notify_port_destroyed(mach_port_t notify, mach_port_t rights)
{ errx(EX_OSERR, "Did not request any port destroyed notifications!?"); }

kern_return_t do_mach_notify_no_senders(mach_port_t notify, mach_port_mscount_t mscount)
{ errx(EX_OSERR, "Did not request any no senders notifications!?"); }

kern_return_t do_mach_notify_send_once(mach_port_t notify)
{ errx(EX_OSERR, "A send-once right was destroyed!?"); }
