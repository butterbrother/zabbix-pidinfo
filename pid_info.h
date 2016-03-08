/*
 * Различные функции, показывающие групповую информацию по одноимённым
 * процессам. Т.е. данные по ним будут суммироваться.
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

#ifndef PID_INFO_H
#define PID_INFO_H

#ifdef __cplusplus
extern "C" {
#endif

	extern enum proc_params /* параметры, которые можно просчитывать при вызове
			 * get_proc_value_summ */ {
		PROC_VMRSS, /* подсчёт резидентной памяти */
		PROC_MAP, /* подсчёт маппинга, целиком */
		PROC_MAP_SHARED, /* подсчёт маппинга, только shared-области */
		PROC_MAP_RW /* подсчёт маппинга, только rw-области */
	};

	/**
	 * Просчитывает сумму значений параметра одноимённых процессов.
	 * Для Unix-систем.
	 * Сейчас поддерживается:
	 * - Linux, Cygwin-Windows, Solaris (VmRSS, подсчёт размера областей памяти)
	 * // дополнять список по мере разработки
	 *
	 * @param proc_name	имя процесса
	 * @param user_name	имя пользователя, может быть NULL.
	 *                      Если указанный пользователь не будет найден, то
	 *                      функция вернёт 0. Если будет указан NULL, то фильтрации
	 *                      по имени пользователя производиться не будет.
	 * @param param		рассчитываемый параметр.
	 * 			Берётся из proc_params
	 * @return 		значений параметра одноимённого процесса.
	 */
	extern unsigned long get_proc_value_summ(char *proc_name, char *user_name, int param);

#ifdef __cplusplus
}
#endif

#endif /* PID_INFO_H */

