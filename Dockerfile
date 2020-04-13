FROM alpine
RUN apk add --update bash alpine-sdk clang valgrind build-base compiler-rt afl
ADD . /esobase
CMD cd /esobase && bash ./go-linux.sh && bash ./go-linux-leak.sh