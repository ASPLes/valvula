#!/bin/sh
# Valvula: a high performance policy daemon
# Copyright (C) 2014 Advanced Software Production Line, S.L.
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2.1 of
# the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA
# 
# You may find a copy of the license under this software is
# released at COPYING file.
# 
# For comercial support about integrating valvula or any other ASPL
# software production please contact as at:
#         
#     Postal address:
# 
#        Advanced Software Production Line, S.L.
#        C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
#        Alcalá de Henares 28802 Madrid
#        Spain
# 
#     Email address: info@aspl.es - http://www.aspl.es/valvula
# 

VERSION=`cat VERSION`
PACKAGE="Valvula ${VERSION}: a high performance policy daemon service"

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed to compile $PACKAGE";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile $PACKAGE";
	echo;
	exit;
}

echo "Generating configuration files for $PACKAGE, please wait...." 
echo; 

export PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/local/lib/pkgconfig

touch NEWS README AUTHORS ChangeLog 
libtoolize --force;
aclocal $ACLOCAL_FLAGS; 
autoheader --warnings=error;
automake --add-missing;
autoconf --force --warnings=error;

./configure $@ --enable-maintainer-mode
