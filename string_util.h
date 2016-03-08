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
#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * Удаляет пробелы в начале и в конце подстроки.
	 *
	 * Т.е. смещает указатель до первого не-пробела
	 * и ставит вместо пробелов в конце подстроки символы окончания строки
	 * @param substring	подстрока
	 */
	extern void trim_substring(char *substring);

	/**
	 * Определяет длину подстроки.
	 * Корректно вычисляет длину подстроки в буфере символов.
	 * Подстрока должна завершаться \0
	 * @param substring
	 * @return длина подстроки без учёта \0
	 */
	extern int substr_len(const char *substring);

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
	extern int line_is_comment(const char *line);

	/**
	 * Считывает строку из файла.
	 * @param file дескриптор файла
	 * @param fbuf строка, куда будет помещён результат.
	 * @param fbuf_size размер строки
	 * @return 1 в случае успешного завершения. 0 в случае неудачи.
	 */
	extern int read_line(FILE *file, char *lbuf, int lbuf_size);

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
	extern char *str_summ(const char *first, const char *second);

	/**
	 * Собирает одну строку из нескольких строк.
	 * Предполагается, что используются строки без подстрок.
	 *
	 * @param num	общее число суммируемых строк
	 * @param ...	суммируемые строки, char *
	 * @return 	указатель на новую строку, содержащую сумму
	 * 		переданных строк.
	 */
	extern char *str_builder(int num, ...);

	/**
	 * Сдвигает символы в строке влево на один символ.
	 * @param str	указатель на строку символов
	 */
	extern void left_shift(char * str);

#ifdef __cplusplus
}
#endif

#endif /* STRING_UTIL_H */

