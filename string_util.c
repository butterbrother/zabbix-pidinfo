/*
 * Функции по работе со строками и подстроками.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#define DEBUG 0

void trim_substring(char *substring);
int substr_len(const char *substring);
int line_is_comment(const char *line);
int read_line(FILE *file, char *lbuf, int lbuf_size);
char *str_summ(const char *first, const char *second);
char *str_builder(int num, ...);

/**
 * Удаляет пробелы в начале и в конце подстроки.
 *
 * Т.е. смещает указатель до первого не-пробела
 * и ставит вместо пробелов в конце подстроки символы окончания строки
 * @param substring	подстрока
 */
void trim_substring(char *substring)
{
#if DEBUG
	printf("before trim: [%s], ", substring);
#endif

	// Сдвигаем указатель подстроки, пропуская пробелы
	while (*substring != '\0' && *substring == ' ')
		substring++;

#if DEBUG
	printf("after begin trim: [%s], ", substring);
#endif

	// Ищем конец подстроки
	//int i = strlen(substring); Не подходит, возвращает размер, меньший чем у подстроки
	int i = substr_len(substring);

	// Удаляем пробелы в конце
	while (--i >= 0 && substring[i] == ' ')
		substring[i] = '\0';

#if DEBUG
	printf("after end trim: [%s]\n", substring);
#endif
}

//------------------------------------------------------------------------------

/**
 * Определяет длину подстроки.
 * Корректно вычисляет длину подстроки в буфере символов.
 * Подстрока должна завершаться \0
 * @param substring
 * @return длина подстроки без учёта \0
 */
int substr_len(const char *substring)
{
	int i = 0;
	while (substring[i] != '\0')
		++i;

	return i;
}

//------------------------------------------------------------------------------

/**
 * Определяет, является ли строка только комментарием
 *
 * Такие строки начинаются с #, при этом # может быть не в самом
 * начале строки, а после нескольких пробелов или табов.
 * Переносы не учитываются, предполагаем, что это одна строка из файла.
 *
 * @param line	строка
 * @return 	1 - является. Иначе - нет
 */
int line_is_comment(const char *line)
{

#if DEBUG
	printf("DEBUG: detecting line is comment [%s]: ", line);
#endif

	const char *pl = line;
	char c;
	while ((c = *(pl++)) != '\0') {

#if DEBUG
		printf("%c", c);
#endif

		switch (c) {
			/* Ищем, начинается ли строка с решётки
			 * пропуская начальные пробелы и табы */
		case '#':
#if DEBUG
			printf(" - this is comment\n");
#endif
			return 1;
			break;

		case ' ': case '\t':
			continue;
			break;

			/* Если не встретили и начались данные - это не коммент */
		default:
#if DEBUG
			printf(" - is not comment, ok\n");
#endif
			return 0;
			break;
		}
	}

#if DEBUG
	printf(" line is empty, skip it\n");
#endif
	return 1; // Пустую строку не обрабатываем, это отступ для красоты
}

//------------------------------------------------------------------------------

/**
 * Считывает строку из файла.
 * @param file дескриптор файла
 * @param fbuf строка, куда будет помещён результат.
 * @param fbuf_size размер строки
 * @return 1 в случае успешного завершения. 0 в случае неудачи.
 */
int read_line(FILE *file, char *lbuf, int lbuf_size)
{
	if (lbuf_size < 2 || file == NULL || feof(file) || ferror(file))
		return 0;

	int i, c;
	for (i = 0; i < (lbuf_size - 1); ++i) {
		c = fgetc(file);

		switch (c) {
		case EOF:
			// Если сразу встретили конец файла, то это не строка
			if (i == 0)
				return 0;
			// Дальнейшее поведение аналогично встрече символа переноса
		case '\n':
			lbuf[i] = '\0';
#if DEBUG
			printf("DEBUG: readed line: [%s]\n", lbuf);
#endif
			return 1;
			break;

		case '\t':
			--i;
			continue;
			break;

		default:
			lbuf[i] = c;
			break;
		}
	}

	lbuf[++i] = '\0';
#if DEBUG
	printf("DEBUG: readed line: [%s]\n", lbuf);
#endif
	return 1;
}

//------------------------------------------------------------------------------

/**
 * Выполняет конкатенацию (склеивание) двух строк.
 *
 * Предполагается, что это полноценные строки, а не подстроки в буфере,
 * иначе может быть неверно определена длина подстрок.
 * Возвращает указатель на новую строку, длина которой равна сумме длин
 * склеиваемых строк + '\0' (будет создана новая строка).
 *
 * @param first		первая строка
 * @param second	вторая строка
 * @return 		указатель на новую строку,
 * 			содержащую сумму двух предыдущих
 * 			строк.
 */
char *str_summ(const char *first, const char *second)
{
	int common_length = strlen(first) + strlen(second) + 1;
	char *new_str = malloc(sizeof(char) * common_length);

	strcpy(new_str, first);
	strcat(new_str, second);

	return new_str;
}

//------------------------------------------------------------------------------

/**
 * Собирает одну строку из нескольких строк.
 * Предполагается, что используются строки без подстрок.
 *
 * @param num	общее число суммируемых строк
 * @param ...	суммируемые строки, char *
 * @return 	указатель на новую строку, содержащую сумму
 * 		переданных строк.
 */
char *str_builder(int num, ...)
{
	size_t common_length = 1;
	va_list strs;
	char *ptr;

	va_start(strs, num);
	int i = num;
	while (--i >= 0) {
		ptr = va_arg(strs, char *);
		common_length += strlen(ptr);
	}

	va_end(strs);

	char *new_str = malloc(common_length);
	new_str[0] = '\0';

	va_start(strs, num);
	i = num;
	while (--i >= 0) {
		ptr = va_arg(strs, char *);
		strcat(new_str, ptr);
	}
	va_end(strs);

	return new_str;
}

//------------------------------------------------------------------------------

/**
 * Сдвигает символы в строке влево на один символ.
 * @param str	указатель на строку символов
 */
void left_shift(char * str)
{
	int i;
	for (i = 1; i <= strlen(str); ++i)
		str[i - 1] = str[i];
}