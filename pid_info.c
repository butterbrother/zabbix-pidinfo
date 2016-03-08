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
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "string_util.h"
#include <string.h>
#include <pwd.h>
#include <limits.h>
#include <ctype.h>
// solaris
#if defined(__sun) && defined(__SVR4)
#include <procfs.h>
#endif

#define DEBUG   0 // Режим отладки.
#define NBUF_SIZE 16384 // Размер буфера чтения из файла
#define NLINE_SIZE 1024 // Размер строки при чтении из файла

enum proc_params /* параметры, которые можно просчитывать при вызове
			 * get_proc_value_summ */ {
	PROC_VMRSS, /* подсчёт резидентной памяти */
	PROC_MAP, /* подсчёт маппинга, целиком */
	PROC_MAP_SHARED, /* подсчёт маппинга, только shared-области */
	PROC_MAP_RW /* подсчёт маппинга, только rw-области */
};

typedef struct linux_stat_s {
	int pid; /* (1) %d ID процесса. */
	char comm[256]; /* (2) %s Имя исполнимого файла */
	char state; /* (3) %c Состояние процесса */
	int ppid; /* (4) %d PID родительского процесса */
	int pgrp; /* (5) %d ID группы процесса */
	int session; /* (6) %d ID сессии процесса */
	int tty_nr; /* (7) %d Номер терминала (tty),
				 * контроллирующего процесс */
	int tpgid; /* (8) %d ID группы приоритетного процесса
				 *  терминала управления процессов */
	unsigned flags; /* (9) %u Флаги ядра процесса */
	unsigned long minflt; /* (10) %lu Число незначительных отказов,
				 * произведённых процессом, которые не
				 * потребовали загрузки страницы памяти с
				 * диска. */
	unsigned long cminflt; /* (11) %lu Число незначительных отказов,
				 * которые произошли при ожидании выполнения
				 * действий дочерних процессов */
	unsigned long majflt; /* (12) %lu Число значительных отказов,
				 * проиведённых процессом, которые
				 * потребовали загрузки страниц памяти с
				 * диска */
	unsigned long cmajflt; /* (13) %lu Число значительных отказов,
				 * которые произошли при ожидании выполнения
				 * действий дочерних процессов */
	unsigned long utime; /* (14) %lu Время выполнения процесса в
				 * пространстве пользователя
				 * (непривелегированный режим). Измеряется
				 * в тактах */
	unsigned long stime; /* (15) %lu Время выполнения процесса в
				 * пространстве ядра. Измеряется в тактах */
	long cutime; /* (16) %ld Время ожидания исполнения действий
				 * доверних процессов в пространсве пользователя.
				 * Измеряется в тактах */
	long cstime; /* (17) %ld Время ожидания исполнения действия
				 * дочерних процессов в пространстве ядра.
				 * Измеряется в тактах */
	long priority; /* (18) %ld Приоритет процесса. Измеряется
				 * по-разному, в зависимости от параметров
				 * планировщика процессов */
	long nice; /* (19) %ld Приоритет процесса */
	long num_threads; /* (20) %ld Число потоков процесса
				 * (подпроцессов) */
	long itrealvalue; /* (21) %ld Не используется, здесь всегда 0 */
	unsigned long long starttime; /* (22) %llu Время старта процесса после
				   * загрузки системы. Измеряется в тактах */
	unsigned long vsize; /* (23) %lu Размер виртуальной памяти, в
				 * байтах. */
	long rss; /* (24) %ld Расход резидентной памяти. Показывает,
				 * сколько процесс использует реальной физической
				 * памяти за вычетом свопа и выгруженных данных */
} linux_stat_t;

/* Права-флаги региона памяти процесса linux */
typedef struct linux_maps_perms {
	unsigned read; /* r - чтение */
	unsigned write; /* w - запись */
	unsigned executable; /* e - исполнение */
	unsigned shared; /* s - разделяемая память */
	unsigned private; /* p - приватная, копирование при записи */
} linux_maps_perms_t;

