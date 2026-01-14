# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: nicolive <nicolive@student.42belgium.be    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/06/06 12:20:22 by nicolive          #+#    #+#              #
#    Updated: 2026/01/13 13:49:21 by nicolive         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


NAME = webserv

SRC_DIR     = src
INC_DIR     = inc
OBJ_DIR     = obj

CC          = c++
CPPFLAGS    = -Wall -Wextra -Werror -std=c++98 -pedantic
INCS        = -I$(INC_DIR) -I.

SRC         = \
	$(SRC_DIR)/main.cpp \
	$(SRC_DIR)/config/Config.cpp \
	$(SRC_DIR)/config/ConfigChecks.cpp \
	$(SRC_DIR)/config/ConfigParse.cpp \
	$(SRC_DIR)/config/ConfigUtils.cpp \
	$(SRC_DIR)/config/getterConfig.cpp \
	$(SRC_DIR)/utils/utils_transform.cpp \
	$(SRC_DIR)/Server/CGI.cpp \
	$(SRC_DIR)/Server/cgiResponse.cpp \
	$(SRC_DIR)/Server/ChooseServerBlock.cpp \
	$(SRC_DIR)/Server/delete.cpp \
	$(SRC_DIR)/Server/FindKeys.cpp \
	$(SRC_DIR)/Server/FindLocationBlock.cpp \
	$(SRC_DIR)/Server/get.cpp \
	$(SRC_DIR)/Server/HandleRequest.cpp \
	$(SRC_DIR)/Server/InitServer.cpp \
	$(SRC_DIR)/Server/IsMethodAllowed.cpp \
	$(SRC_DIR)/Server/Path.cpp \
	$(SRC_DIR)/Server/post.cpp \
	$(SRC_DIR)/Server/ReadFromClient.cpp \
	$(SRC_DIR)/Server/Responses.cpp \
	$(SRC_DIR)/Server/RunServer.cpp \
	$(SRC_DIR)/Server/Serverutils.cpp \
	$(SRC_DIR)/Server/WriteToClient.cpp

OBJ         = $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS        = $(OBJ:.o=.d)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CPPFLAGS) $(OBJ) -o $(NAME)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(INCS) -MMD -MP -c $< -o $@

-include $(DEPS)

run: all
	./$(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all run clean fclean re
