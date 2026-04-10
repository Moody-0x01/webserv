CXX=c++
SOURCE_DIR=./src
NAME=./webserv
OBJDIR = .build
OBJS=$(SRCS:%.cpp=$(OBJDIR)/%.o)
INCLUDE=./include/
CXXFLAGS=-Wall -Wextra -Werror -std=c++98 -I$(INCLUDE) -g3 -fsanitize=address 
RM=rm -rf
MAIN=$(SOURCE_DIR)/main.cpp
# $(SOURCE_DIR)/multiplexing/SocketHandlers.cpp removed
SRCS=$(SOURCE_DIR)/multiplexing/Multiplexer.cpp $(SOURCE_DIR)/multiplexing/ASocketContext.cpp \
	$(SOURCE_DIR)/multiplexing/Server.cpp $(SOURCE_DIR)/multiplexing/Client.cpp\
	$(SOURCE_DIR)/HTTP/Parser/Lexer.cpp \
	$(SOURCE_DIR)/HTTP/Parser/HttpParser.cpp  $(SOURCE_DIR)/HTTP/Request.cpp $(SOURCE_DIR)/HTTP/Response.cpp \
	$(SOURCE_DIR)/HTTP/Resource.cpp \
	$(SOURCE_DIR)/Config/Lexer.cpp $(SOURCE_DIR)/Config/ConfigParser.cpp\
	$(SOURCE_DIR)/multiplexing/Cgi.cpp


all: $(NAME)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAME): $(OBJS) $(MAIN)
	$(CXX) $(CXXFLAGS) $(OBJS) $(MAIN) -o $@

clean:
	$(RM) $(OBJS)

fclean: clean
	$(RM) $(NAME)
	$(RM) $(OBJDIR)

run: $(NAME)
	./$(NAME)

re: fclean all

.PHONY: re fclean clean bonus $(NAME) run
