/*
 * Copyright (c) 2013 Christiano F. Haesbaert <haesbaert@haesbaert.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define FFQ_LEN 	1024
#define FFQ_FREE	NULL

struct ffq {
	int ffq_head;
	int ffq_tail;

	void *ffq_buffer[FFQ_LEN];
};

int	ffq_enqueue(struct ffq *, void *);
int	ffq_dequeue(struct ffq *, void **);
void	ffq_slip(struct ffq *);

int
ffq_enqueue(struct ffq *ffq, void *data)
{
	if (data == FFQ_FREE)
		errx(1, "whoops got data as FFQ_FREE");

	if (ffq->ffq_buffer[ffq->ffq_head] != FFQ_FREE)
		return (EWOULDBLOCK);

	ffq->ffq_buffer[ffq->ffq_head] = data;
	ffq->ffq_head = (ffq->ffq_head + 1) % FFQ_LEN;

	return (0);
}

int
ffq_dequeue(struct ffq *ffq, void **data)
{
	*data = ffq->ffq_buffer[ffq->ffq_tail];

	if (*data == FFQ_FREE)
		return (EWOULDBLOCK);

	ffq->ffq_buffer[ffq->ffq_tail] = FFQ_FREE;
	ffq->ffq_tail = (ffq->ffq_tail + 1) % FFQ_LEN;

	return (0);
}

void
ffq_slip(struct ffq *ffq)
{
	/* TODO */
}


/*
 * Test
 */

#include <pthread.h>
#include <string.h>

#define TESTDEFN 250000000;

void __dead usage(void);
void *stage1(void *);
void *stage2(void *);

u_int64_t testn = TESTDEFN;	/* Number of entries in the system (-n) */

static inline u_int64_t
rdtsc(void)
{
	u_int32_t hi, lo;
	__asm __volatile("rdtsc" : "=d" (hi), "=a" (lo));

	return (((u_int64_t)hi << 32) | (u_int64_t)lo);
}

static inline void
busywait(u_int64_t cycles)
{
	u_int64_t x;

	x = rdtsc();

	while ((rdtsc() - x) < cycles)
		;
}

void *
stage1(void *ffq0)
{
	struct ffq *ffq = ffq0;
	u_int64_t data, last;
	u_int64_t blocks = 0;

	for (data = 1, last = 0;
	     data <= testn;
	     last = data, data++) {
again:
		if (ffq_enqueue(ffq, (void *)data) == EWOULDBLOCK) {
			blocks++;
			goto again;
		}
		
		if (data > testn)
			errx(1, "data is funny !!!!!! (%llu)", data);
		if ((data -1) != last)
			errx(1, "data/last mismatch data = %llu, last = %llu\n",
			    data, last);
	}
	printf("stage1 blocks: %llu\n", blocks);

	return (NULL);
}

void *
stage2(void *ffq0)
{
	struct ffq *ffq = ffq0;
	u_int64_t data = 0;
	u_int64_t blocks = 0;

	while (data != testn) {
		if (ffq_dequeue(ffq, (void *)&data) == EWOULDBLOCK) {
			blocks++;
			continue;
		}
	}
	printf("stage2 blocks: %llu\n", blocks);
	
	return (NULL);
}

void __dead
usage(void)
{
	fprintf(stderr,
	    "usage: ffq [-n nentries]\n"
	    "       ffq -h\n");
	exit(1);
}

int
main(int argc, char *argv[])
{
	struct ffq	ffq;
	pthread_t	stage1_tid;
	pthread_t	stage2_tid;
	const char 	*errstr;
	int 		c;

	while ((c = getopt(argc, argv, "n:")) != -1) {
		switch (c) {
			case 'n':
				testn = strtonum(optarg, 1, LLONG_MAX,
				    &errstr);
				if (errstr != NULL)
					errx(1, "Invalid value for n: %s",
					    errstr);
				break;
			case 'h':
			default:
				usage();
			}
	}

	bzero(&ffq, sizeof(ffq));

	if (pthread_create(&stage1_tid, NULL, stage1, &ffq) == -1)
		err(1, "pthread_create");
	if (pthread_create(&stage2_tid, NULL, stage2, &ffq) == -1)
		err(1, "pthread_create");

	if (pthread_join(stage1_tid, NULL) == -1)
		err(1, "pthread_join");
	if (pthread_join(stage2_tid, NULL) == -1)
		err(1, "pthread_join");

	return (0);
}
