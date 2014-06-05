#!/bin/bash
rsync --exclude=.svn --exclude=update-web.sh -avz *.css *.html aspl-web@www.aspl.es:www/valvula/
rsync --exclude=.svn --exclude=update-web.sh -avz es/*.html aspl-web@www.aspl.es:www/valvula/es/
rsync --exclude=.svn --exclude=update-web.sh -avz images/*.png images/*.gif aspl-web@www.aspl.es:www/valvula/images/



