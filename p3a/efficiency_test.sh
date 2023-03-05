#!/bin/bash
# https://piazza.com/class/l7kt5onxp5h4hp/post/1401

if [ ! -f Makefile ]
then
    echo "Makefile not found."
    exit 1
fi

if [ ! -f psort.c ]
then
    echo "psort.c not found."
    exit 1
fi

source env.sh

echo "Project directory $PROJ_PATH"
echo "Setup test folder $TEST_PATH"

if [ -d $TEST_PATH ]
then
    echo "Old test folder found, removing..."
    rm -rf $TEST_PATH
fi

mkdir $TEST_PATH
cp Makefile psort.c test-psort.sh ~cs537-1/tests/p3a/efficiency_tests/benchmark $TEST_PATH
echo "Copying tests..."
cp -r ~cs537-1/tests/p3a/tests $TEST_PATH/tests
cp -r ~cs537-1/tests/p3a/efficiency_tests $TEST_PATH/efficiency_tests
cd $TEST_PATH
echo "Building project..."
make

echo "[NORMAL TEST]"
echo "Warm up Test, this may take seconds or minutes"
time ./test-psort.sh -c
rm -rf tests-out
echo "Repeat Test 1/3"
time ./test-psort.sh -c
rm -rf tests-out
echo "Repeat Test 2/3"
time ./test-psort.sh -c
rm -rf tests-out
echo "Repeat Test 3/3"
time ./test-psort.sh -c
rm -rf tests-out

echo "[EFFICIENCY TEST]"

for CASE in {1..10}; do
    echo "Test Case $CASE : Baseline result"
    { time ./benchmark ./efficiency_tests/eff$CASE.in ./eff-baseline.out; } 2>&1 | grep real
    echo "Test Case $CASE : Your result"
    { time ./psort ./efficiency_tests/eff$CASE.in ./eff-my.out; } 2>&1 | grep real
    echo "SHA1 Checksums"
    sha1sum ./eff-baseline.out ./eff-my.out
    rm -f ./eff-baseline.out ./eff-my.out
    echo "---------------------------------------------------"
done

echo "Clean up test folder $TEST_PATH"
cd $PROJ_PATH
rm -rf $TEST_PATH