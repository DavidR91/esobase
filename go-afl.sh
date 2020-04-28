cd /esobase
mkdir afl_out
mkdir afl_in

cp tests/**/*.pass afl_in/

afl-fuzz -i afl_in -o afl_out -- ./eb-afl @@