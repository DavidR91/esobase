clang  -g -fsanitize=address main.c -o eb

for file in tests/*.eso
do
    echo "********** $file **********"

    while read assertion; 
        do 
            if [ -n "$assertion" ]; then
                ./eb "$assertion" || exit 1
            fi
    done < $file
done 
