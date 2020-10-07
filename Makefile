CC := gcc
CFLAGS := -Wall -g

main_target := master
sub_target := palin
targets := $(main_target) $(sub_target)

main_dependencies := master.c $(sub_target)
sub_dependencies := palin.c

$(main_target): $(main_dependencies)
	$(CC) $(CFLAGS) $< -o $@

$(sub_target): $(sub_dependencies)
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(targets) *.out
