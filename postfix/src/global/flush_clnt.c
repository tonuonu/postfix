/*++
/* NAME
/*	flush_clnt 3
/* SUMMARY
/*	fast flush cache manager client interface
/* SYNOPSIS
/*	#include <flush_clnt.h>
/*
/*	int	flush_add(site, queue_id)
/*	const char *site;
/*	const char *queue_id;
/*
/*	int	flush_send(site)
/*	const char *site;
/*
/*	int	flush_purge()
/* DESCRIPTION
/*	The following routines operate through the "fast flush" service.
/*	This service maintains a cache of what mail is queued. The cache
/*	is maintained for eligible destinations. A destination is the
/*	right-hand side of a user@domain email address.
/*
/*	flush_add() informs the "fast flush" cache manager that mail is
/*	queued for the specified site with the specified queue ID.
/*
/*	flush_send() requests delivery of all mail that is queued for
/*	the specified destination.
/*
/*	flush_purge() requests the "fast flush" cache manager to refresh
/*	cached information that was not used or not updated for some
/*	configurable amount of time.
/* DIAGNOSTICS
/*	The result codes and their meanings are (see flush_clnt(5h)):
/* .IP MAIL_FLUSH_OK
/*	The request completed successfully.
/* .IP MAIL_FLUSH_FAIL
/*	The request failed (the request could not be sent to the server,
/*	or the server reported failure).
/* .IP MAIL_FLUSH_BAD
/*	The "fast flush" server rejected the request (invalid request
/*	parameter).
/* SEE ALSO
/*	flush(8) Postfix fast flush cache manager
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include "sys_defs.h"
#include <unistd.h>
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <vstream.h>

/* Global library. */

#include <mail_proto.h>
#include <mail_flush.h>
#include <flush_clnt.h>
#include <mail_params.h>

/* Application-specific. */

#define STR(x)	vstring_str(x)

/* flush_clnt - generic fast flush service client */

static int flush_clnt(const char *format,...)
{
    VSTREAM *flush;
    int     status;
    va_list ap;

    /*
     * Connect to the fast flush service over local IPC.
     */
    if ((flush = mail_connect(MAIL_CLASS_PRIVATE, MAIL_SERVICE_FLUSH,
			      BLOCKING)) == 0)
	return (FLUSH_STAT_FAIL);

    /*
     * Do not get stuck forever.
     */
    vstream_control(flush,
		    VSTREAM_CTL_TIMEOUT, var_ipc_timeout,
		    VSTREAM_CTL_END);

    /*
     * Send a request with the site name, and receive the request acceptance
     * status.
     */
    va_start(ap, format);
    mail_vprint(flush, format, ap);
    va_end(ap);
    if (mail_scan(flush, "%d", &status) != 1)
	status = FLUSH_STAT_FAIL;

    /*
     * Clean up.
     */
    vstream_fclose(flush);

    return (status);
}

/* flush_purge - house keeping */

int     flush_purge(void)
{
    char   *myname = "flush_purge";
    int     status;

    if (msg_verbose)
	msg_info("%s", myname);

    /*
     * Don't bother the server if the service is turned off.
     */
    if (strcmp(var_fflush_policy, FFLUSH_POLICY_NONE) == 0)
	status = FLUSH_STAT_OK;
    else
	status = flush_clnt("%s", FLUSH_REQ_PURGE);

    if (msg_verbose)
	msg_info("%s: status %d", myname, status);

    return (status);
}

/* flush_send - deliver mail queued for site */

int     flush_send(const char *site)
{
    char   *myname = "flush_send";
    int     status;

    if (msg_verbose)
	msg_info("%s: site %s", myname, site);

    /*
     * Don't bother the server if the service is turned off.
     */
    if (strcmp(var_fflush_policy, FFLUSH_POLICY_NONE) == 0)
	status = mail_flush_deferred();
    else
	status = flush_clnt("%s %s", FLUSH_REQ_SEND, site);

    if (msg_verbose)
	msg_info("%s: site %s status %d", myname, site, status);

    return (status);
}

/* flush_add - inform "fast flush" cache manager */

int     flush_add(const char *site, const char *queue_id)
{
    char   *myname = "flush_add";
    int     status;

    if (msg_verbose)
	msg_info("%s: site %s id %s", myname, site, queue_id);

    /*
     * Don't bother the server if the service is turned off.
     */
    if (strcmp(var_fflush_policy, FFLUSH_POLICY_NONE) == 0)
	status = FLUSH_STAT_OK;
    else
	status = flush_clnt("%s %s %s", FLUSH_REQ_ADD, site, queue_id);

    if (msg_verbose)
	msg_info("%s: site %s id %s status %d", myname, site, queue_id,
		 status);

    return (status);
}