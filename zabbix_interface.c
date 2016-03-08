/*
 * Функции взаимодействия с zabbix-агентом.
 *
 * Copyright (C) 2016  Oleg Bobukh <o.bobukh@yandex.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */

#include <sys/types.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "pid_info.h"
#include <module.h>
#include <sysinc.h>

int zbx_module_api_version();
int zbx_module_init(void);
ZBX_METRIC *zbx_module_item_list(void);
int zbx_module_uninit();

int zbx_proc_summ(AGENT_REQUEST *request, AGENT_RESULT *result, int mode);
int zbx_proc_vmrss(AGENT_REQUEST *request, AGENT_RESULT *result);
int zbx_proc_map_all(AGENT_REQUEST *request, AGENT_RESULT *result);
int zbx_proc_map_rw(AGENT_REQUEST *request, AGENT_RESULT *result);
int zbx_proc_map_shared(AGENT_REQUEST *request, AGENT_RESULT *result);

/* Поддерживаемые метрики */
static ZBX_METRIC keys[] =
	/* KEY               FLAG           FUNCTION                TEST PARAMETERS */{
	{"procinf.vmrss", CF_HAVEPARAMS, zbx_proc_vmrss, "bash"},
	{"procinf.allmap", CF_HAVEPARAMS, zbx_proc_map_all, "bash"},
	{"procinf.rwmap", CF_HAVEPARAMS, zbx_proc_map_rw, "bash"},
	{"procinf.shmap", CF_HAVEPARAMS, zbx_proc_map_shared, "bash"},
	{NULL}
};

/**
 * Обязательная функция модуля Zabbix.
 * Возвращает используемую версию api модуля.
 * @return версия api.
 */
int zbx_module_api_version()
{
	return ZBX_MODULE_API_VERSION_ONE;
}

//------------------------------------------------------------------------------

/**
 * Функция, вызов которой должен инициализировать
 * этот модуль. Инициализировать нечего. Возвращаем OK.
 * @return OK
 */
int zbx_module_init(void)
{
	return ZBX_MODULE_OK;
}

//------------------------------------------------------------------------------

/**
 * Возвращает список поддерживаемых элементов данных/метрик.
 * @return	список функций по сбору метрик
 */
ZBX_METRIC *zbx_module_item_list(void)
{
	return keys;
}

//------------------------------------------------------------------------------

/**
 * Деинициализация. Ничего деинициализировать не нужно, просто возвращаем ОК
 * @return OK
 */
int zbx_module_uninit()
{
	return ZBX_MODULE_OK;
}

//------------------------------------------------------------------------------

/**
 * Возвращает какой-либо параметр для процесса
 * @param request	запрос агента
 * @param result	ответ агенту
 * @return		результат обработки запроса
 */
int zbx_proc_summ(AGENT_REQUEST *request, AGENT_RESULT *result, int mode)
{
	unsigned long value;
	char *proc_name, *user_name;
	switch (request->nparam) {
	case 1:
		proc_name = get_rparam(request, 0);
		value = get_proc_value_summ(proc_name, NULL, mode);
		SET_UI64_RESULT(result, value);
		return SYSINFO_RET_OK;
		break;
	case 2:
		proc_name = get_rparam(request, 0);
		user_name = get_rparam(request, 1);
		value = get_proc_value_summ(proc_name, user_name, mode);
		SET_UI64_RESULT(result, value);
		return SYSINFO_RET_OK;
		break;
	default:
		SET_MSG_RESULT(result, strdup("You must set one or two parameters."));
		return SYSINFO_RET_FAIL;
	}
}

//------------------------------------------------------------------------------

/**
 * Возвращает сумму значений размера резидентной памяти для всех одноимённых
 * процессов.
 *
 * @param request	запрос агента
 * @param result	ответ агенту
 * @return 		результат обработки запроса
 */
int zbx_proc_vmrss(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	return zbx_proc_summ(request, result, PROC_VMRSS);
}

//------------------------------------------------------------------------------

/**
 * Возвращает сумму значений областей памяти для всех одноимённых процессов.
 * @param request	запрос агента
 * @param result	ответ агенту
 * @return 		результат обработки запроса
 */
int zbx_proc_map_all(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	return zbx_proc_summ(request, result, PROC_MAP);
}

//------------------------------------------------------------------------------

/**
 * Возвращает сумму значений областей памяти для всех одноимённых процессов.
 * Суммируются только области, куда данные агенты могут производить запись
 * (и чтение)
 * @param request	запрос агента
 * @param result	ответ агенту
 * @return 		результат обработки запроса
 */
int zbx_proc_map_rw(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	return zbx_proc_summ(request, result, PROC_MAP_RW);
}

//------------------------------------------------------------------------------

/**
 * Возвращает сумму значений областей памяти для всех одноимённых процессов.
 * Суммируются только разделяемые области памяти данных процессов.
 * @param request	запрос агента
 * @param result	ответ агенту
 * @return 		результат обработки запроса
 */
int zbx_proc_map_shared(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	return zbx_proc_summ(request, result, PROC_MAP_SHARED);
}
