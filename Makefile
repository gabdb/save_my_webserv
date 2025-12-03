# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: nicolive <nicolive@student.s19.be>         +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/06/06 12:20:22 by nicolive          #+#    #+#              #
#    Updated: 2025/11/13 13:43:54 by nicolive         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME 		= webserv

SRC_DIR 	= src
INC_DIR		= inc
OBJ_DIR 	= obj

SRC 		= 	$(SRC_DIR)/main.cpp \
				$(SRC_DIR)/config/Config.cpp $(SRC_DIR)/config/ConfigChecks.cpp $(SRC_DIR)/config/ConfigParse.cpp $(SRC_DIR)/config/getterConfig.cpp\
				$(SRC_DIR)/utils/utils_transform.cpp \
				$(SRC_DIR)/Server/InitServer.cpp $(SRC_DIR)/Server/RunServer.cpp $(SRC_DIR)/Server/CGI.cpp\
				\
				
				
OBJ			= $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS 		= $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.d)
CC 			= c++
RM 			= rm -f
CPPFLAGS 	= -Wall -Wextra -Werror -std=c++98 -pedantic
INCS 		= -I$(INC_DIR) -I.
	
all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CPPFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)/config
	@mkdir -p $(OBJ_DIR)/utils
	@mkdir -p $(OBJ_DIR)/Server
	
	$(CC) $(CPPFLAGS) $(INCS) -MMD -MP -c $< -o $@

-include $(DEPS)

run: all
	./$(NAME)

clean:
	$(RM) -r $(OBJ_DIR)

fclean: clean
	$(RM) -r $(OBJ_DIR)
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
