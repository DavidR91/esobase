clang  -g -fsanitize=address main.c -o eb

passed=0

for file in tests/*.{pass,fail}
do
    echo "********** $file **********"
   
    # Case should be OK
    if [[ $file == *.pass ]]; then
        ./eb "$file" || exit 1
        printf "\e[32m*************OK*************\e[39m\n"
        ((passed+=1))
    else 

        # Case should fail
        ./eb "$file"

        if [ $? -eq 0 ]; then
            printf "\n\e[31m*************EXPECTED CASE TO FAIL BUT IT PASSED*************\e[39m\n\n"
            exit 1
        else 
            printf "\e[32m*************CASE FAILURE EXPECTED: OK*************\e[39m\n"
            ((passed+=1))
        fi
    fi
done 

printf "\e[32m************* $passed cases passed *************\e[39m\n"
