#!/bin/sh
#
# change version
# (should be executed in the parent directory)
#
newver=$1

for i in ./libjulius/configure.in ./support/build-all.sh; do
	echo $i
	sed -e "s/^JULIUS_VERSION=[^ ]*/JULIUS_VERSION=$newver/" < $i > tmp
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
