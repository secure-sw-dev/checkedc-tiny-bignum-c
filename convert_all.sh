3c -alltypes -warn-root-cause \
-output-dir=out \
./bn.c \
tests/hand_picked.c \
tests/rsa.c \
tests/factorial.c \
tests/load_cmp.c \
tests/test_div_algo.c \
tests/golden.c \
tests/randomized.c \
-- -Wall -Wextra -I.
