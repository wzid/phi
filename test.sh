#!/bin/bash

# formatting help 
ITALIC="\033[3m"
BOLD="\033[1m"
BLUE="\033[34m"
GREEN="\033[32m"
WHITE="\033[97m"
RED="\033[31m"
RESET="\033[0m"


type=$1
COMPILER=./bin/test-$type
DIR=./test-cases/$type

# Generate output for each testcase and multithread it
pids=()
file_names=()
for file in ./test-cases/$type/*.phi; do
    
    file_name=$(basename $file .phi)
    # Run the compiler with a timeout of 10 seconds, redirecting both stdout and stderr to .out file
    # Use & to run in background
    # The timeout is used to make sure that we don't get stuck in an infinite loop while compiling the program
    # This only matters if there is an error in our compiler code where an infinite loop could occur
    gtimeout 10s $COMPILER $file > $DIR/$file_name.out 2>&1 &

    pids+=($!)
    file_names+=($file_name)
done


error_file_names=()
for i in ${!pids[@]}; do
    pid=${pids[$i]}
    file_name=${file_names[$i]}
    out_file="$DIR/$file_name.out"

    # Wait for the process to finish
    wait $pid
    status=$?

    # If non zero return value (compile error)
    if [ $status -ne 0 ]; then
        error_file_names+=($file_name)
    fi

    # If timeout occurred (status 124), append message to .out file
    if [ $status -eq 124 ]; then
        echo "[TIMEOUT] Test case exceeded 10 seconds." >> "$out_file"
    fi
done

echo
echo

echo -e "${GREEN}******** ${BOLD}SUMMARY${RESET}${GREEN} ********${RESET}"

if [ ${#error_file_names[@]} -ne 0 ]; then

    echo -e "${BLUE}${#error_file_names[@]}${RESET} test cases resulted in an ${RED}error${RESET}:"
    for file_name in "${error_file_names[@]}"; do
        echo -e "\t- $file_name.phi"
    done
    echo
    echo -e "${BOLD}${ITALIC}Review error logs in each files respective .out file${RESET}"
    echo

fi


passed_count=0
failed_file_names=()
for file_name in ${file_names[@]}; do
    # If the file name is in the error list, skip it (using a regex match)
    if [[ ${error_file_names} =~ $file_name ]]; then
        continue
    fi
    
    # Insert diff command here
    if ! diff -w -u $DIR/$file_name.out $DIR/$file_name.ans > $DIR/$file_name.diff; then
        failed_file_names+=($file_name)
    else
        rm $DIR/$file_name.diff # remove diff if its empty
        passed_count=$(($passed_count + 1))
    fi
done


if [ ${#failed_file_names[@]} -ne 0 ]; then
    echo -e "${RED}${#failed_file_names[@]}${RESET} test case(s) did not produce a correct result:"

    for file_name in ${failed_file_names[@]}; do
        echo -e "\t- ${file_name}.phi"
    done

    echo
    echo -e "${BOLD}${ITALIC}Review error logs for each failed testcase in their respective .diff file${RESET}"
    echo

fi


if [[ ${#error_file_names[@]} -eq 0 && ${#failed_file_names[@]} -eq 0 ]]; then
    echo -e "${BOLD}All ${BLUE}${passed_count}${RESET}${BOLD} test cases passed!${RESET}"
else
    echo -e "${BLUE}${passed_count}${RESET} test case(s) passed!"
fi

