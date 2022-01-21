#!/usr/bin/env sh

# create 10 files with repeated number of a file inside
echo "Creating 10 files with repeated number of a file inside"

for i in 1 2 3 4 5 6 7 8 9 10 11
do
    rm file$i
    touch file"$i"
    for j in 1 2 3 4 5 6 7 8 9 10
    do
        echo $i >> file$i
    done
done

./lab6 --build-filesystem disk.bin

echo "Putting files into disk.bin"

for i in 1 2 3 4 5 6 7 8 9 10 11
do
    ./lab6 --put-file disk.bin file$i file$i
done

echo "Done putting"


echo "List of files in disk.bin"
./lab6 --list-files disk.bin

echo "Map of disk.bin"
./lab6 --print-map disk.bin

echo "Deleting files from disk.bin"

# delete even files
for i in 2 4 6 8 10
do
    ./lab6 --delete-file disk.bin file"$i"
done

echo "Done deleting"

echo "List of files in disk.bin"
./lab6 --list-files disk.bin

echo "Map of disk.bin"
./lab6 --print-map disk.bin