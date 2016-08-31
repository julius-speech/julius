#!/bin/sh
#
# change version
# (should be executed in the parent directory)
#
newver_major=$1
newver_minor=$2

newver=${newver_major}.${newver_minor}

for i in ./support/build-all.sh; do
	echo $i
	sed -e "s/^JULIUS_VERSION=[^ ]*/JULIUS_VERSION=$newver/" < $i > tmp
	diff $i tmp
	mv tmp ${i}
done
for i in ./libjulius/configure.in ; do
	echo $i
	cat $i | sed -e "s/^JULIUS_MAJOR_VERSION=[^ ]*/JULIUS_MAJOR_VERSION=$newver_major/" | sed -e "s/^JULIUS_MINOR_VERSION=[^ ]*/JULIUS_MINOR_VERSION=$newver_minor/"  > tmp
	diff $i tmp
	mv tmp ${i}
done
for i in ./libsent/configure.in; do
	echo $i
	sed -e "s/^LIBSENT_VERSION=[^ ]*/LIBSENT_VERSION=$newver/" < $i > tmp
	diff $i tmp
	mv tmp ${i}
done

echo 
echo updating configure scripts...
cd ./julius; autoconf
cd ../libsent; autoconf
cd ../libjulius; autoconf
