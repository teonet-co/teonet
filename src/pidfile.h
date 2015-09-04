/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * pidfile.h
 * Copyright (C) Kirill Scherba 2011-2015 <kirill@scherba.ru>
 *
 * TEONET is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TEONET is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PIDFILE_H
#define	PIDFILE_H

#ifdef	__cplusplus
extern "C" {
#endif

void	init_pidfile(int port);
void	kill_pidfile ();
int		write_pidfile();
void	read_pidfile(int* port_pid);
int		check_pid(int pid);
void	remove_pidfile();

#ifdef	__cplusplus
}
#endif

#endif	/* PIDFILE_H */