#!/bin/bash
#
# sanity check script for Project 1A
#
LAB="lab1b"
README="README"
MAKEFILE="Makefile"

EXPECTED="my.key"
EXPECTEDS="c"
CLIENT=./lab1b-client
SERVER=./lab1b-server
PGMS="$CLIENT $SERVER"

TIMEOUT=1

if [ -z "$1" ]
then
	echo usage: $0 your-student-id
	exit 1
else
	student=$1
fi

# make sure the tarball has the right name
tarball="$LAB-$student.tar.gz"
if [ ! -s $tarball ]
then
	echo "ERROR: Unable to find submission tarball:" $tarball
	exit 1
fi

# make sure we can untar it
TEMP="/tmp/TestTemp.$$"
echo "... Using temporary testing directory" $TEMP
function cleanup {
	cd
	rm -rf $TEMP
	exit $1
}

mkdir $TEMP
cp $tarball $TEMP
cd $TEMP
echo "... untaring" $tarbsll
tar xvf $tarball
if [ $? -ne 0 ]
then
	echo "ERROR: Error untarring $tarball"
	cleanup 1
fi

# make sure we find all the expected files
echo "... checking for expected files"
for i in $README $MAKEFILE $EXPECTED
do
	if [ ! -s $i ]
	then
		echo "ERROR: unable to find file" $i
		cleanup 1
	else
		echo "        $i ... OK"
	fi
done

# make sure the README contains name and e-mail
echo "... checking for submitter info in $README"
name=`grep "NAME:" $README | cut -d: -f2`
if [ -z "$name" ]
then
	echo "ERROR: $README does not contain a NAME: line"
	cleanup 1
else
	echo "Make sure that $name is your correct name."
fi

email=`grep "EMAIL:" $README | cut -d: -f2`
if [ -z "$email" ]
then
	echo "ERROR: $README does not contain a EMAIL: line"
	cleanup 1
else
	echo "Make sure that $email is your correct email address."
fi

id=`grep "ID:" $README | cut -d: -f2`
if [ -z "$id" ]
then
	echo "ERROR: $README does not contain a ID: line"
	cleanup 1
else
	echo "Make sure that $id is your correct student ID."
fi

# make sure we find files with all the expected suffixes
echo "... checking for files of expected types"
for s in $EXPECTEDS
do
	names=`echo *.$s`
	if [ "$names" = '*'.$s ]
	then
		echo "ERROR: unable to find any .$s files"
		cleanup 1
	else
		for f in $names
		do
			echo "        $f ... OK"
		done
	fi
done

# make sure we can build the expected program
echo "... building default target(s)"
make 2> STDERR
if [ $? -ne 0 ]
then
	echo "ERROR: default make fails"
	cleanup 1
fi
if [ -s STDERR ]
then
	echo "ERROR: make produced output to stderr"
	cleanup 1
fi

echo "... checking for expected products"
for p in $PGMS
do
	if [ ! -x $p ]
	then
		echo "ERROR: unable to find expected executable" $p
		cleanup 1
	else
		echo "        $p ... OK"
	fi
done

# see if it excepts the expected arguments
function testrc {
	if [ $1 -ne $2 ]
	then
		echo "ERROR: expected RC=$2, GOT $1"
		cleanup 1
	fi
}

for p in $PGMS
do
	echo "... $p detects/reports bogus arguments"
	$p --bogus < /dev/tty > /dev/null 2>STDERR
	testrc $? 1
	if [ ! -s STDERR ]
	then
		echo "No Usage message to stderr for --bogus"
		cleanup 1
	else
		cat STDERR
	fi
done

echo "... $CLIENT detects/reports system unwritable log file"
$CLIENT --log=/tmp < /dev/tty > /dev/null 2>STDERR
testrc $? 1
if [ ! -s STDERR ]
then
	echo "No error message to stderr for non-writable log file"
	cleanup 1
else
	cat STDERR
fi

echo "... usage of mcrypt"
for r in mcrypt_module_open mcrypt_generic mdecrypt_generic
do
	grep $r *.c > /dev/null
	if [ $? -ne 0 ] 
	then
		echo "No calls to $r"
		cleanup 1
	else
		echo "    ... $r ... OK"
	fi
done

# looks good
echo
echo "This submission passes the sanity check."
cleanup 0
