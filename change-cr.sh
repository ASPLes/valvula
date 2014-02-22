#!/bin/sh

sed -i 's/Copyright (C) 2013/Copyright (C) 2014/g' `rgrep "Copyright (C) 2013"  * | grep -v "\.svn/" | grep -v "\~:" | cut -f1 -d:`