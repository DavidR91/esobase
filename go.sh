clang  -g -fsanitize=address *.c -o eb || exit 1

passed=0

for file in tests/**/*.{pass,fail}
do
   
    # Case should be OK
    if [[ $file == *.pass ]]; then
        raw_cmd_output=$(./eb "$file")

        if [ $? -ne 0 ]; then
        echo "********** $file **********"
        echo "$raw_cmd_output"
        exit 1
        fi

        printf "\e[32m************* $file *************\e[39m\n"
        ((passed+=1))
    else 

        # Case should fail
        raw_cmd_output=$(./eb "$file")

        if [ $? -eq 0 ]; then
            echo "$raw_cmd_output"
            printf "\n\e[31m*************EXPECTED CASE TO FAIL BUT IT PASSED*************\e[39m\n\n"            
            exit 1
        else 
            printf "\e[32m************* Intentional failure: $file *************\e[39m\n"
            ((passed+=1))
        fi
    fi
done 

printf "\e[32m************* $passed cases passed *************\e[39m\n"
