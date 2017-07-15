#!/bin/sh
. /www/cgi-bin/func.sh
eval "`/www/cgi-bin/proccgi $*`"
refreshed_page=${QUERY_STRING}

SYS_PREFIX=$(${nvram} get leafp2p_sys_prefix)

EXEC_RESULT="register_fail"

cd "${SYS_PREFIX}/bin"
. "/opt/remote/bin/comm.sh"

case "$FORM_submit_flag" in
    register_user)
		 do_register "$FORM_TXT_remote_login" "$FORM_TXT_remote_password" && {
			if [ "xSUCCESS" == "x$COMM_RESULT" ]; then
				EXEC_RESULT="register_ok"				
			else
				EXEC_RESULT="register_fail"
			fi
		}
	;;
    unregister_user)
		do_unregister "$FORM_TXT_remote_login" "$FORM_TXT_remote_password" && {
			if [ "xSUCCESS" == "x$COMM_RESULT" ]; then
				EXEC_RESULT="unreg_ok"
			else
				EXEC_RESULT="unreg_fail"
			fi
		}
	;;
esac

#HTTP_URL=`echo ${HTTP_HOST} | sed '$s/.$//' `
#uncomment the last line above, because we save HTTP_HOST in our router is just numbers . no \r\n or \n kind of thing. otherwise the HTTP_HOST will be removed the last number.(ep 192.168.1.1 --> 192.168.1.) and return error.
HTTP_URL=`echo ${HTTP_HOST} `
print_http_refresh "http://$HTTP_URL${refreshed_page}?${EXEC_RESULT}" "0"

exec >&-
