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
#define FFQ_FREE	0

struct ffq {
	int ffq_head;
	int ffq_tail;

	u_int64_t ffq_buffer[FFQ_LEN];
};

int	ffq_enqueue(struct ffq *, u_int64_t);
int	ffq_dequeue(struct ffq *, u_int64_t *);
void	ffq_slip(struct ffq *);

int
ffq_enqueue(struct ffq *ffq, u_int64_t data)
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
ffq_dequeue(struct ffq *ffq, u_int64_t *data)
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

void *stage1(void *);
void *stage2(void *);

#define TESTN 10000000

void *
stage1(void *ffq0)
{
	struct ffq *ffq = ffq0;
	u_int64_t data;

	for (data = 1; data <= TESTN; data++) {
again:
		if (ffq_enqueue(ffq, data) == EWOULDBLOCK)
			goto again;
		if (data > TESTN)
			errx(1, "data is funny !!!!!! (%llu)", data);
	}
	return (NULL);
}

void *
stage2(void *ffq0)
{
	struct ffq *ffq = ffq0;
	u_int64_t data = 0;

	while (data != TESTN) {
		if (ffq_dequeue(ffq, &data) == EWOULDBLOCK)
			continue;
		printf("data = %llu\n", data);
	}
	return (NULL);
}

int
main(int argc, char *argv[])
{
	struct ffq	ffq;
	pthread_t	stage1_tid;
	pthread_t	stage2_tid;

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
