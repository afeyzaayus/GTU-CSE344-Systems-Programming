NAME = myFind

CC = gcc
CFLAGS = -Wall -Wextra -Werror

SRC = 	main.c \
		arguments.c

OBJ = $(SRC:.c=.o)

all: $(NAME)

$(NAME): $(OBJ)
		$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

clean:
		rm -rf $(OBJ)

fclean: clean
		rm -rf $(OBJ)
		rm -rf $(NAME)

re: fclean all

.PHONY: all fclean clean re


