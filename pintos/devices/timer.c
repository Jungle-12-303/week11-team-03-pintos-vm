#include "devices/timer.h"
#include <debug.h>
#include <inttypes.h>
#include <round.h>
#include <stdio.h>
#include "threads/interrupt.h"
#include "threads/io.h"
#include "threads/synch.h"
#include "threads/thread.h"

/* See [8254] for hardware details of the 8254 timer chip. */

#if TIMER_FREQ < 19
#error 8254 timer requires TIMER_FREQ >= 19
#endif
#if TIMER_FREQ > 1000
#error TIMER_FREQ <= 1000 recommended
#endif

/* Number of timer ticks since OS booted. */
static int64_t ticks;

/* Number of loops per timer tick.
   Initialized by timer_calibrate(). */
static unsigned loops_per_tick;

static intr_handler_func timer_interrupt;
static bool too_many_loops (unsigned loops);
static void busy_wait (int64_t loops);
static void real_time_sleep (int64_t num, int32_t denom);

/* Sets up the 8254 Programmable Interval Timer (PIT) to
   interrupt PIT_FREQ times per second, and registers the
   corresponding interrupt. */
void
timer_init (void) {
	/* 8254 input frequency divided by TIMER_FREQ, rounded to
	   nearest. */
	uint16_t count = (1193180 + TIMER_FREQ / 2) / TIMER_FREQ;

	outb (0x43, 0x34);    /* CW: counter 0, LSB then MSB, mode 2, binary. */
	outb (0x40, count & 0xff);
	outb (0x40, count >> 8);

	intr_register_ext (0x20, timer_interrupt, "8254 Timer");
}

/* Calibrates loops_per_tick, used to implement brief delays. */
/* 1 tick 동안 busy-wait 루프를 몇 번 돌 수 있는지 측정 
 * 1 tick보다 짧은 ms/us/ns 단위 지연은 타이머 tick으로 재기 애매하니, CPU 루프를 직접 돌림
 */
void
timer_calibrate (void) {
	unsigned high_bit, test_bit;

	ASSERT (intr_get_level () == INTR_ON);
	printf ("Calibrating timer...  ");

	/* Approximate loops_per_tick as the largest power-of-two
	   still less than one timer tick. */
	loops_per_tick = 1u << 10;
	while (!too_many_loops (loops_per_tick << 1)) {
		loops_per_tick <<= 1;
		ASSERT (loops_per_tick != 0);
	}

	/* Refine the next 8 bits of loops_per_tick. */
	high_bit = loops_per_tick;
	for (test_bit = high_bit >> 1; test_bit != high_bit >> 10; test_bit >>= 1)
		if (!too_many_loops (high_bit | test_bit))
			loops_per_tick |= test_bit;

	printf ("%'"PRIu64" loops/s.\n", (uint64_t) loops_per_tick * TIMER_FREQ);
}

/* 부팅 이후 누적된 timer tick 수 반환
 * timer_interrupt()가 ticks를 증가시키므로, 읽는 동안 인터럽트를 잠깐 꺼서 tick 값이 중간에 바뀌지 않게 보호
 * 
 * [추가] 왜 굳이 전역 ticks 변수를 읽지 않고, timer_ticks 함수를 사용하는가?
 * 	     >>> 그냥 전역 ticks를 직접 읽으면 race condition 가능성이 있기 때문
 */
int64_t
timer_ticks (void) {
	enum intr_level old_level = intr_disable (); // 현재 인터럽트 상태 저장 후 인터럽트 비활성화
	int64_t t = ticks; // 전역 tick 카운터를 지역 변수로 복사
	intr_set_level (old_level); // timer_ticks() 호출 전 인터럽트 상태로 복구
	barrier ();  // 컴파일러가 tick 읽기 순서를 재배치하지 못하게 제한
	return t; // 누적된 timer tick 수 반환
}