static char path_separator[] = "/"; // Разделитель каталогов
static char proc_path[] = "/proc"; // Путь к /proc

unsigned long get_proc_value_summ(char *proc_name, char *user_name, int param);
int is_valid_dir(struct dirent *dir_entry, int uid_filter, long uid);
int use_filter(char *user_name, long *uid);
unsigned long get_vmrss_linux(char *pid_dir, char *fbuf, char *proc_name);
linux_stat_t *read_linux_stat(char *pid_dir, char *fbuf);
int is_valid_linux_proc(char *pid_dir, char *proc_name, char *fbuf);
unsigned long calc_linux_proc_map(char *pid_dir, char *proc_name, char* fbuf, int mode);
linux_maps_perms_t *parse_linux_perms(const char *str_perms);
unsigned long htol(const char *hex);

#if defined(__sun) && defined(__SVR4)
unsigned long get_vmrss_solaris(char *pid_dir, char *fbuf, char *proc_name);
psinfo_t *read_solaris_psinfo(char *pid_dir, char *fbuf);
unsigned long calc_solaris_proc_map(char *pid_dir, char *proc_name, char* fbuf, int mode);
int is_valid_solaris_proc(char *pid_dir, char *proc_name, char *fbuf);
#endif

/*
int main()
{
	printf("bash: %lu\n", get_proc_value_summ("java", "tomcat", PROC_VMRSS));
	return 0;
}
 */

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
unsigned long get_proc_value_summ(char *proc_name, char *user_name, int param)
{
	// Определяем параметры и необходимость фильтрации по uid
	long uid;
	int uid_filtering = use_filter(user_name, &uid);

	if (uid_filtering < 0)
		return 0;

	DIR *directory;
	struct dirent *direntry;

	directory = opendir(proc_path);
	if (directory == NULL) {
		return 0;
	}

	unsigned long result = 0;
	char *fbuf = malloc(sizeof(char) * NBUF_SIZE);

	// Обрабатываем список pid-каталогов в /proc
	while ((direntry = readdir(directory))) {
		if (strcmp(".", direntry->d_name) == 0 || strcmp("..", direntry->d_name) == 0)
			continue;

		if (is_valid_dir(direntry, uid_filtering, uid)) {
			switch (param) {
			case PROC_VMRSS:
#if defined(__linux__) || (defined(__CYGWIN__) && !defined(_WIN32))
				// реализация для linux и cygwin в режиме cygwin
				result += get_vmrss_linux(direntry->d_name, fbuf, proc_name);
#endif
#if defined(__sun) && defined(__SVR4)
				// реализация для solaris и opensolaris/openindiana
				result += get_vmrss_solaris(direntry->d_name, fbuf, proc_name);
#endif
				break;
			case PROC_MAP:
			case PROC_MAP_SHARED:
			case PROC_MAP_RW:
#if defined(__linux__) || (defined(__CYGWIN__) && !defined(_WIN32))
				result += calc_linux_proc_map(direntry->d_name, proc_name, fbuf, param);
#endif
#if defined(__sun) && defined(__SVR4)
				result += calc_solaris_proc_map(direntry->d_name, proc_name, fbuf, param);
#endif
				break;
			}
		}
	}

	free(fbuf);
	closedir(directory);

	return result;
}

//------------------------------------------------------------------------------

/**
 * Определение, является ли элемент директории поддиректорией.
 * Фильтрует директорию по uid владельца, если необходимо.
 *
 * @param dir_entry	подэлемент каталога
 * @param uid_filter	фильтрация по uid. 1 - включено. 0 - нет
 * @param uid		UID пользователя.
 * @return		1 - если это подходящая поддиректория.
 * 			0 - если иначе
 */
