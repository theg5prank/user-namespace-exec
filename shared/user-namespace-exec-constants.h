//
//  user-namespace-exec-constants.h
//  user-namespace-exec
//
//  Created by Conor Hughes on 2/19/14.
//
//

#ifndef user_namespace_exec_user_namespace_exec_constants_h
#define user_namespace_exec_user_namespace_exec_constants_h

#define USER_EXEC_SERVICE_NAME "net.conorh.user-namespace-execd"

enum user_exec_session_types {
	USER_EXEC_SESSION_TYPE_SYSTEM = 0,
	USER_EXEC_SESSION_TYPE_BACKGROUND,
	USER_EXEC_SESSION_TYPE_AQUA,
	USER_EXEC_SESSION_TYPE_LOGINWINDOW,
	USER_EXEC_SESSION_TYPE_STANDARDIO,
};
#define USER_EXEC_SESSION_TYPE_MAX USER_EXEC_SESSION_TYPE_STANDARDIO

#endif
