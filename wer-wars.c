/*
 * $Id: wer-wars.c,v 1.5 2012/08/18 07:44:23 urs Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static void usage(const char *name)
{
	fprintf(stderr, "Usage: %s n\n", name);
}

enum die { D_1 = 1, D_2, D_3, D_123, D_CLOCK, D_GHOST };
enum state { CLOCK, JUMP, PEEK, UNKNOWN, KNOWN, OPEN };

static void play(int n);
static int  find(const enum state *a, int size,
		 enum state what, int pos, int steps);
static void swap(enum state *a, enum state *b);
static enum die die(void);
static int guess(int nunknown);
static int rnd(int min, int max);

int main(int argc, char **argv)
{
	if (argc != 2) {
		usage(argv[0]);
		exit(1);
	}

	int n = atoi(argv[1]);

	play(n);

	return 0;
}

static void play(int n)
{
	int nunknown = n;
	int m = n + 2;
	enum state a[m + 1];
	int clock = 0;
	int pos = 0, new, peek, sw, f, f2;
	int i;

	for (i = 0; i < sizeof(a) / sizeof(a[0]); i++)
		a[i] = UNKNOWN;

	a[0]     = CLOCK;
	a[m/3]   = JUMP;
	a[2*m/3] = PEEK;

	while (nunknown > 1) {
		for (i = 0; i < sizeof(a) / sizeof(a[0]); i++)
			printf(" %d", a[i]);
		putchar('\n');
		enum die d = die();
		printf("die: %d\n", d);
		switch (d) {
		case D_CLOCK:
			clock++;
			printf("clock: %d\n", clock);
			break;
		case D_GHOST:
			f  = find(a, m, UNKNOWN, pos + 1, m);
			f2 = find(a, m, KNOWN, pos + 1, m);
			if (f < 0 || f2 >=0 && f2 < f)
				f = f2;
			sw = (pos + 1 + f) % m;
			printf("swap %d,%d\n", sw, m);
			swap(&a[sw], &a[m]);
			break;
		case D_123:
			if ((f = find(a, m, KNOWN, pos + 1, 3)) >= 0)
				d = f + 1;
			else if ((f = find(a, m, JUMP, pos + 1, 3)) >= 0)
				d = f + 1;
			else if ((f = find(a, m, PEEK, pos + 1, 3)) >= 0)
				d = f + 1;
			else if ((f = find(a, m, UNKNOWN, pos + 1, 3)) >= 0)
				d = f + 1;
			else if ((f = find(a, m, OPEN, pos + 1, 3)) >= 0)
				d = f + 1;
			else
				assert(0);
		case D_1:
		case D_2:
		case D_3:
			new = (pos + d) % m;
			printf("walk %d -> %d\n", pos, new);
		jump:
			pos = new;
			switch (a[pos]) {
			case CLOCK:
				clock++;
				printf("clock: %d\n", clock);
				break;
			case JUMP:
				if ((f = find(a, m, KNOWN, pos + 1, m)) >= 0)
					new = (pos + 1 + f) % m;
				else if ((f = find(a, m, PEEK, pos + 1, m)) >= 0)
					new = (pos + 1 + f) % m;
				else
					assert(0);
				printf("jump %d -> %d\n", pos, new);
				goto jump;
			case PEEK:
				f = find(a, m, UNKNOWN, pos + 1, m);
				if (f < 0)
					break;
				peek = (pos + 1 + f) % m;
				a[peek] = KNOWN;
				nunknown--;
				printf("peek %d, left %d\n", peek, nunknown);
				break;
			case UNKNOWN:
				printf("guess %d: ", pos);
				if (guess(nunknown)) {
					a[pos] = OPEN;
					nunknown--;
					printf("ok, left %d\n", nunknown);
				} else {
					a[pos] = KNOWN;
					nunknown--;
					clock++;
					printf("clock %d, left %d\n",
					       clock, nunknown);
				}
				break;
			case KNOWN:
				a[pos] = OPEN;
				break;
			case OPEN:
				break;
			}
			break;
		}
		putchar('\n');
	}

	printf("done\n");
}

static int find(const enum state *a, int size,
		enum state what, int pos, int steps)
{
	int i;

	for (i = 0; i < steps; i++)
		if (a[(pos + i) % size] == what)
			return i;
	return -1;
}

static void swap(enum state *a, enum state *b)
{
	enum state tmp;

	tmp = *a, *a = *b, *b = tmp;
}

static enum die die(void)
{
	return rnd(D_1, D_GHOST);
}

static int guess(int nunknown)
{
	return rnd(0, nunknown - 1) == 0;
}

static int rnd(int min, int max)
{
	return min + rand() % (max + 1 - min);
}
