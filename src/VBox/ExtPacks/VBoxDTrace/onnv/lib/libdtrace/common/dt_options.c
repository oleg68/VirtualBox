/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef VBOX
#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/types.h>

#include <strings.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <alloca.h>
#include <errno.h>
#include <fcntl.h>

#else  /* VBOX */
# ifndef _MSC_VER
#  include <sys/resource.h>
#  include <unistd.h>
#  include <signal.h>
#  if defined(RT_OS_DARWIN)
#   define open64 open
#  endif
# else
#  include <io.h>
#  define STDERR_FILENO 2
#  define open64 open
# endif
# include <fcntl.h>
# include <stdlib.h>

#endif /* VBOX */

#include <dt_impl.h>
#include <dt_string.h>

static int
dt_opt_agg(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dt_aggregate_t *agp = &dtp->dt_aggregate;

	if (arg != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	agp->dtat_flags |= option;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_amin(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	char str[DTRACE_ATTR2STR_MAX];
	dtrace_attribute_t attr;
	RT_NOREF1(option);

	if (arg == NULL || dtrace_str2attr(arg, &attr) == -1)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dt_dprintf("set compiler attribute minimum to %s\n",
	    dtrace_attr2str(attr, str, sizeof (str)));

	if (dtp->dt_pcb != NULL) {
		dtp->dt_pcb->pcb_cflags |= DTRACE_C_EATTR;
		dtp->dt_pcb->pcb_amin = attr;
	} else {
		dtp->dt_cflags |= DTRACE_C_EATTR;
		dtp->dt_amin = attr;
	}

	return (0);
}

static void
dt_coredump(void)
{
	const char msg[] = "libdtrace DEBUG: [ forcing coredump ]\n";

#ifndef _MSC_VER
	struct sigaction act;
	struct rlimit lim;
#endif

	int res = write(STDERR_FILENO, msg, sizeof (msg) - 1);
	NOREF(res);

#ifndef _MSC_VER
	act.sa_handler = SIG_DFL;
	act.sa_flags = 0;

	(void) sigemptyset(&act.sa_mask);
	(void) sigaction(SIGABRT, &act, NULL);

	lim.rlim_cur = RLIM_INFINITY;
	lim.rlim_max = RLIM_INFINITY;

	(void) setrlimit(RLIMIT_CORE, &lim);
#endif /* !_MSC_VER */
	abort();
}

/*ARGSUSED*/
static int
dt_opt_core(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	static int enabled = 0;
	RT_NOREF1(option);

	if (arg != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (enabled++ || atexit(dt_coredump) == 0)
		return (0);

	return (dt_set_errno(dtp, errno));
}

/*ARGSUSED*/
static int
dt_opt_cpp_hdrs(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF1(option);

	if (arg != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (dtp->dt_pcb != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTCTX));

	if (dt_cpp_add_arg(dtp, "-H") == NULL)
		return (dt_set_errno(dtp, EDT_NOMEM));

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_cpp_path(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	char *cpp;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (dtp->dt_pcb != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTCTX));

	if ((cpp = strdup(arg)) == NULL)
		return (dt_set_errno(dtp, EDT_NOMEM));

	dtp->dt_cpp_argv[0] = (char *)strbasename(cpp);
	free(dtp->dt_cpp_path);
	dtp->dt_cpp_path = cpp;

	return (0);
}

static int
dt_opt_cpp_opts(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	char *buf;
	size_t len;
	const char *opt = (const char *)option;

	if (opt == NULL || arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (dtp->dt_pcb != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTCTX));

	len = strlen(opt) + strlen(arg) + 1;
	buf = alloca(len);

	(void) strcpy(buf, opt);
	(void) strcat(buf, arg);

	if (dt_cpp_add_arg(dtp, buf) == NULL)
		return (dt_set_errno(dtp, EDT_NOMEM));

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_ctypes(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	int fd;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if ((fd = open64(arg, O_CREAT | O_WRONLY, 0666)) == -1)
		return (dt_set_errno(dtp, errno));

	(void) close(dtp->dt_cdefs_fd);
	dtp->dt_cdefs_fd = fd;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_droptags(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF2(option, arg);

	dtp->dt_droptags = 1;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_dtypes(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	int fd;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if ((fd = open64(arg, O_CREAT | O_WRONLY, 0666)) == -1)
		return (dt_set_errno(dtp, errno));

	(void) close(dtp->dt_ddefs_fd);
	dtp->dt_ddefs_fd = fd;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_debug(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF1(option);

	if (arg != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	_dtrace_debug = 1;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_iregs(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	int n;
	RT_NOREF2(arg, option);

	if (arg == NULL || (n = atoi(arg)) <= 0)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dtp->dt_conf.dtc_difintregs = n;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_lazyload(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF2(arg, option);
	dtp->dt_lazyload = 1;

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_ld_path(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	char *ld;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (dtp->dt_pcb != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTCTX));

	if ((ld = strdup(arg)) == NULL)
		return (dt_set_errno(dtp, EDT_NOMEM));

	free(dtp->dt_ld_path);
	dtp->dt_ld_path = ld;

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_libdir(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dt_dirpath_t *dp;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if ((dp = malloc(sizeof (dt_dirpath_t))) == NULL ||
	    (dp->dir_path = strdup(arg)) == NULL) {
		free(dp);
		return (dt_set_errno(dtp, EDT_NOMEM));
	}

	dt_list_append(&dtp->dt_lib_path, dp);
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_linkmode(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (strcmp(arg, "kernel") == 0)
		dtp->dt_linkmode = DT_LINK_KERNEL;
	else if (strcmp(arg, "primary") == 0)
		dtp->dt_linkmode = DT_LINK_PRIMARY;
	else if (strcmp(arg, "dynamic") == 0)
		dtp->dt_linkmode = DT_LINK_DYNAMIC;
	else if (strcmp(arg, "static") == 0)
		dtp->dt_linkmode = DT_LINK_STATIC;
	else
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_linktype(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (strcasecmp(arg, "elf") == 0)
		dtp->dt_linktype = DT_LTYP_ELF;
	else if (strcasecmp(arg, "dof") == 0)
		dtp->dt_linktype = DT_LTYP_DOF;
	else
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_evaltime(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (strcmp(arg, "exec") == 0)
		dtp->dt_prcmode = DT_PROC_STOP_CREATE;
	else if (strcmp(arg, "preinit") == 0)
		dtp->dt_prcmode = DT_PROC_STOP_PREINIT;
	else if (strcmp(arg, "postinit") == 0)
		dtp->dt_prcmode = DT_PROC_STOP_POSTINIT;
	else if (strcmp(arg, "main") == 0)
		dtp->dt_prcmode = DT_PROC_STOP_MAIN;
	else
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_pgmax(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	int n;
	RT_NOREF1(option);

	if (arg == NULL || (n = atoi(arg)) < 0)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dtp->dt_procs->dph_lrulim = n;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_stdc(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (dtp->dt_pcb != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTCTX));

	if (strcmp(arg, "a") == 0)
		dtp->dt_stdcmode = DT_STDC_XA;
	else if (strcmp(arg, "c") == 0)
		dtp->dt_stdcmode = DT_STDC_XC;
	else if (strcmp(arg, "s") == 0)
		dtp->dt_stdcmode = DT_STDC_XS;
	else if (strcmp(arg, "t") == 0)
		dtp->dt_stdcmode = DT_STDC_XT;
	else
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_syslibdir(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dt_dirpath_t *dp = dt_list_next(&dtp->dt_lib_path);
	char *path;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if ((path = strdup(arg)) == NULL)
		return (dt_set_errno(dtp, EDT_NOMEM));

	free(dp->dir_path);
	dp->dir_path = path;

	return (0);
}


/*ARGSUSED*/
static int
dt_opt_tree(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	int m;
	RT_NOREF1(option);

	if (arg == NULL || (m = atoi(arg)) <= 0)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dtp->dt_treedump = m;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_tregs(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	int n;
	RT_NOREF1(option);

	if (arg == NULL || (n = atoi(arg)) <= 0)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dtp->dt_conf.dtc_diftupregs = n;
	return (0);
}

/*ARGSUSED*/
static int
dt_opt_xlate(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (strcmp(arg, "dynamic") == 0)
		dtp->dt_xlatemode = DT_XL_DYNAMIC;
	else if (strcmp(arg, "static") == 0)
		dtp->dt_xlatemode = DT_XL_STATIC;
	else
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_cflags(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	if (arg != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (dtp->dt_pcb != NULL)
		dtp->dt_pcb->pcb_cflags |= option;
	else
		dtp->dt_cflags |= option;

	return (0);
}

static int
dt_opt_dflags(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	if (arg != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dtp->dt_dflags |= option;
	return (0);
}

static int
dt_opt_invcflags(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	if (arg != NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (dtp->dt_pcb != NULL)
		dtp->dt_pcb->pcb_cflags &= ~option;
	else
		dtp->dt_cflags &= ~option;

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_version(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dt_version_t v;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (dt_version_str2num(arg, &v) == -1)
		return (dt_set_errno(dtp, EDT_VERSINVAL));

	if (!dt_version_defined(v))
		return (dt_set_errno(dtp, EDT_VERSUNDEF));

	return (dt_reduce(dtp, v));
}

static int
dt_opt_runtime(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
#ifndef VBOX
	char *end;
#endif
	dtrace_optval_t val = 0;
	int i;

	const struct {
		char *positive;
		char *negative;
	} couples[] = {
		{ "yes",	"no" },
		{ "enable",	"disable" },
		{ "enabled",	"disabled" },
		{ "true",	"false" },
		{ "on",		"off" },
		{ "set",	"unset" },
		{ NULL }
	};

	if (arg != NULL) {
		if (arg[0] == '\0') {
			val = DTRACEOPT_UNSET;
			goto out;
		}

		for (i = 0; couples[i].positive != NULL; i++) {
			if (strcasecmp(couples[i].positive, arg) == 0) {
				val = 1;
				goto out;
			}

			if (strcasecmp(couples[i].negative, arg) == 0) {
				val = DTRACEOPT_UNSET;
				goto out;
			}
		}

#ifndef VBOX
		errno = 0;
		val = strtoull(arg, &end, 0);

		if (*end != '\0' || errno != 0 || val < 0)
			return (dt_set_errno(dtp, EDT_BADOPTVAL));
#else
		i = RTStrToInt64Full(arg, 0, &val);
		if (i != VINF_SUCCESS)
			return (dt_set_errno(dtp, EDT_BADOPTVAL));
#endif
	}

out:
	dtp->dt_options[option] = val;
	return (0);
}

static int
dt_optval_parse(const char *arg, dtrace_optval_t *rval)
{
	dtrace_optval_t mul = 1;
	size_t len;
	char *end;
#ifdef VBOX
	int rc;
#endif

	len = strlen(arg);
#ifndef VBOX
	errno = 0;
#endif

	switch (arg[len - 1]) {
	case 't':
	case 'T':
		mul *= 1024;
		RT_FALL_THRU();
	case 'g':
	case 'G':
		mul *= 1024;
		RT_FALL_THRU();
	case 'm':
	case 'M':
		mul *= 1024;
		RT_FALL_THRU();
	case 'k':
	case 'K':
		mul *= 1024;
		RT_FALL_THRU();
	default:
		break;
	}

#ifndef VBOX
	errno = 0;
	*rval = strtoull(arg, &end, 0) * mul;

	if ((mul > 1 && end != &arg[len - 1]) || (mul == 1 && *end != '\0') ||
	    *rval < 0 || errno != 0)
		return (-1);
#else
	*rval = -1;
	if (mul == 1) {
		rc = RTStrToInt64Full(arg, 0, rval);
		if (rc != VINF_SUCCESS || *rval < 0)
			return (-1);
	} else {
		rc = RTStrToInt64Ex(arg, &end, 0, rval);
		if (   rc != VWRN_TRAILING_CHARS
			|| end != &arg[len - 1]
			|| *rval < 0)
			return (-1);
		*rval *= mul;
		if (*rval < 0)
			return (-1);
	}
#endif

	return (0);
}

static int
dt_opt_size(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dtrace_optval_t val = 0;

	if (arg != NULL && dt_optval_parse(arg, &val) != 0)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dtp->dt_options[option] = val;
	return (0);
}

static int
dt_opt_rate(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	char *end;
	int i;
	dtrace_optval_t mul = 1, val = 0;

	const struct {
		char *name;
		hrtime_t mul;
	} suffix[] = {
		{ "ns", 	NANOSEC / NANOSEC },
		{ "nsec",	NANOSEC / NANOSEC },
		{ "us",		NANOSEC / MICROSEC },
		{ "usec",	NANOSEC / MICROSEC },
		{ "ms",		NANOSEC / MILLISEC },
		{ "msec",	NANOSEC / MILLISEC },
		{ "s",		NANOSEC / SEC },
		{ "sec",	NANOSEC / SEC },
		{ "m",		NANOSEC * (hrtime_t)60 },
		{ "min",	NANOSEC * (hrtime_t)60 },
		{ "h",		NANOSEC * (hrtime_t)60 * (hrtime_t)60 },
		{ "hour",	NANOSEC * (hrtime_t)60 * (hrtime_t)60 },
		{ "d",		NANOSEC * (hrtime_t)(24 * 60 * 60) },
		{ "day",	NANOSEC * (hrtime_t)(24 * 60 * 60) },
		{ "hz",		0 },
		{ NULL }
	};

	if (arg != NULL) {
#ifndef VBOX
		errno = 0;
		val = strtoull(arg, &end, 0);
#else
		int rc = RTStrToInt64Ex(arg, &end, 0, &val);
		if (rc != VWRN_TRAILING_CHARS)
			return (dt_set_errno(dtp, EDT_BADOPTVAL));
#endif

		for (i = 0; suffix[i].name != NULL; i++) {
			if (strcasecmp(suffix[i].name, end) == 0) {
				mul = suffix[i].mul;
				break;
			}
		}

		if ((suffix[i].name == NULL && *end != '\0') || val < 0)
			return (dt_set_errno(dtp, EDT_BADOPTVAL));

		if (mul == 0) {
			/*
			 * The rate has been specified in frequency-per-second.
			 */
			if (val != 0)
				val = NANOSEC / val;
		} else {
			val *= mul;
		}
	}

	dtp->dt_options[option] = val;
	return (0);
}

/*
 * When setting the strsize option, set the option in the dt_options array
 * using dt_opt_size() as usual, and then update the definition of the CTF
 * type for the D intrinsic "string" to be an array of the corresponding size.
 * If any errors occur, reset dt_options[option] to its previous value.
 */
static int
dt_opt_strsize(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dtrace_optval_t val = dtp->dt_options[option];
	ctf_file_t *fp = DT_STR_CTFP(dtp);
	ctf_id_t type = ctf_type_resolve(fp, DT_STR_TYPE(dtp));
	ctf_arinfo_t r;

	if (dt_opt_size(dtp, arg, option) != 0)
		return (-1); /* dt_errno is set for us */

	if (dtp->dt_options[option] > UINT_MAX) {
		dtp->dt_options[option] = val;
		return (dt_set_errno(dtp, EOVERFLOW));
	}

	if (ctf_array_info(fp, type, &r) == CTF_ERR) {
		dtp->dt_options[option] = val;
		dtp->dt_ctferr = ctf_errno(fp);
		return (dt_set_errno(dtp, EDT_CTF));
	}

	r.ctr_nelems = (uint_t)dtp->dt_options[option];

	if (ctf_set_array(fp, type, &r) == CTF_ERR ||
	    ctf_update(fp) == CTF_ERR) {
		dtp->dt_options[option] = val;
		dtp->dt_ctferr = ctf_errno(fp);
		return (dt_set_errno(dtp, EDT_CTF));
	}

	return (0);
}

static const struct {
	const char *dtbp_name;
	int dtbp_policy;
} _dtrace_bufpolicies[] = {
	{ "ring", DTRACEOPT_BUFPOLICY_RING },
	{ "fill", DTRACEOPT_BUFPOLICY_FILL },
	{ "switch", DTRACEOPT_BUFPOLICY_SWITCH },
	{ NULL, 0 }
};

/*ARGSUSED*/
static int
dt_opt_bufpolicy(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dtrace_optval_t policy = DTRACEOPT_UNSET;
	int i;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	for (i = 0; _dtrace_bufpolicies[i].dtbp_name != NULL; i++) {
		if (strcmp(_dtrace_bufpolicies[i].dtbp_name, arg) == 0) {
			policy = _dtrace_bufpolicies[i].dtbp_policy;
			break;
		}
	}

	if (policy == DTRACEOPT_UNSET)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dtp->dt_options[DTRACEOPT_BUFPOLICY] = policy;

	return (0);
}

static const struct {
	const char *dtbr_name;
	int dtbr_policy;
} _dtrace_bufresize[] = {
	{ "auto", DTRACEOPT_BUFRESIZE_AUTO },
	{ "manual", DTRACEOPT_BUFRESIZE_MANUAL },
	{ NULL, 0 }
};

/*ARGSUSED*/
static int
dt_opt_bufresize(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dtrace_optval_t policy = DTRACEOPT_UNSET;
	int i;
	RT_NOREF1(option);

	if (arg == NULL)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	for (i = 0; _dtrace_bufresize[i].dtbr_name != NULL; i++) {
		if (strcmp(_dtrace_bufresize[i].dtbr_name, arg) == 0) {
			policy = _dtrace_bufresize[i].dtbr_policy;
			break;
		}
	}

	if (policy == DTRACEOPT_UNSET)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	dtp->dt_options[DTRACEOPT_BUFRESIZE] = policy;

	return (0);
}

int
dt_options_load(dtrace_hdl_t *dtp)
{
	dof_hdr_t hdr, *dof;
	dof_sec_t *sec VBDTMSC(NULL);
	size_t offs;
	VBDTTYPE(uint32_t,int) i;

	/*
	 * To load the option values, we need to ask the kernel to provide its
	 * DOF, which we'll sift through to look for OPTDESC sections.
	 */
	bzero(&hdr, sizeof (dof_hdr_t));
	hdr.dofh_loadsz = sizeof (dof_hdr_t);

	if (dt_ioctl(dtp, DTRACEIOC_DOFGET, &hdr) == -1)
		return (dt_set_errno(dtp, errno));

	if (hdr.dofh_loadsz < sizeof (dof_hdr_t))
		return (dt_set_errno(dtp, EINVAL));

	dof = alloca(hdr.dofh_loadsz);
	bzero(dof, sizeof (dof_hdr_t));
	dof->dofh_loadsz = hdr.dofh_loadsz;

	for (i = 0; i < DTRACEOPT_MAX; i++)
		dtp->dt_options[i] = (uint64_t)DTRACEOPT_UNSET;

	if (dt_ioctl(dtp, DTRACEIOC_DOFGET, dof) == -1)
		return (dt_set_errno(dtp, errno));

	for (i = 0; i < dof->dofh_secnum; i++) {
		sec = (dof_sec_t *)(uintptr_t)((uintptr_t)dof +
		    dof->dofh_secoff + i * dof->dofh_secsize);

		if (sec->dofs_type != DOF_SECT_OPTDESC)
			continue;

		break;
	}

	for (offs = 0; offs < sec->dofs_size; offs += sec->dofs_entsize) {
		dof_optdesc_t *opt = (dof_optdesc_t *)(uintptr_t)
		    ((uintptr_t)dof + sec->dofs_offset + offs);

		if (opt->dofo_strtab != DOF_SECIDX_NONE)
			continue;

		if (opt->dofo_option >= DTRACEOPT_MAX)
			continue;

		dtp->dt_options[opt->dofo_option] = opt->dofo_value;
	}

	return (0);
}

/*ARGSUSED*/
static int
dt_opt_preallocate(dtrace_hdl_t *dtp, const char *arg, uintptr_t option)
{
	dtrace_optval_t size;
	void *p;
	RT_NOREF1(option);

	if (arg == NULL || dt_optval_parse(arg, &size) != 0)
		return (dt_set_errno(dtp, EDT_BADOPTVAL));

	if (size > SIZE_MAX)
		size = SIZE_MAX;

	if ((p = dt_zalloc(dtp, size)) == NULL) {
		do {
			size /= 2;
		} while ((p = dt_zalloc(dtp, size)) == NULL);
	}

	dt_free(dtp, p);

	return (0);
}

typedef struct dt_option {
	const char *o_name;
	int (*o_func)(dtrace_hdl_t *, const char *, uintptr_t);
	uintptr_t o_option;
} dt_option_t;

/*
 * Compile-time options.
 */
static const dt_option_t _dtrace_ctoptions[] = {
	{ "aggpercpu", dt_opt_agg, DTRACE_A_PERCPU },
	{ "amin", dt_opt_amin },
	{ "argref", dt_opt_cflags, DTRACE_C_ARGREF },
	{ "core", dt_opt_core },
	{ "cpp", dt_opt_cflags, DTRACE_C_CPP },
	{ "cpphdrs", dt_opt_cpp_hdrs },
	{ "cpppath", dt_opt_cpp_path },
	{ "ctypes", dt_opt_ctypes },
	{ "defaultargs", dt_opt_cflags, DTRACE_C_DEFARG },
	{ "dtypes", dt_opt_dtypes },
	{ "debug", dt_opt_debug },
	{ "define", dt_opt_cpp_opts, (uintptr_t)"-D" },
	{ "droptags", dt_opt_droptags },
	{ "empty", dt_opt_cflags, DTRACE_C_EMPTY },
	{ "errtags", dt_opt_cflags, DTRACE_C_ETAGS },
	{ "evaltime", dt_opt_evaltime },
	{ "incdir", dt_opt_cpp_opts, (uintptr_t)"-I" },
	{ "iregs", dt_opt_iregs },
	{ "kdefs", dt_opt_invcflags, DTRACE_C_KNODEF },
	{ "knodefs", dt_opt_cflags, DTRACE_C_KNODEF },
	{ "late", dt_opt_xlate },
	{ "lazyload", dt_opt_lazyload },
	{ "ldpath", dt_opt_ld_path },
	{ "libdir", dt_opt_libdir },
	{ "linkmode", dt_opt_linkmode },
	{ "linktype", dt_opt_linktype },
	{ "nolibs", dt_opt_cflags, DTRACE_C_NOLIBS },
	{ "pgmax", dt_opt_pgmax },
	{ "preallocate", dt_opt_preallocate },
	{ "pspec", dt_opt_cflags, DTRACE_C_PSPEC },
	{ "stdc", dt_opt_stdc },
	{ "strip", dt_opt_dflags, DTRACE_D_STRIP },
	{ "syslibdir", dt_opt_syslibdir },
	{ "tree", dt_opt_tree },
	{ "tregs", dt_opt_tregs },
	{ "udefs", dt_opt_invcflags, DTRACE_C_UNODEF },
	{ "undef", dt_opt_cpp_opts, (uintptr_t)"-U" },
	{ "unodefs", dt_opt_cflags, DTRACE_C_UNODEF },
	{ "verbose", dt_opt_cflags, DTRACE_C_DIFV },
	{ "version", dt_opt_version },
	{ "zdefs", dt_opt_cflags, DTRACE_C_ZDEFS },
	{ NULL }
};

/*
 * Run-time options.
 */
static const dt_option_t _dtrace_rtoptions[] = {
	{ "aggsize", dt_opt_size, DTRACEOPT_AGGSIZE },
	{ "bufsize", dt_opt_size, DTRACEOPT_BUFSIZE },
	{ "bufpolicy", dt_opt_bufpolicy, DTRACEOPT_BUFPOLICY },
	{ "bufresize", dt_opt_bufresize, DTRACEOPT_BUFRESIZE },
	{ "cleanrate", dt_opt_rate, DTRACEOPT_CLEANRATE },
	{ "cpu", dt_opt_runtime, DTRACEOPT_CPU },
	{ "destructive", dt_opt_runtime, DTRACEOPT_DESTRUCTIVE },
	{ "dynvarsize", dt_opt_size, DTRACEOPT_DYNVARSIZE },
	{ "grabanon", dt_opt_runtime, DTRACEOPT_GRABANON },
	{ "jstackframes", dt_opt_runtime, DTRACEOPT_JSTACKFRAMES },
	{ "jstackstrsize", dt_opt_size, DTRACEOPT_JSTACKSTRSIZE },
	{ "nspec", dt_opt_runtime, DTRACEOPT_NSPEC },
	{ "specsize", dt_opt_size, DTRACEOPT_SPECSIZE },
	{ "stackframes", dt_opt_runtime, DTRACEOPT_STACKFRAMES },
	{ "statusrate", dt_opt_rate, DTRACEOPT_STATUSRATE },
	{ "strsize", dt_opt_strsize, DTRACEOPT_STRSIZE },
	{ "ustackframes", dt_opt_runtime, DTRACEOPT_USTACKFRAMES },
	{ NULL }
};

/*
 * Dynamic run-time options.
 */
static const dt_option_t _dtrace_drtoptions[] = {
	{ "aggrate", dt_opt_rate, DTRACEOPT_AGGRATE },
	{ "aggsortkey", dt_opt_runtime, DTRACEOPT_AGGSORTKEY },
	{ "aggsortkeypos", dt_opt_runtime, DTRACEOPT_AGGSORTKEYPOS },
	{ "aggsortpos", dt_opt_runtime, DTRACEOPT_AGGSORTPOS },
	{ "aggsortrev", dt_opt_runtime, DTRACEOPT_AGGSORTREV },
	{ "flowindent", dt_opt_runtime, DTRACEOPT_FLOWINDENT },
	{ "quiet", dt_opt_runtime, DTRACEOPT_QUIET },
	{ "rawbytes", dt_opt_runtime, DTRACEOPT_RAWBYTES },
	{ "stackindent", dt_opt_runtime, DTRACEOPT_STACKINDENT },
	{ "switchrate", dt_opt_rate, DTRACEOPT_SWITCHRATE },
	{ NULL }
};

int
dtrace_getopt(dtrace_hdl_t *dtp, const char *opt, dtrace_optval_t *val)
{
	const dt_option_t *op;

	if (opt == NULL)
		return (dt_set_errno(dtp, EINVAL));

	/*
	 * We only need to search the run-time options -- it's not legal
	 * to get the values of compile-time options.
	 */
	for (op = _dtrace_rtoptions; op->o_name != NULL; op++) {
		if (strcmp(op->o_name, opt) == 0) {
			*val = dtp->dt_options[op->o_option];
			return (0);
		}
	}

	for (op = _dtrace_drtoptions; op->o_name != NULL; op++) {
		if (strcmp(op->o_name, opt) == 0) {
			*val = dtp->dt_options[op->o_option];
			return (0);
		}
	}

	return (dt_set_errno(dtp, EDT_BADOPTNAME));
}

int
dtrace_setopt(dtrace_hdl_t *dtp, const char *opt, const char *val)
{
	const dt_option_t *op;

	if (opt == NULL)
		return (dt_set_errno(dtp, EINVAL));

	for (op = _dtrace_ctoptions; op->o_name != NULL; op++) {
		if (strcmp(op->o_name, opt) == 0)
			return (op->o_func(dtp, val, op->o_option));
	}

	for (op = _dtrace_drtoptions; op->o_name != NULL; op++) {
		if (strcmp(op->o_name, opt) == 0)
			return (op->o_func(dtp, val, op->o_option));
	}

	for (op = _dtrace_rtoptions; op->o_name != NULL; op++) {
		if (strcmp(op->o_name, opt) == 0) {
			/*
			 * Only dynamic run-time options may be set while
			 * tracing is active.
			 */
			if (dtp->dt_active)
				return (dt_set_errno(dtp, EDT_ACTIVE));

			return (op->o_func(dtp, val, op->o_option));
		}
	}

	return (dt_set_errno(dtp, EDT_BADOPTNAME));
}
