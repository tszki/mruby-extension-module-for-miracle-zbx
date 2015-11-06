/*
** Takanori Suzuki <mail.tks@gmail.com>
** Copyright (C) 2001-2014
**
** derived from Zabbix SIA's work under GPL2 or later.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
** MA 02110-1301, USA.
**/

#include <sysinc.h>
#include <module.h>
#include <common.h>
#include <log.h>
#include <mruby.h>
#include <mruby/compile.h>
#include <mruby/string.h>

mrb_value mrb_get_backtrace(mrb_state *mrb, mrb_value self);

extern char *CONFIG_LOAD_MODULE_PATH;

typedef struct
{
	char		*path;
	mrb_state	*mrb;
	mrb_value	instance;
	mrbc_context	*context;
	int		ai;
}
MRB_WITH_PATH;

MRB_WITH_PATH	*mrb_list = NULL;
int		mrb_list_len = 0;

/* the variable keeps timeout setting for item processing */
static int item_timeout = 0;

int zbx_module_mruby_module(AGENT_REQUEST *request, AGENT_RESULT *result);
int zbx_module_mruby_eval(AGENT_REQUEST *request, AGENT_RESULT *result);

static ZBX_METRIC keys[] =
/* KEY FLAG FUNCTION TEST PARAMETERS */
{
	{"mruby.module", CF_HAVEPARAMS, zbx_module_mruby_module, NULL},
	{"mruby.eval", CF_HAVEPARAMS, zbx_module_mruby_eval, NULL},
	{NULL}
};

#define MRUBY_MODULE_SUBDIRECTORY "mruby_module"

/******************************************************************************
*                                                                             *
* Function: zbx_module_api_version                                            *
*                                                                             *
* Purpose: returns version number of the module interface                     *
*                                                                             *
* Return value: ZBX_MODULE_API_VERSION_ONE - the only version supported by    *
* Zabbix currently                                                            *
*                                                                             *
******************************************************************************/
int zbx_module_api_version()
{
	return ZBX_MODULE_API_VERSION_ONE;
}

/******************************************************************************
*                                                                             *
* Function: zbx_module_item_timeout                                           *
*                                                                             *
* Purpose: set timeout value for processing of items                          *
*                                                                             *
* Parameters: timeout - timeout in seconds, 0 - no timeout set                *
*                                                                             *
******************************************************************************/
void zbx_module_item_timeout(int timeout)
{
	item_timeout = timeout;
}

/******************************************************************************
*                                                                             *
* Function: zbx_module_item_list                                              *
*                                                                             *
* Purpose: returns list of item keys supported by the module                  *
*                                                                             *
* Return value: list of item keys                                             *
*                                                                             *
******************************************************************************/
ZBX_METRIC *zbx_module_item_list()
{
	return keys;
}