/* Returns the number of timer ticks elapsed since THEN, which
   should be a value once returned by timer_ticks(). */
int64_t
timer_elapsed (int64_t then) {
	return timer_ticks () - then; // `then` 시점부터 지금까지 지난 tick 수 계산
}

/* Suspends execution for approximately TICKS timer ticks. */
void
timer_sleep (int64_t ticks) {
	if (ticks <= 0) // sleep이 필요 없는 경우 리턴
		return;
	
	int64_t start = timer_ticks (); // 현재 ticks start에 저장
	
	enum intr_level old_level = intr_disable();

	struct thread *t = thread_current ();
	ASSERT (intr_get_level () == INTR_OFF); // 현재 인터럽트 상태가 꺼져있는 경우 통과

	t->wakeup_tick = start + ticks; // 현재 스레드 구조체 일어나야 할 시간에 일어나야 할 절대시간 대입
	insert_sleep(t);
	thread_block();
	intr_set_level(old_level); // 이전 인터럽트 상태로 되돌림
}

/* Suspends execution for approximately MS milliseconds. */
void
timer_msleep (int64_t ms) {
	real_time_sleep (ms, 1000);
}

/* Suspends execution for approximately US microseconds. */
void
timer_usleep (int64_t us) {
	real_time_sleep (us, 1000 * 1000);
}

/* Suspends execution for approximately NS nanoseconds. */
void
timer_nsleep (int64_t ns) {
	real_time_sleep (ns, 1000 * 1000 * 1000);
}

/* Prints timer statistics. */
void
timer_print_stats (void) {
	printf ("Timer: %"PRId64" ticks\n", timer_ticks ());
}

/* 8254 PIT 타이머 인터럽트 핸들러
 * TIMER_FREQ가 100이므로 기본 설정에서는 초당 100번 호출
 * 즉 1 tick은 약 10ms
 */
static void
timer_interrupt (struct intr_frame *args UNUSED) {
	/* 이미 INTR_OFF에서 동작하므로 인터럽트 걱정은 하지 않아도 됨 */
	ticks++;
	thread_tick ();
	thread_wakeup(ticks);
}

/* Returns true if LOOPS iterations waits for more than one timer
   tick, otherwise false. */
static bool
too_many_loops (unsigned loops) {
	/* Wait for a timer tick. */
	int64_t start = ticks;
	while (ticks == start)
		barrier ();

	/* Run LOOPS loops. */
	start = ticks;
	busy_wait (loops);

	/* If the tick count changed, we iterated too long. */
	barrier ();
	return start != ticks;
}

/* Iterates through a simple loop LOOPS times, for implementing
   brief delays.

   Marked NO_INLINE because code alignment can significantly
   affect timings, so that if this function was inlined
   differently in different places the results would be difficult
   to predict. */
static void NO_INLINE
busy_wait (int64_t loops) {
	while (loops-- > 0)
		barrier ();
}

/* Sleep for approximately NUM/DENOM seconds. */
static void
real_time_sleep (int64_t num, int32_t denom) {
	/* Convert NUM/DENOM seconds into timer ticks, rounding down.

	   (NUM / DENOM) s
	   ---------------------- = NUM * TIMER_FREQ / DENOM ticks.
	   1 s / TIMER_FREQ ticks
	   */
	int64_t ticks = num * TIMER_FREQ / denom;

	ASSERT (intr_get_level () == INTR_ON);
	if (ticks > 0) {
		/* We're waiting for at least one full timer tick.  Use
		   timer_sleep() because it will yield the CPU to other
		   processes. */
		timer_sleep (ticks);
	} else {
		/* Otherwise, use a busy-wait loop for more accurate
		   sub-tick timing.  We scale the numerator and denominator
		   down by 1000 to avoid the possibility of overflow. */
		ASSERT (denom % 1000 == 0);
		busy_wait (loops_per_tick * num / 1000 * TIMER_FREQ / (denom / 1000));
	}
}
