passed=0

# We only care about leaks from passing test cases
for file in tests/*.pass
do
    echo "********** $file **********"
   
    valgrind --leak-check=full --error-exitcode=1 ./eb "$file" || exit 1
    printf "\e[32m*************OK*************\e[39m\n"
    ((passed+=1))
done 

printf "\e[32m************* $passed cases passed *************\e[39m\n"