int is_valid_dir(struct dirent *dir_entry, int uid_filter, long uid)
{
#if DEBUG
	printf("DEBUG: check is valid dir %s: ", dir_entry->d_name);
#endif
	struct stat status;
	char *fname = str_summ("/proc/", dir_entry->d_name);
#if DEBUG
	printf("file name is [%s], ", fname);
#endif

	if (stat(fname, &status) >= 0) {
#if DEBUG
		printf("stat() is ok\n");
#endif
		free(fname);

		if (uid_filter)
			return S_ISDIR(status.st_mode) && status.st_uid == uid;
		else
			return S_ISDIR(status.st_mode);
	} else {
#if DEBUG
		printf("stat() is not ok.\n");
#endif
		free(fname);
		return 0;
	}
}

//------------------------------------------------------------------------------

/**
 * Определение необходимости фильтрации процессов по uid
 *
 * @param user_name	имя пользователя. Может быть NULL
 * @param uid		uid пользователя. Сюда будет записан
 * 			реальный uid, если он будет удачно определён.
 * @return 		1 - необходимо, 0 - нет необходимости. -1 - ошибка
 *
 */
int use_filter(char *user_name, long *uid)
{
	int uid_filtering = user_name == NULL ? 0 : 1;
	struct passwd *user;

	if (uid_filtering) {
		user = getpwnam(user_name);
		if (user != NULL) {
			*uid = user -> pw_uid;
		} else {
			uid_filtering = -1;
		}
	} else
		*uid = 0;

	return uid_filtering;
}

//------------------------------------------------------------------------------

/**
 * Сбор значения vmrss для linux-систем
 * @param pid_dir
 * @param fbuf
 * @param proc_name
 * @return
 */
unsigned long get_vmrss_linux(char *pid_dir, char *fbuf, char *proc_name)
{
#if DEBUG
	printf("DEBUG: get vmrss from %s using pid dir %s\n", proc_name, pid_dir);
#endif
	unsigned long result = 0;

	linux_stat_t *stat = read_linux_stat(pid_dir, fbuf);
	if (stat != NULL) {
#if DEBUG
		printf("DEBUG: getting process status is ok, status proc name is [%s]\n", stat->comm);
#endif
		if (strcmp(stat->comm, proc_name) == 0) {
#if DEBUG
			printf("DEBUG: rss pages cound: %ld\n", stat->rss);
			printf("DEBUG: _SC_PAGESIZE is: %lu\n", sysconf(_SC_PAGESIZE));
#endif
			result = (unsigned long) stat->rss * (unsigned long) sysconf(_SC_PAGESIZE);
#if defined(__CYGWIN__) && !defined(_WIN32)
			// У cygwin pagesize == 64k, для корректного рассчёта map.
			// Что не подходит в нашем случае
			result = (unsigned long) stat->rss * 4096;
#endif
		}

		free(stat);
	}

	return result;
}

//------------------------------------------------------------------------------

/**
 * Считывает stat-файл процесса. Для linux-систем
 *
 * @param pid_dir	PID процесса, имя каталога в /proc
 * @param fbuf		Файловый буфер
 * @return 		Указатель на stat при успешном чтении.
 * 			NULL - при неудачном.
 */
