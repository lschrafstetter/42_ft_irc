RED			= \033[0m\033[91m
GREEN		= \033[0m\033[92m
UNDO_COL	= \033[0m
CC			= c++
CFLAGS		= -Wall -Werror -Wextra -std=c++98 -pedantic
RM			= rm -rf
NAME		= ircserv

SRCDIR		= srcs/
SRC			= main.cpp Server_run.cpp Server.cpp Client.cpp helpers.cpp Channel.cpp \
			  Server_authentication.cpp Server_welcome.cpp Server_join.cpp Server_privmsg.cpp \
			  Server_topic.cpp Server_mode.cpp Server_errors.cpp Server_quit.cpp Server_oper.cpp \
			  Server_replies.cpp Server_invite.cpp

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