/******************************************************************************
*                                                                             *
* Function: zbx_module_mruby_module                                           *
*                                                                             *
* Purpose: function for "mruby.module" key                                    *
*                                                                             *
* Return value: SYSINFO_RET_OK                                                *
*                                                                             *
******************************************************************************/
int zbx_module_mruby_module(AGENT_REQUEST *request, AGENT_RESULT *result)
{
#define PARAM_LEN 10
	char		*param[PARAM_LEN];
	mrb_value	mrb_func_param[PARAM_LEN - 1];
	int		mrb_func_param_len = 0;
	mrb_value	ret;
	char		mruby_file_path[MAX_BUFFER_LEN];
	int		i = 0;
	int		j = 0;
	int		find = 0;

	for (i = 0; i < PARAM_LEN; i++)
	{
		param[i] = get_rparam(request, i);
	}
	zabbix_log(LOG_LEVEL_DEBUG, "get_rparams_num %d", get_rparams_num(request));
	zbx_snprintf(mruby_file_path, sizeof(mruby_file_path), "%s/%s/%s", CONFIG_LOAD_MODULE_PATH, MRUBY_MODULE_SUBDIRECTORY, param[0]);

	for (i = 0; i < mrb_list_len; i++)
	{
		if (!strcmp(mruby_file_path, mrb_list[i].path))
		{
			find = 1;
			break;
		}
	}

	if (find)
	{
		mrb_func_param_len = get_rparams_num(request) - 1;
		if (mrb_func_param_len > PARAM_LEN - 1)
			mrb_func_param_len = PARAM_LEN - 1;
		for (j = 0; j < mrb_func_param_len; j++)
		{
			mrb_func_param[j] = mrb_str_new_cstr(mrb_list[i].mrb, param[j + 1]);
		}
		ret = mrb_funcall_argv(mrb_list[i].mrb, mrb_list[i].instance, mrb_intern_cstr(mrb_list[i].mrb, "zbx_module_run"), mrb_func_param_len, mrb_func_param);
		/* ret should be zabbix compatible type, like string, int, float  */
		if (mrb_list[i].mrb->exc){
			mrb_value exc = mrb_obj_value(mrb_list[i].mrb->exc);
			mrb_value backtrace = mrb_get_backtrace(mrb_list[i].mrb, exc);
			zabbix_log(LOG_LEVEL_ERR, "mruby_module: %s", mrb_str_to_cstr(mrb_list[i].mrb, mrb_inspect(mrb_list[i].mrb, backtrace)));
			mrb_value inspect = mrb_inspect(mrb_list[i].mrb, exc);
			zabbix_log(LOG_LEVEL_ERR, "mruby_module backtrace: %s", mrb_str_to_cstr(mrb_list[i].mrb, inspect));
			mrb_list[i].mrb->exc = 0;
			SET_TEXT_RESULT(result, strdup(""));
		}
		else if (MRB_TT_STRING == ret.tt)
			SET_TEXT_RESULT(result, strdup(RSTRING_PTR(ret)));
		else if (MRB_TT_FLOAT == ret.tt)
			SET_DBL_RESULT(result, mrb_float(ret));
		else if (MRB_TT_FIXNUM == ret.tt)
			SET_DBL_RESULT(result, mrb_fixnum(ret));
		else
			SET_TEXT_RESULT(result, strdup(""));
	}
	else
	{
		zabbix_log(LOG_LEVEL_ERR, "mruby_module: no muching mruby file");
		SET_TEXT_RESULT(result, strdup(""));
	}

	return SYSINFO_RET_OK;
}

/******************************************************************************
*                                                                             *
* Function: zbx_module_mruby_eval                                             *
*                                                                             *
* Purpose: function for "mruby.eval" key                                      *
*                                                                             *
* Return value: SYSINFO_RET_OK                                                *
*                                                                             *
******************************************************************************/
int zbx_module_mruby_eval(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	char		*param1 = NULL;
	mrb_state	*mrb = mrb_open();
	mrb_value	ret;
	mrbc_context	*context = mrbc_context_new(mrb);
	int		ai;

	param1 = get_rparam(request, 0);
	ret = mrb_load_string_cxt(mrb, param1, context);
	ai = mrb_gc_arena_save(mrb);
	/* ret should be zabbix compatible type, like string, int, float  */
	if (mrb->exc){
		mrb_value exc = mrb_obj_value(mrb->exc);
		mrb_value backtrace = mrb_get_backtrace(mrb, exc);
		zabbix_log(LOG_LEVEL_ERR, "mruby_module: %s", mrb_str_to_cstr(mrb, mrb_inspect(mrb, backtrace)));
		mrb_value inspect = mrb_inspect(mrb, exc);
		zabbix_log(LOG_LEVEL_ERR, "mruby_module backtrace: %s", mrb_str_to_cstr(mrb, inspect));
		mrb->exc = 0;
		SET_TEXT_RESULT(result, strdup(""));
	}
	else if (MRB_TT_STRING == ret.tt)
		SET_TEXT_RESULT(result, strdup(RSTRING_PTR(ret)));
	else if (MRB_TT_FLOAT == ret.tt)
		SET_DBL_RESULT(result, mrb_float(ret));
	else if (MRB_TT_FIXNUM == ret.tt)
		SET_DBL_RESULT(result, mrb_fixnum(ret));
	else
		SET_TEXT_RESULT(result, strdup(""));
	mrb_gc_arena_restore(mrb, ai);
	mrbc_context_free(mrb, context);
	mrb_close(mrb);

	return SYSINFO_RET_OK;
}

