#!/bin/sh

set -x

echo "Preparing sources"
rm -rf rpm/SOURCES/*.tar.gz
rm -rf rpm/BUILD/*
rm -rf rpm/BUILDROOT/*
find rpm/RPMS/ -type f -exec rm {} \;

rm valvula-*.tar.gz
make dist
cp valvula-*.tar.gz rpm/SOURCES/

echo "Calling to compile packages.."
LANG=C rpmbuild -ba --define '_topdir /usr/src/valvula/rpm' rpm/SPECS/valvula.spec

echo "Output ready at rpm/RPMS"
find rpm/RPMS -type f -name '*.rpm' > rpm/RPMS/files
cat rpm/RPMS/files