linux_stat_t *read_linux_stat(char *pid_dir, char *fbuf)
{
	static char linux_stat_path[] = "stat";
	static char linux_stat_fmt[] =
		"%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu %ld %ld %ld %ld %ld %ld %llu %lu %ld ";

	// Открываем файл
	char *stat_path = str_builder(5,
		proc_path, path_separator, pid_dir, path_separator, linux_stat_path);
	FILE *stat_file = fopen(stat_path, "rt");
	free(stat_path);

	if (stat_file == NULL)
		return NULL;

	setvbuf(stat_file, fbuf, _IOFBF, NBUF_SIZE);

	linux_stat_t *stat = calloc(1, sizeof(linux_stat_t));

	int result = fscanf(stat_file, linux_stat_fmt,
		&stat->pid, stat->comm, &stat->state, &stat->ppid,
		&stat->pgrp, &stat->session, &stat->tty_nr, &stat->tpgid,
		&stat->flags, &stat->minflt, &stat->cminflt, &stat->majflt,
		&stat->cmajflt, &stat->utime, &stat->stime, &stat->cutime,
		&stat->cstime, &stat->priority, &stat->nice, &stat->num_threads,
		&stat->itrealvalue, &stat->starttime, &stat->vsize, &stat->rss);

	fclose(stat_file);

	if (result >= 2) {
		// Удаляем открывающую скобку в имени процесса
		left_shift(stat->comm);
		// и закрывающую скобку
		(stat->comm)[strlen(stat->comm) - 1] = '\0';
#if DEBUG
		printf("DEBUG: readed stat pid: %d\n", stat->pid);
		printf("DEBUG: readed stat proc name: [%s]\n", stat->comm);
#endif
		return stat;
	}

	free(stat);
	return NULL;
}

//------------------------------------------------------------------------------

/**
 * Определение, соответствует ли каталог PID в /proc имени процесса.
 * Для linux - систем.
 *
 * @param pid_dir	PID-каталог в /proc
 * @param proc_name	Имя процесса
 * @param fbuf		Файловый буфер чтения
 * @return 		1 - PID-каталог в /proc соответствует имени процессе.
 * 			0 - не соответствует.
 */
int is_valid_linux_proc(char *pid_dir, char *proc_name, char *fbuf)
{
	int valid = 0;

	linux_stat_t *stat = read_linux_stat(pid_dir, fbuf);
	if (stat != NULL) {
		if (strcmp(stat->comm, proc_name) == 0)
			valid = 1;

		free(stat);
	}

	return valid;
}

//------------------------------------------------------------------------------

/**
 * Суммирует размер областей памяти процесса linux.
 * @param pid_dir	PID-каталог процесса в /proc
 * @param proc_name	Имя процесса.
 * @param fbuf		Файловый буфер
 * @param mode		Режим сбора.
 * @return		Если имя процесса соответствует pid, то вернётся сумма
 * 			областей памяти данного процесса. Иначе возвращается 0.
 */
unsigned long calc_linux_proc_map(char *pid_dir, char *proc_name, char* fbuf, int mode)
{
	if (!is_valid_linux_proc(pid_dir, proc_name, fbuf))
		return 0;

	static char maps_file_name[] = "maps";

	char *maps_path = str_builder(5,
		proc_path, path_separator, pid_dir, path_separator, maps_file_name);
	FILE *maps_file = fopen(maps_path, "rt");
	free(maps_path);

	if (maps_file == NULL)
		return 0;

	unsigned long result = 0;
	setvbuf(maps_file, fbuf, _IOFBF, NBUF_SIZE);
	char *lbuf = malloc(sizeof(char) * NLINE_SIZE);

	char *sbegin_addr, *send_addr, *perms;
	unsigned long begin, end;
	linux_maps_perms_t *flags;
	while (read_line(maps_file, lbuf, NLINE_SIZE)) {
		// Разбиваем строку
		sbegin_addr = strtok(lbuf, "-");
		if (sbegin_addr == NULL) continue;
		begin = htol(sbegin_addr);

		send_addr = strtok(NULL, " ");
		if (send_addr == NULL) continue;
		end = htol(send_addr);

		perms = strtok(NULL, " ");
		if (perms == NULL) continue;
		flags = parse_linux_perms(perms);
		if (flags == NULL) continue;

#if DEBUG
		printf("DEBUG: raw perms: %s\n", perms);
		printf("DEBUG: parsed perms: r:%u, w:%u, x:%u, s:%u, p:%u\n",
			flags->read, flags->write, flags->executable,
			flags->shared, flags->private);
#endif

		if (mode == PROC_MAP)
			result += end - begin;
		else if (mode == PROC_MAP_RW && flags->read && flags->write)
			result += end - begin;
		else if (mode == PROC_MAP_SHARED && flags->shared)
			result += end - begin;

		free(flags);
	}

	free(lbuf);
	fclose(maps_file);

	return result;
}

