NAME = ircserv

ODIR = build
SRCS = channel.cpp command.cpp context.cpp irc_user.cpp main.cpp server.cpp str-utils.cpp

OBJS = ${patsubst %.cpp, ${ODIR}/%.o, ${SRCS}}
HEADERS = channel.hpp command.hpp context.hpp errors.hpp irc_user.hpp server.hpp str-utils.hpp


INC =
FLAGS = -Wall -Wextra -Werror -std=c++98
# FLAGS = -Wall -Wextra -Werror -std=c++98 -fsanitize=address
# FLAGS = -g -fsanitize=address
# FLAGS = -g

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
