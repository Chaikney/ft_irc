## **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: chaikney <marvin@42.fr>                    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2023/04/20 11:36:56 by chaikney          #+#    #+#              #
#    Updated: 2023/05/17 15:40:23 by chaikney         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

BUILD_DIR = ../build
SRC_DIR	=	source

NAME	=	$(BUILD_DIR)/ircserv

# NOTE addprefix allows us to add the path before each file, without needing a wildcard
# i.e. we still explicitly list our source files
SRC		=	$(addprefix $(SRC_DIR)/, main.cpp \
			Server.cpp \
			Message.cpp \
			User.cpp \
			Channel.cpp \
			ACommand.cpp \
			Ping.cpp \
			ListCmd.cpp \
			Privmsg.cpp \
			KickCmd.cpp \
			Join.cpp \
			Part.cpp \
			Names.cpp \
			Topic.cpp \
			Invite.cpp \
			Mode.cpp \
			Away.cpp \
			Who.cpp \
			Whois.cpp \
			Userhost.cpp \
			Pass.cpp \
			UserCmd.cpp \
			Nick.cpp \
			QuitCmd.cpp )

# Define a folder for the intermediate files to keep it clean
OBJ_DIR = ../obj
OBJ		= $(SRC:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# NOTE make built-in rules no longer work, so must explicitly
# say where the object files are
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# NOTE -Weffc++ adds stricter C++ compilation warnings
# -Iinclude says to get the headers from ./include
CFLAGS = -Iinclude -Werror -Wall -Wextra -ggdb -std=c++98 -O0 -Weffc++
CC		= c++

all: $(BUILD_DIR) $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

# if the obj dir does not exist, create it
$(OBJ_DIR):
	mkdir -p $@

# if the build dir does not exist, create it
$(BUILD_DIR):
	mkdir -p $@
#
# Create the executable statically
static: $(BUILD_DIR) $(OBJ)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJ) -static

.PHONY: clean, all, fclean, re

clean:
	/bin/rm -f $(OBJ_DIR)/*.o
	/bin/rm -f __.SYMDEF
	/bin/rm -rf *.dSYM

fclean: clean
	/bin/rm -f $(NAME)

re: fclean all
