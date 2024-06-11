#定义编译器
cc=g++

#编译目标
TARGETS = parser http_server

#生成全部目标
all: $(TARGETS)

#生成parser
parser:parser.cc
	$(cc) -o $@ $^ -lboost_system -lboost_filesystem -std=c++11

#生成http_server
http_server:http_server.cc
	$(cc) -o $@ $^ -ljsoncpp -lpthread -std=c++11

#清理
.PNONY:clean
clean:
	rm -f parser
	rm -f http_server
