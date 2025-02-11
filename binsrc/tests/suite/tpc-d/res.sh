#!/bin/sh
#
#  $Id$
#
#  This file is part of the OpenLink Software Virtuoso Open-Source (VOS)
#  project.
#
#  Copyright (C) 1998-2022 OpenLink Software
#
#  This project is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by the
#  Free Software Foundation; only version 2 of the License, dated June 1991.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

action=$1
case z$action
    in
    zvirtuoso)
    #isql $PORT dba dba < Q.sql > temp.res 
    gawk -f test.awk -v mode=oracle temp.res
    ;;

    zmysql)
    isql-iodbc mysql demo demo < Q_mysql.sql > temp.res
    gawk -f test.awk -v mode=virt temp.res
    ;;

    zinno)
    isql-iodbc inno demo demo < Q_mysql.sql > temp.res
    gawk -f test.awk -v mode=virt temp.res
    ;;

    zpostgesql)
    psql demo < Q_psql.sql > temp.res
    gawk -f test.awk -v mode="postgesql" temp.res
    ;;

    zoracle)
    isql-iodbc oralite demo demo < Q_ora.sql > temp.res
    gawk -f test.awk -v mode=oracle temp.res
    ;;

    z*)
    echo "usage (virtuoso | mysql | inno | postgesql | oracle )"
    exit 1
    ;;
esac
    



