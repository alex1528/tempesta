/*
 *		Tempesta TLS
 *
 *  Debugging routines.
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  Copyright (C) 2015-2018 Tempesta Technologies, Inc.
 *  SPDX-License-Identifier: GPL-2.0
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#if defined(DEBUG) && (DEBUG == 3)

#include "debug.h"

#define DEBUG_BUF_SIZE	  512

static int debug_threshold = 0;

void ttls_debug_set_threshold(int threshold)
{
	debug_threshold = threshold;
}

void ttls_debug_print_msg(const ttls_context *ssl, int level,
							  const char *file, int line,
							  const char *format, ...)
{
	va_list argp;
	char str[DEBUG_BUF_SIZE];
	int ret;

	if (NULL == ssl || NULL == ssl->conf || level > debug_threshold)
		return;

	va_start(argp, format);
	ret = vsnprintf(str, DEBUG_BUF_SIZE, format, argp);
	va_end(argp);

	if (ret >= 0 && ret < DEBUG_BUF_SIZE - 1)
	{
		str[ret]	 = '\n';
		str[ret + 1] = '\0';
	}
}

void ttls_debug_print_ret(const ttls_context *ssl, int level,
					  const char *file, int line,
					  const char *text, int ret)
{
	char str[DEBUG_BUF_SIZE];

	if (ssl->conf == NULL || level > debug_threshold)
		return;

	/*
	 * With non-blocking I/O and examples that just retry immediately,
	 * the logs would be quickly flooded with WANT_READ, so ignore that.
	 * Don't ignore WANT_WRITE however, since is is usually rare.
	 */
	if (ret == TTLS_ERR_WANT_READ)
		return;

	ttls_snprintf(str, sizeof(str), "%s() returned %d (-0x%04x)\n",
			  text, ret, -ret);
}

void ttls_debug_print_buf(const ttls_context *ssl, int level,
					  const char *file, int line, const char *text,
					  const unsigned char *buf, size_t len)
{
	char str[DEBUG_BUF_SIZE];
	char txt[17];
	size_t i, idx = 0;

	if (ssl->conf == NULL || level > debug_threshold)
		return;

	ttls_snprintf(str + idx, sizeof(str) - idx, "dumping '%s' (%u bytes)\n",
			  text, (unsigned int) len);

	idx = 0;
	memset(txt, 0, sizeof(txt));
	for (i = 0; i < len; i++)
	{
		if (i >= 4096)
			break;

		if (i % 16 == 0)
		{
			if (i > 0)
			{
				ttls_snprintf(str + idx, sizeof(str) - idx, "  %s\n", txt);
				idx = 0;
				memset(txt, 0, sizeof(txt));
			}

			idx += ttls_snprintf(str + idx, sizeof(str) - idx, "%04x: ",
							 (unsigned int) i);

		}

		idx += ttls_snprintf(str + idx, sizeof(str) - idx, " %02x",
						 (unsigned int) buf[i]);
		txt[i % 16] = (buf[i] > 31 && buf[i] < 127) ? buf[i] : '.' ;
	}

	if (len > 0)
	{
		for (/* i = i */; i % 16 != 0; i++)
			idx += ttls_snprintf(str + idx, sizeof(str) - idx, "   ");

		ttls_snprintf(str + idx, sizeof(str) - idx, "  %s\n", txt);
	}
}

void ttls_debug_print_ecp(const ttls_context *ssl, int level,
					  const char *file, int line,
					  const char *text, const ttls_ecp_point *X)
{
	char str[DEBUG_BUF_SIZE];

	if (ssl->conf == NULL || level > debug_threshold)
		return;

	ttls_snprintf(str, sizeof(str), "%s(X)", text);
	ttls_debug_print_mpi(ssl, level, file, line, str, &X->X);

	ttls_snprintf(str, sizeof(str), "%s(Y)", text);
	ttls_debug_print_mpi(ssl, level, file, line, str, &X->Y);
}

