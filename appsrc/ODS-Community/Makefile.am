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
#  

TOP=@TOP@

if WITH_ODS_VAD
nodist_vad_DATA = ods_community_dav.vad
endif

CLEANFILES = \
	*.vad \
	make_community_vad.log \
	make_community_vad.xml \
	virtuoso.db \
	virtuoso.ini \
	virtuoso.lck \
	virtuoso.log \
	virtuoso.pxa \
	virtuoso.tdb \
	virtuoso.trx

ods_community_dav.vad: $(EXTRA_DIST)
	PATH="$(TOP)/binsrc/virtuoso:$(TOP)/binsrc/tests:.:$$PATH" PORT=$(PORT) HOME="$(TOP)" HOST=localhost sh ./make_vad.sh

clean-local:
	-rm -rf vad

EXTRA_DIST = \
	make_vad.sh \
	sql/*.sql \
	sql/install.bat \
	sql/uninstall.bat \
	vad_version \
	www-root/*.vsp \
	www-root/*.vspx \
	www-root/public/*.html \
	www-root/public/css/*.css \
	www-root/public/images/*.gif \
	www-root/public/images/*.jpg \
	www-root/public/images/*.png \
	www-root/public/images/lightblue/*.css \
	www-root/public/images/lightblue/*.gif \
	www-root/public/images/lightblue/*.jpg \
	www-root/public/images/lightblue/*.png \
	www-root/public/images/lightblue/*.jfif \
	www-root/public/js/*.js \
	www-root/templates/openlink/*.css \
	www-root/templates/openlink/*.vspx \
	www-root/widgets/*.xsl
