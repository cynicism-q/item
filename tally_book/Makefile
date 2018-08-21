#提取当前工作目录，WORK_PATH就是一个变量，变量放的是当前的目录，
#Makefile里面可以定义变量，没有类型，直接使用，等号两边最好不要
#有空格,shell的方式把pwd执行了，然后把结果给了变量WORK_PATH,
#需要提取变量的值$变量名
WORK_PATH=$(shell_pwd)
#生成的可执行程序叫httpd
BIN=httpd
#依赖的源文件
SRC=httpd.c
#编译器
CC=gcc
#gcc的-D选项用来在编译时指定我们的宏，指定DEBUG宏
LDFLAGS=-lpthread -DDEBUG   

#执行多个makefile
.PHONY:all
all:$(BIN) cgi

$(BIN):$(SRC)
	@$(CC) -o $@ $^ $(LDFLAGS)
#到这个目录下执行在返回
cgi:
	@cd wwwroot/cgi; make clean;make; cd -
	@cd connectSql; make clean; make; cd -

.PHONY:output
output:
	@mkdir -p output/wwwroot/cgi
	@cp $(BIN) output
	@cp start.sh  output
	@cp wwwroot/*.html output/wwwroot
	@cp -rf  wwwroot/imag output/wwwroot
	@cp wwwroot/cgi/math_cgi output/wwwroot/cgi
	@cp connectSql/insert_cgi output/wwwroot/cgi
	@cp connectSql/select_cgi output/wwwroot/cgi
	@cp -rf lib/lib output/

.PHONY:clean
clean:
	@rm -f $(BIN)
	@rm -rf output
	@cd wwwroot/cgi; make clean; cd -
