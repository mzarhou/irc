NAME = ircserv

ODIR = build
SRCS = command.cpp context.cpp irc_user.cpp main.cpp str-utils.cpp
OBJS = ${patsubst %.cpp, ${ODIR}/%.o, ${SRCS}}
HEADERS = command.hpp context.hpp irc_user.hpp str-utils.hpp

INC =
# FLAGS = -Wall -Wextra -Werror -std=c++98
FLAGS =
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
