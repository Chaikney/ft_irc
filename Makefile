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

NAME	=	ircserv

SRC		=	main.cpp \
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
			Names.cpp

OBJ		= $(SRC:.cpp=.o)

# NOTE -Weffc++ adds stricter C++ compilation warnings
CFLAGS = -Werror -Wall -Wextra -ggdb -std=c++98 -O0 -Weffc++
CC		= c++

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean, all, fclean, re

clean:
	/bin/rm -f *.o
	/bin/rm -f __.SYMDEF
	/bin/rm -rf *.dSYM

fclean: clean
	/bin/rm -f $(NAME)

re: fclean all
