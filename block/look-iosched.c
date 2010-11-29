/*
 * elevator look
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

#define FWD 1 //This is "next" on the list
#define REV 2 //This is "prev" on the list

struct look_data{
	struct * look_queue queue;
	int dir;
	sector_t head_pos;
};

struct look_queue {
	struct list_head queue;
	sector_t beg_pos;
	struct request *rq;
	struct * look_data look_metadata;
};

static void look_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static void look_put_req_fn(struct request_queue *q, struct request *rq)
{
	struct look_queue *nd;
        rq->elevator_private = kmalloc(sizeof(*nd), GFP_KERNEL);
	rq->elevator_private2 = NULL;
}

static void look_set_req_fn(struct request_queue *q, struct request *rq)
{
	rq->elevator_private = kfree();
        rq->elevator_private2 = NULL;

}

/*
 * TODO:
 * I/O schedulers are free to postpone requests by
	not filling the dispatch queue unless @force
	is non-zero.  Once dispatched, I/O schedulers
	are not allowed to manipulate the requests -
	they belong to generic dispatch queue.
 */ 

static int look_dispatch(struct look_queue *q, int force)
{
	struct look_data *nd = q->look_metadata;

	if (!list_empty(&nd->queue)) {
		struct request *rq;
		// Change the below line to grab the appropriate node (either next OR prev, depending on dir)
		rq = list_entry(nd->queue.next, struct request, queuelist);
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		// Move the head to the appropriate position based on head_pos
		return 1;
	}
	return 0;
}

static void look_add_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

    /*Allocate a new look_node for the request, and initialize it */
    struct look_queue *new = kmalloc(sizeof(struct look_queue), GFP_KERNEL)
    INIT_LIST_HEAD(&new->queue);
    new->rq = rq;
    new->beg_pos = rq->bio->bi_sector;
    new->look_metadata = nd;

    struct look_queue *pos, *next;
    if( new->beg_pos > nd->head_position ) {

        /* The new request is after the current head position, search forward */
        list_for_each_entry(pos, &nd, queue)
	    {
            /* If we are at the end of the list, insert here */
            if( pos->queue->next == nd->queue )
            {
                list_add( &new->queue, &pos->queue );
                break;
            }
            
            /* We are not at the end of the list, fetch the next entry */
            next = list_entry( &new->queue->next, struct look_queue, queue );

            /* If pos < new < next, insert here */
            if( pos->beg_pos < new->beg_pos &&  new->beg_pos < next->beg_pos )
            {
                list_add( &new->queue, &pos->queue );
                break;
            }

            /* If next < pos, then we have reached the end of this side of the queue, insert here */
            if( next->beg_pos < pos->beg_pos )
            {
                list_add( &new->queue, &pos->queue );
                break;
            }
	    }
    } else {
        /* The new request is before the current head position, search backwards */
	    list_for_each_entry_reverse(c, &nd, queue)
        {
	       //TODO 
        {
    }
	
}

static int look_queue_empty(struct request_queue *q)
{
	struct look_data *nd = q->elevator->elevator_data;

	return list_empty(&nd->queue);
}

static struct request *
look_former_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
look_latter_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static void *look_init_queue(struct request_queue *q)
{
	struct look_data *nd;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	INIT_LIST_HEAD(&nd->queue);
	return nd;
}

static void look_exit_queue(struct elevator_queue *e)
{
	struct look_data *nd = e->elevator_data;
kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_look = {
	.ops = {
		.elevator_merge_req_fn		= look_merged_requests,
		.elevator_dispatch_fn		= look_dispatch,
		.elevator_add_req_fn		= look_add_request,
		.elevator_queue_empty_fn	= look_queue_empty,
		.elevator_former_req_fn		= look_former_request,
		.elevator_latter_req_fn		= look_latter_request,
		.elevator_init_fn		= look_init_queue,
		.elevator_exit_fn		= look_exit_queue,
	},
	.elevator_name = "look",
	.elevator_owner = THIS_MODULE,
};

static int __init look_init(void)
{
	elv_register(&elevator_look);

	return 0;
}

static void __exit look_exit(void)
{
	elv_unregister(&elevator_look);
}

module_init(look_init);
module_exit(look_exit);


MODULE_AUTHOR("Alex Weeks,Josh Jordahl, Kevin McIntosh, Tyler McLung ");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Look IO scheduler");
