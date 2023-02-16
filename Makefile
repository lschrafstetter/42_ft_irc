RED			= \033[0m\033[91m
GREEN		= \033[0m\033[92m
UNDO_COL	= \033[0m
CC			= c++
CFLAGS		= -Wall -Werror -Wextra -g -std=c++98
RM			= rm -rf
NAME		= ircserv

SRCDIR		= srcs/
SRC			= main.cpp Server_run.cpp Server.cpp Client.cpp Server_client_functions.cpp helpers.cpp Channel.cpp Serverfunctions.cpp

INCL_NAME	= include.hpp Server.hpp Client.hpp Channel.hpp
INCLUDES	= $(addprefix $(SRCDIR), $(INCL_NAME))

OBJDIR		= obj/
OBJ_NAME	= $(patsubst %.cpp,%.o,$(SRC))
OBJS		= $(addprefix $(OBJDIR), $(OBJ_NAME))

all:	$(NAME)

$(NAME):	$(OBJDIR) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)SUCCESSFULLY CREATED IRCSERV!$(UNDO_COL)"

$(OBJDIR)%.o:	$(SRCDIR)%.cpp $(INCLUDES)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir obj

clean:
	$(RM) $(OBJDIR)

fclean:	clean
	$(RM) $(NAME)
	@echo "$(RED)Finished cleaning up$(UNDO_COL)"

re:	fclean all

.PHONY:	all clean fclean re
