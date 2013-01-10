/*
 * $Id: wer-wars.c,v 1.14 2013/01/10 02:49:46 urs Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>

static void usage(const char *name)
{
	fprintf(stderr, "Usage: %s [-c count] [-l limit] [-n n]\n", name);
}

enum die { D_1 = 1, D_2, D_3, D_123, D_CLOCK, D_GHOST };
enum state { CLOCK, JUMP, PEEK, UNKNOWN, KNOWN, OPEN };

static void play(int n, int limit);
static void setup(enum state *a, int size);
static void print(const enum state *a, int size);
static int  select_123(const enum state *a, int size, int pos);
static int  select_jump(const enum state *a, int size, int pos);
static int  select_peek(const enum state *a, int size, int pos);
static int  find_swap(const enum state *a, int size, int pos);
static int  find(const enum state *a, int size,
		 enum state what, int pos, int steps);
static void swap(enum state *a, enum state *b);
static void init_die(unsigned int seed);
static enum die die(void);
static void init_guess(unsigned int seed);
static int guess(int nunknown);
static int rnd(int min, int max, unsigned int *seedp);

int main(int argc, char **argv)
{
	int n     = 12;
	int limit = 12;
	int count = 1;
	int opt, i;
	int errflg = 0;

	while ((opt = getopt(argc, argv, "c:l:n:")) != -1) {
		switch (opt) {
		case 'c':
			count = atoi(optarg);
			break;
		case 'l':
			limit = atoi(optarg);
			break;
		case 'n':
			n = atoi(optarg);
			break;
		default:
			errflg = 1;
			break;
		}
	}

	if (errflg || argc != optind) {
		usage(argv[0]);
		exit(1);
	}

	for (i = 0; i < count; i++) {
		printf("---- game %d ----\n", i);
		init_die(i);
		init_guess(i);
		play(n, limit);
	}

	return 0;
}

static void play(int n, int limit)
{
	/* We have n fields with symbols to be revealed, plus CLOCK,
	 * JUMP, and PEEK.  One of the n fields is at the GHOST and is
	 * not part of the round trip.
	 */
	int size     = n + 3;
	int len      = size - 1;
	/* Initially, all n symbols are unknown and we have to open
	 * all fields except the one at the GHOST.
	 */
	int nunknown = n;
	int left     = n - 1;
	enum state a[size];
	int clock = 0;
	int pos = 0, new, peek, sw;

	setup(a, size);

	while (left > 0 && clock < limit) {
		print(a, size);

		enum die d = die();

		switch (d) {
		case D_CLOCK:
			printf("die: CLOCK\n");
			break;
		case D_GHOST:
			printf("die: GHOST\n");
			break;
		case D_123:
			printf("die: 123\n");
			break;
		case D_1:
		case D_2:
		case D_3:
			printf("die: %d\n", d);
			break;
		}

		switch (d) {
		case D_CLOCK:
			clock++;
			printf("clock: %d\n", clock);
			break;
		case D_GHOST:
			sw = find_swap(a, len, pos);
			printf("swap %d,%d\n", sw, size - 1);
			swap(&a[sw], &a[size - 1]);
			break;
		case D_123:
			d = select_123(a, len, pos);
		case D_1:
		case D_2:
		case D_3:
			new = (pos + d) % len;
			printf("walk %d -> %d\n", pos, new);
		jump:
			pos = new;
			switch (a[pos]) {
			case CLOCK:
				clock++;
				printf("clock: %d\n", clock);
				break;
			case JUMP:
				new = select_jump(a, len, pos);
				printf("jump %d -> %d\n", pos, new);
				goto jump;
			case PEEK:
				peek = select_peek(a, len, pos);
				if (peek < 0)
					break;
				a[peek] = KNOWN;
				nunknown--;
				printf("peek %d: unknown %d, left %d\n",
				       peek, nunknown, left);
				break;
			case UNKNOWN:
				printf("guess %d: ", pos);
				if (guess(nunknown)) {
					a[pos] = OPEN;
					nunknown--;
					left--;
					printf("ok, unknown %d, left %d\n",
					       nunknown, left);
				} else {
					a[pos] = KNOWN;
					nunknown--;
					clock++;
					printf("clock %d, unknown %d, left %d\n",
					       clock, nunknown, left);
				}
				break;
			case KNOWN:
				a[pos] = OPEN;
				left--;
				printf("open %d: left %d\n", pos, left);
				break;
			case OPEN:
				break;
			}
			break;
		}
		putchar('\n');
	}

	printf("done: clock %d, left %d\n", clock, left);
}

static void setup(enum state *a, int size)
{
	int len = size - 1;
	int i;

	for (i = 0; i < size; i++)
		a[i] = UNKNOWN;

	a[0]       = CLOCK;
	a[len/3]   = JUMP;
	a[2*len/3] = PEEK;
}

static void print(const enum state *a, int size)
{
	static const char sym[] = "CJP?-+";
	int i;

	for (i = 0; i < size; i++)
		printf(" %c", sym[a[i]]);
	putchar('\n');
}

static int select_123(const enum state *a, int len, int pos)
{
	int f, d;

	if ((f = find(a, len, KNOWN, pos + 1, 3)) >= 0)
		d = f + 1;
	else if ((f = find(a, len, JUMP, pos + 1, 3)) >= 0)
		d = f + 1;
	else if ((f = find(a, len, PEEK, pos + 1, 3)) >= 0)
		d = f + 1;
	else if ((f = find(a, len, UNKNOWN, pos + 1, 3)) >= 0)
		d = f + 1;
	else if ((f = find(a, len, OPEN, pos + 1, 3)) >= 0)
		d = f + 1;
	else
		assert(0);

	return d;
}

static int select_jump(const enum state *a, int len, int pos)
{
	int f, new;

	if ((f = find(a, len, KNOWN, pos + 1, len)) >= 0)
		new = (pos + 1 + f) % len;
	else if ((f = find(a, len, PEEK, pos + 1, len)) >= 0)
		new = (pos + 1 + f) % len;
	else
		assert(0);

	return new;
}

static int select_peek(const enum state *a, int len, int pos)
{
	int f = find(a, len, UNKNOWN, pos + 1, len);

	return f < 0 ? -1 : (pos + 1 + f) % len;
}

static int find_swap(const enum state *a, int len, int pos)
{
	int sw;
	int f1 = find(a, len, UNKNOWN, pos + 1, len);
	int f2 = find(a, len, KNOWN, pos + 1, len);

	assert(f1 >= 0 || f2 >= 0);
	assert(f1 != f2);

	if (f2 < 0 || f1 >= 0 && f1 < f2)
		sw = (pos + 1 + f1) % len;
	else
		sw = (pos + 1 + f2) % len;
	return sw;
}

static int find(const enum state *a, int len,
		enum state what, int pos, int steps)
{
	int i;

	for (i = 0; i < steps; i++)
		if (a[(pos + i) % len] == what)
			return i;
	return -1;
}

static void swap(enum state *a, enum state *b)
{
	enum state tmp;

	tmp = *a, *a = *b, *b = tmp;
}

static unsigned int dseed = 1;
static void init_die(unsigned int seed)
{
	dseed = seed;
}

static enum die die(void)
{
	return rnd(D_1, D_GHOST, &dseed);
}

static unsigned int gseed = 1;
static void init_guess(unsigned int seed)
{
	gseed = seed;
}

static int guess(int nunknown)
{
	return rnd(0, nunknown - 1, &gseed) == 0;
}

static int rnd(int min, int max, unsigned int *seedp)
{
	return min + rand_r(seedp) % (max + 1 - min);
}
