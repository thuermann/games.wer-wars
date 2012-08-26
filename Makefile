#
# $Id: Makefile,v 1.1 2012/08/26 10:53:02 urs Exp $
#

RM = rm -f

programs = wer-wars

.PHONY: all
all: $(programs)

.PHONY: clean
clean:
	$(RM) $(programs) *.o core