//------------------------------------------------------------------------------

/**
 * Преобразует флаги-разрешения процесса linux в структуру linux_maps_perms
 * @param str_perms	подстрока с флагами из /proc/pid/maps
 * @return 		указатель на стрктуру с флагами в случае удачного чтения.
 * 			NULL - в случае неудачного разбора
 */
linux_maps_perms_t *parse_linux_perms(const char *str_perms)
{
	if (substr_len(str_perms) == 0)
		return NULL;
	int i = 0;

	linux_maps_perms_t *perms = (linux_maps_perms_t *) calloc(1, sizeof(linux_maps_perms_t));

	while (str_perms[i] != '\0') {
		switch (str_perms[i]) {
		case 'r':
			perms->read = 1;
			break;
		case 'w':
			perms->write = 1;
			break;
		case 'e':
			perms->executable = 1;
			break;
		case 's':
			perms->shared = 1;
			break;
		case 'p':
			perms->private = 1;
		}

		++i;
	}

	return perms;
}

//------------------------------------------------------------------------------

/**
 * Конвертирование hex-строки в unsigned long
 * @param hex	hex-строка
 * @return 	результат конвертации.
 * 		Неудачная конвертация вернёт 0
 */
unsigned long htol(const char *hex)
{
#if DEBUG
	printf("DEBUG: htol string: [%s]\n", hex);
#endif
	int i = 0, n;
	// Пропускаем первые 0x, если они есть
	if (substr_len(hex) >= 2 && hex[0] == '0')
		if (tolower(hex[1]) == 'x') {
			i = 2;
		}

	unsigned long result = 0;
	// Конвертируем
	while (hex[i] != '\0') {
#if DEBUG
		printf("[%c|", hex[i]);
#endif
		if (isdigit(hex[i]))
			n = hex[i] - '0';
		else if (isalpha(hex[i]) && tolower(hex[i]) >= 'a' && tolower(hex[i]) <= 'h')
			n = tolower(hex[i]) - 'a' + 10;
		else
			return 0;

		result = result * 16 + n;
#if DEBUG
		printf("%i:%lu]", n, result);
#endif
		++i;
	}

#if DEBUG
	printf("\nDEBUG: htol result: %lu\n", result);
#endif

	return result;
}

//------------------------------------------------------------------------------

#if defined(__sun) && defined(__SVR4)

/**
 * Получение значения VmRSS для процесса solaris-систем.
 * @param pid_dir
 * @param fbuf
 * @param proc_name
 * @return
 */
unsigned long get_vmrss_solaris(char *pid_dir, char *fbuf, char *proc_name)
{
#if DEBUG
	printf("DEBUG: trying get vmrss of pid %s\n", pid_dir);
#endif
	psinfo_t *psinfo = read_solaris_psinfo(pid_dir, fbuf);

	if (psinfo == NULL)
		return 0;

	unsigned long result = 0;
#if DEBUG
	printf("DEBUG: process name is [%s]\n", psinfo->pr_fname);
	printf("DEBUG: process rss size is [%zu]\n", psinfo->pr_rssize);
#endif

	if (strcmp(proc_name, psinfo->pr_fname) == 0)
		result = psinfo->pr_rssize * 1024;

	free(psinfo);

	return result;
}

//------------------------------------------------------------------------------

/**
 * Считывает psinfo-файл процесса. Для solaris/sysv4-систем
 *
 * @param pid_dir	PID процесса. Соответствующее имя каталога в /proc.
 * @param fbuf		Файловый буфер
 * @return 		Указатель psinfo_t в случае успешного чтения.
 * 			NULL - при невозможности чтения файла
 */
