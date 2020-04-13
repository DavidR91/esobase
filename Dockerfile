FROM alpine
RUN apk add --update bash alpine-sdk clang valgrind build-base compiler-rt afl
ADD . /esobase
RUN cd /esobase && clang  -g *.c -o eb && afl-clang-fast *.c -o eb-afl
CMD cd /esobase && bash ./go-linux.sh && bash ./go-linux-leak.sh