void ttls_debug_print_mpi(const ttls_context *ssl, int level,
					  const char *file, int line,
					  const char *text, const ttls_mpi *X)
{
	char str[DEBUG_BUF_SIZE];
	int j, k, zeros = 1;
	size_t i, n, idx = 0;

	if (ssl->conf == NULL || X == NULL || level > debug_threshold)
		return;

	for (n = X->n - 1; n > 0; n--)
		if (X->p[n] != 0)
			break;

	for (j = (sizeof(ttls_mpi_uint) << 3) - 1; j >= 0; j--)
		if (((X->p[n] >> j) & 1) != 0)
			break;

	ttls_snprintf(str + idx, sizeof(str) - idx, "value of '%s' (%d bits) is:\n",
			  text, (int) ((n * (sizeof(ttls_mpi_uint) << 3)) + j + 1));

	idx = 0;
	for (i = n + 1, j = 0; i > 0; i--)
	{
		if (zeros && X->p[i - 1] == 0)
			continue;

		for (k = sizeof(ttls_mpi_uint) - 1; k >= 0; k--)
		{
			if (zeros && ((X->p[i - 1] >> (k << 3)) & 0xFF) == 0)
				continue;
			else
				zeros = 0;

			if (j % 16 == 0)
			{
				if (j > 0)
				{
					ttls_snprintf(str + idx, sizeof(str) - idx, "\n");
					idx = 0;
				}
			}

			idx += ttls_snprintf(str + idx, sizeof(str) - idx, " %02x", (unsigned int)
							 (X->p[i - 1] >> (k << 3)) & 0xFF);

			j++;
		}

	}

	if (zeros == 1)
		idx += ttls_snprintf(str + idx, sizeof(str) - idx, " 00");

	ttls_snprintf(str + idx, sizeof(str) - idx, "\n");
}

static void debug_print_pk(const ttls_context *ssl, int level,
							const char *file, int line,
							const char *text, const ttls_pk_context *pk)
{
	size_t i;
	ttls_pk_debug_item items[TTLS_PK_DEBUG_MAX_ITEMS];
	char name[16];

	memset(items, 0, sizeof(items));

	if (ttls_pk_debug(pk, items) != 0)
		return;

	for (i = 0; i < TTLS_PK_DEBUG_MAX_ITEMS; i++)
	{
		if (items[i].type == TTLS_PK_DEBUG_NONE)
			return;

		ttls_snprintf(name, sizeof(name), "%s%s", text, items[i].name);
		name[sizeof(name) - 1] = '\0';

		if (items[i].type == TTLS_PK_DEBUG_MPI)
			ttls_debug_print_mpi(ssl, level, file, line, name, items[i].value);
		else
		if (items[i].type == TTLS_PK_DEBUG_ECP)
			ttls_debug_print_ecp(ssl, level, file, line, name, items[i].value);
	}
}

static void debug_print_line_by_line(const ttls_context *ssl, int level,
									  const char *file, int line, const char *text)
{
	char str[DEBUG_BUF_SIZE];
	const char *start, *cur;

	start = text;
	for (cur = text; *cur != '\0'; cur++)
	{
		if (*cur == '\n')
		{
			size_t len = cur - start + 1;
			if (len > DEBUG_BUF_SIZE - 1)
				len = DEBUG_BUF_SIZE - 1;

			memcpy(str, start, len);
			str[len] = '\0';
			start = cur + 1;
		}
	}
}

void ttls_debug_print_crt(const ttls_context *ssl, int level,
					  const char *file, int line,
					  const char *text, const ttls_x509_crt *crt)
{
	char buf[1024];
	int i = 0;

	if (ssl->conf == NULL || crt == NULL || level > debug_threshold)
		return;

	while (crt != NULL)
	{
		ttls_snprintf(buf, sizeof(buf), "%s #%d:\n", text, ++i);

		ttls_x509_crt_info(buf, sizeof(buf) - 1, "", crt);
		debug_print_line_by_line(ssl, level, file, line, buf);

		debug_print_pk(ssl, level, file, line, "crt->", &crt->pk);

		crt = crt->next;
	}
}

#endif