psinfo_t *read_solaris_psinfo(char *pid_dir, char *fbuf)
{
	static char solaris_psinfo_path[] = "psinfo";

	// Открываем файл /proc/pid/psinfo
	char *psinfo_path = str_builder(5,
		proc_path, path_separator, pid_dir, path_separator, solaris_psinfo_path);
	FILE *psinfo_file = fopen(psinfo_path, "rb");
	free(psinfo_path);

	if (psinfo_file == NULL)
		return NULL;

	// Считываем
	setvbuf(psinfo_file, fbuf, _IOFBF, NBUF_SIZE);
	psinfo_t *psinfo = (psinfo_t *) malloc(sizeof(psinfo_t));
	int result = fread(psinfo, sizeof(psinfo_t), 1, psinfo_file);
	fclose(psinfo_file);

	if (result)
		return psinfo;

	free(psinfo);
	return NULL;
}

//------------------------------------------------------------------------------

/**
 * Суммирует размер областей памяти процесса solaris.
 * @param pid_dir	PID-каталог процесса в /proc
 * @param proc_name	Имя процесса.
 * @param fbuf		Файловый буфер
 * @param mode		Режим сбора.
 * @return		Если имя процесса соответствует pid, то вернётся сумма
 * 			областей памяти данного процесса. Иначе возвращается 0.
 */
unsigned long calc_solaris_proc_map(char *pid_dir, char *proc_name, char* fbuf, int mode)
{
	if (!is_valid_solaris_proc(pid_dir, proc_name, fbuf))
		return 0;

	static char map_name[] = "map";
	char *map_path = str_builder(5, proc_path, path_separator,
		pid_dir, path_separator,
		map_name);
	FILE *map_file = fopen(map_path, "rb");
	free(map_path);

	if (map_file == NULL)
		return 0;

#if DEBUG
	printf("DEBUG: calculating map for %s\n", proc_name);
#endif

	unsigned long result = 0;
	setvbuf(map_file, fbuf, _IOFBF, NBUF_SIZE);
	prmap_t *pmap = (prmap_t *) malloc(sizeof(prmap_t));
	int mflags, readed;

	while ((readed = fread(pmap, sizeof(prmap_t), 1, map_file)) >= 1) {
		mflags = pmap->pr_mflags;

		if (mode == PROC_MAP)
			result += pmap -> pr_size;
		else if (mode == PROC_MAP_RW &&
			mflags & MA_WRITE == MA_WRITE &&
			mflags & MA_READ == MA_READ)
			result += pmap -> pr_size;
		else if (mode == PROC_MAP_SHARED &&
			(mflags & MA_SHARED == MA_SHARED ||
			mflags & MA_SHM == MA_SHM))
			result += pmap -> pr_size;

		if (feof(map_file))
			break;
	}
#if DEBUG
	if (ferror(map_file)) {
		printf("DEBUG: reading error\n");
		perror("err:");
	}

	printf("DEBUG: readed: %d\n", readed);
#endif

	fclose(map_file);
	free(pmap);

	return result;
}

//------------------------------------------------------------------------------

/**
 * Проверка, соответствует ли PID-каталог из /proc имени процесса.
 * @param pid_dir	PID-каталог
 * @param proc_name	имя процесса
 * @param fbuf		файловый буфер
 * @return		1 - PID-каталог соответствует имени процесса
 * 			0 - не соответствует.
 */
int is_valid_solaris_proc(char *pid_dir, char *proc_name, char *fbuf)
{
#if DEBUG
	printf("DEBUG: validating proc name: %s\n", proc_name);
#endif
	psinfo_t *psinfo = read_solaris_psinfo(pid_dir, fbuf);
	if (psinfo == NULL)
		return 0;

	int valid = 0;
	if (strcmp(proc_name, psinfo->pr_fname) == 0)
		valid = 1;

#if DEBUG
	printf("DEBUG: %s is %s - %d\n", proc_name, psinfo->pr_fname, valid);
#endif
	free(psinfo);

	return valid;
}
#endif
