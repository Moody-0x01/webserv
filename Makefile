CXX=c++
SOURCE_DIR=./src
NAME=./webserv
SRCS=./src/multiplexing/Multiplexer.cpp ./src/multiplexing/SocketContext.cpp  ./src/multiplexing/SocketHandlers.cpp
OBJS=$(SRCS:%.cpp=%.o)
INCLUDE=./include/
CXXFLAGS=-Wall -Wextra -Werror -std=c++98 -I$(INCLUDE)
MAIN=$(SOURCE_DIR)/main.cpp
RM=rm -rf

all: $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS) $(MAIN)
	$(CXX) $(CXXFLAGS) $(OBJS) $(MAIN) -o $@
clean:
	$(RM) $(OBJS)
fclean: clean
	$(RM) $(NAME)
re: fclean all
run: all
	$(NAME)

.PHONY: re fclean clean bonus $(NAME)
