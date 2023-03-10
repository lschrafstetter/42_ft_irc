RED			= \033[0m\033[91m
GREEN		= \033[0m\033[92m
UNDO_COL	= \033[0m
CC			= c++
CFLAGS		= -Wall -Werror -Wextra -std=c++98 -pedantic
RM			= rm -rf
NAME		= chatbot

SRCDIR		= srcs/
SRC			= main.cpp Chatbot.cpp Chatbot_init.cpp Chatbot_run.cpp helpers.cpp

INCL_NAME	= include.hpp
INCLUDES	= $(addprefix $(SRCDIR), $(INCL_NAME))

OBJDIR		= obj/
OBJ_NAME	= $(patsubst %.cpp,%.o,$(SRC))
OBJS		= $(addprefix $(OBJDIR), $(OBJ_NAME))

all:	$(NAME)

$(NAME):	$(OBJDIR) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(NAME)
	@echo "$(GREEN)SUCCESSFULLY CREATED CHATBOT!$(UNDO_COL)"

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
