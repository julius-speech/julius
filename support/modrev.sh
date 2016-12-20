#!/bin/sh
#
# change version
# (should be executed in the parent directory)
#
# ./support/modrev.sh 4 4.1
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
for i in ./libjulius/include/julius/config-msvc-libjulius.h ./libjulius/include/julius/config-ios-libjulius.h ./libjulius/include/julius/config-android-libjulius.h; do
	echo $i
	sed -e "s/^#define JULIUS_VERSION \"[^ ]*\"/#define JULIUS_VERSION \"$newver\"/" < $i > tmp
	diff $i tmp
	mv tmp ${i}
done
for i in ./libsent/configure.in; do
	echo $i
	cat $i | sed -e "s/^LIBSENT_MAJOR_VERSION=[^ ]*/LIBSENT_MAJOR_VERSION=$newver_major/" | sed -e "s/^LIBSENT_MINOR_VERSION=[^ ]*/LIBSENT_MINOR_VERSION=$newver_minor/"  > tmp
	diff $i tmp
	mv tmp ${i}
done
for i in ./libsent/include/sent/config-msvc-libsent.h ./libsent/include/sent/config-ios-libsent.h ./libsent/include/sent/config-android-libsent.h; do
	echo $i
	sed -e "s/^#define LIBSENT_VERSION \"[^ ]*\"/#define LIBSENT_VERSION \"$newver\"/" < $i > tmp
	diff $i tmp
	mv tmp ${i}
done

echo 
echo updating configure scripts...
cd ./julius; autoconf
cd ../libsent; autoconf
cd ../libjulius; autoconf
