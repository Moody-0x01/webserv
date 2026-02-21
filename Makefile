CXX=c++
SOURCE_DIR=./src
NAME=webserv
SRCS=
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

# Docker Shit!
r:
	docker start webserv-container
	docker exec -it webserv-container zsh

re: fclean all
.PHONY: re fclean clean bonus $(NAME) r
