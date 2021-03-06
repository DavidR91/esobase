clang  -g -fsanitize=address *.c -o eb || exit 1

passed=0

for file in tests/**/*.{pass,fail}
do
    # Case should be OK
    if [[ $file == *.pass ]]; then
        
        printf "\e[32m************* $file *************\e[39m\n"
        raw_cmd_output=$(./eb "$file")

        if [ $? -ne 0 ]; then
        echo "$raw_cmd_output"
        exit 1
        fi

       
        ((passed+=1))
    else 

        # Case should fail
        raw_cmd_output=$(./eb "$file")
        exit_code=$? 

        if [ $exit_code -ne 9 ]; then
            echo "$raw_cmd_output"
            printf "\n\e[31m*************EXPECTED CASE $file TO FAIL BUT IT PASSED (code $exit_code) *************\e[39m\n\n"            
            exit 1
        else 
            printf "\e[32m************* Intentional failure: $file *************\e[39m\n"
            ((passed+=1))
        fi
    fi
done 

printf "\e[32m************* $passed cases passed *************\e[39m\n"
