clang  -g -fsanitize=address main.c -o eb

for file in tests/*.{pass,fail}
do
    echo "********** $file **********"

    while read assertion; 
        do 
            if [ -n "$assertion" ]; then

                if [[ $assertion == \#* ]]; then 
                    continue
                fi

                # Case should be OK
                if [[ $file == *.pass ]]; then
                    ./eb "$assertion" || exit 1
                    printf "\e[32m*************OK*************\e[39m\n"
                else 

                    # Case should fail
                    ./eb "$assertion"

                    if [ $? -eq 0 ]; then
                        printf "\n\e[31m*************EXPECTED CASE TO FAIL BUT IT PASSED*************\e[39m\n\n"
                        exit 1
                    else 
                        printf "\e[32m*************CASE FAILURE EXPECTED: OK*************\e[39m\n"
                    fi


                fi
            fi
    done < $file
done 
