all: restore_env build tests

restore_env:
	./restore_env.sh

tests: restore_env
	cd ./libAuMsgParserTests && make all

build: restore_env
	cd ./libAuMsgParser && make all
