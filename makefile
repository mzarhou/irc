NAME = ircserv

ODIR = build
SRCS = main.cpp
OBJS = ${patsubst %.cpp, ${ODIR}/%.o, ${SRCS}}
HEADERS =
INC =
FLAGS = -Wall -Wextra -Werror -std=c++98
CC = c++ ${FLAGS}

all: ${NAME}

${ODIR}/%.o: %.cpp ${HEADERS}
	@mkdir -p ${@D}
	${CC} -c $< -o $@ ${INC}

${NAME}: ${OBJS}
	${CC} ${OBJS} -o ${NAME}

clean:
	rm -rf ${ODIR}

fclean: clean
	rm -rf ${NAME}

re: fclean all

.PHONY: clean fclean re all
