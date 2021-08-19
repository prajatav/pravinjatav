#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/delay.h>
#include<linux/workqueue.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ITtraining.com.tw");
MODULE_DESCRIPTION("A Simple Blocking IO device RaspPi");


struct __wait_queue {
	unsigned int flags;
	struct task_struct * task;
	struct list_head task_list;
};
typedef struct __wait_queue wait_queue_t;



void sleep_on(wait_queue_head_t *queue)
{
	wait_queue_t wait;  /* entry in the sleeping queue */

        init_waitqueue_entry(&wait, current);

        wq_write_lock_irqsave(&queue->lock, flags);
        __add_wait_queue(queue, &wait)
	wq_write_unlock(&queue->lock);

        current->state = TASK_UNINTERRUPTIBLE;
        schedule();

	wq_write_lock_irq(&queue->lock);
        __remove_wait_queue(queue, &wait);


	static wait_queue_head_t wq;    /* the readers wait queue */

static int __init dev_init(void)
{
        ...
        init_waitqueue_head(&wq);  /* initialize wait queue */
        ...
}

static ssize_t dev_read(struct file *filp, char *buf, size_t count, loff_t *offp)
{
        if (buffer_size == 0) {
                interruptible_sleep_on(&wq);  /* go to sleep, wait for writers */
		if (signal_pending(current))  /* woken up by a signal? */
			return(-EINTR);
	}
	/* send message to reader */
	count = (count>buffer_size) ? buffer_size : count;
	copy_to_user(buf, dev_buffer, count);
	return(count);
} /* dev_read() */

static ssize_t dev_write(struct file *filp, const char *buf, size_t count, loff_t *offp)
{
	/* store message in device buffer */
	count = (count>BUFMAX) ? BUFMAX : count;
	copy_from_user(dev_buffer, buf, count);
	buffer_size = count;
        wake_up_interruptible(&wq);  /* wake up readers */
	return(count);
}