/******************************************************************************
*                                                                             *
* Function: search_and_load_mruby_files                                       *
*                                                                             *
* Purpose: load "*.rb" files in CONFIG_LOAD_MODULE_PATH + "/mruby_module"     *
*                                                                             *
******************************************************************************/
void search_and_load_mruby_files()
{
	char mruby_dir_path[MAX_BUFFER_LEN];
	char		mruby_file_path[MAX_BUFFER_LEN];
	struct dirent	*dir_entry = NULL;
	DIR		*dir = NULL;
	FILE		*f = NULL;
	int 		i = 0;

	zbx_snprintf(mruby_dir_path, sizeof(mruby_dir_path), "%s/%s", CONFIG_LOAD_MODULE_PATH, MRUBY_MODULE_SUBDIRECTORY);

	if (NULL == (dir = opendir(mruby_dir_path)))
		return;

	for (i = 0; NULL != (dir_entry = readdir(dir)); i++){
		if (!strcmp(&(dir_entry->d_name[strlen(dir_entry->d_name)-3]),".rb"))
		{
			mrb_list_len++;
			if (NULL == mrb_list)
				mrb_list = zbx_malloc(mrb_list, sizeof(MRB_WITH_PATH) * mrb_list_len);
			else
				mrb_list = zbx_realloc(mrb_list, sizeof(MRB_WITH_PATH) * mrb_list_len);

			zbx_snprintf(mruby_file_path, sizeof(mruby_file_path), "%s/%s", mruby_dir_path, dir_entry->d_name);
			mrb_list[mrb_list_len - 1].path = zbx_strdup(NULL, mruby_file_path);
			mrb_list[mrb_list_len - 1].mrb = mrb_open();
			mrb_list[mrb_list_len - 1].context = mrbc_context_new(mrb_list[mrb_list_len - 1].mrb);
			mrb_list[mrb_list_len - 1].ai = mrb_gc_arena_save(mrb_list[mrb_list_len - 1].mrb);
			f = fopen(mruby_file_path, "r");
			if(f)
			{
				mrb_load_file(mrb_list[mrb_list_len - 1].mrb, f);
				fclose(f);
			}
			mrb_list[mrb_list_len - 1].instance = mrb_class_new_instance(mrb_list[mrb_list_len - 1].mrb, 0, NULL, mrb_class_get(mrb_list[mrb_list_len - 1].mrb, "MonitoringModule"));
		}
	}
	closedir(dir);
	return;
}

/******************************************************************************
*                                                                             *
* Function: exec_mruby_function                                               *
*                                                                             *
* Purpose: execute "function" in loaded "*.rb" files                          *
*                                                                             *
******************************************************************************/
void exec_mruby_function(char *function)
{
	int i;
	mrb_value	ret;

	for (i = 0; i < mrb_list_len; i++)
	{
		mrb_funcall(mrb_list[i].mrb, mrb_list[i].instance, function, 0);
		if (mrb_list[i].mrb->exc){
			mrb_value exc = mrb_obj_value(mrb_list[i].mrb->exc);
			mrb_value backtrace = mrb_get_backtrace(mrb_list[i].mrb, exc);
			zabbix_log(LOG_LEVEL_ERR, "mruby_module: %s", mrb_str_to_cstr(mrb_list[i].mrb, mrb_inspect(mrb_list[i].mrb, backtrace)));
			mrb_value inspect = mrb_inspect(mrb_list[i].mrb, exc);
			zabbix_log(LOG_LEVEL_ERR, "mruby_module backtrace: %s", mrb_str_to_cstr(mrb_list[i].mrb, inspect));
			mrb_list[i].mrb->exc = 0;
		}
	}

	return;
}

/******************************************************************************
*                                                                             *
* Function: zbx_module_init                                                   *
*                                                                             *
* Purpose: the function is called on agent startup                            *
* It should be used to call any initialization routines                       *
*                                                                             *
* Return value: ZBX_MODULE_OK - success                                       *
* ZBX_MODULE_FAIL - module initialization failed                              *
*                                                                             *
* Comment: the module won't be loaded in case of ZBX_MODULE_FAIL              *
*                                                                             *
******************************************************************************/
int zbx_module_init()
{
	search_and_load_mruby_files();
	exec_mruby_function("zbx_module_init");
	return ZBX_MODULE_OK;
}

/******************************************************************************
*                                                                             *
* Function: zbx_module_uninit                                                 *
*                                                                             *
* Purpose: the function is called on agent shutdown                           *
* It should be used to cleanup used resources if there are any                *
*                                                                             *
* Return value: ZBX_MODULE_OK - success                                       *
* ZBX_MODULE_FAIL - function failed                                           *
*                                                                             *
******************************************************************************/
int zbx_module_uninit()
{
	int i;

	exec_mruby_function("zbx_module_uninit");

	for(i = 0; i < mrb_list_len; i++)
	{
		mrb_gc_arena_restore(mrb_list[i].mrb, mrb_list[i].ai);
		mrbc_context_free(mrb_list[i].mrb, mrb_list[i].context);
		zbx_free(mrb_list[i].path);
		mrb_close(mrb_list[i].mrb);
	}
	zbx_free(mrb_list);

	return ZBX_MODULE_OK;
}
