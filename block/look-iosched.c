/*
 * The Look Scheduler is an elevator-type IO Scheduler.  Requests are
 * dispatched based on the current sector, the requests sector, and the
 * direction that the arm is progressing
 * 
 * @Author: Alex Weeks, Kevin McIntosh, Tyler McClung, Josh Jordahl
 */ 

#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

#define FWD 1 //This is "next" on the list
#define REV 2 //This is "prev" on the list

static char get_dir(struct request * rq);
void look_print_queue( struct request_queue * q );

/**
* struct look_data - points to the queue of requests
* @queue: points to the queue of requests
* @dir: uses macros FWD and REV
* @head_pos: the end of the last dispatched request
*
*/ 
struct look_data{
	struct list_head queue;
	int dir;
	sector_t head_pos;
};

/**
* struct look_queue - holds the request
* @queue: a linked list node
* @beg_pos: beggining sector of the request
* @rq: the IO request
* @look_metadata
*/ 
struct look_queue {
	struct list_head queue;
	sector_t beg_pos;
	struct request *rq;
	struct look_data *look_metadata;
};

/**
* look_merged_requests - Merge requests
* @q: the request queue the holds these requests
* @rq: the first request
* @next: the next request
*
* Delete the request from the queue because it was merged
*/
static void look_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

/**
* look_put_req_fn - Free request
* @q: the request_queue
* @rq: the request
*
*/
static void look_put_req_fn(struct request_queue *q, struct request *rq)
{
	/*printk( "PUT %x\n", rq);*/
    kfree(rq->elevator_private);
}

/**
* look_put_req_fn - Allocate request
* @q: the request_queue
* @rq: the request
*
*/
static void look_set_req_fn(struct request_queue *q, struct request *rq)
{
	struct look_queue *new = kmalloc(sizeof(struct look_queue), GFP_KERNEL);
    
    /*printk( "SET %x\n", rq);*/

    INIT_LIST_HEAD(&new->queue);

    new->rq = rq;
    new->beg_pos = rq->bio->bi_sector;
    new->look_metadata = q->elevator->elevator_data;

	rq->elevator_private = new;
}

/*
 * TODO:
 * I/O schedulers are free to postpone requests by
	not filling the dispatch queue unless @force
	is non-zero.  Once dispatched, I/O schedulers
	are not allowed to manipulate the requests -
	they belong to generic dispatch queue.
 */ 

/**
* look_dispatch - sends the next request to the dispatch queue
* @q: scheduler queue
* @force: postpone requests
* 
* Requests are dispatched via "elevator" algorithm.  Returns success
*/
static int look_dispatch(struct request_queue *q, int force)
{
	struct look_data *ld = q->elevator->elevator_data;

	if (!list_empty(&ld->queue)) {
		struct look_queue *lq;
		if (ld->dir == FWD)
		{
			lq = list_entry(ld->queue.next, struct look_queue, queue);
			if (lq->beg_pos < ld->head_pos)
			{
				ld->dir = REV;
				lq = list_entry(ld->queue.prev, struct look_queue, queue);			
			}
		}
		else
		{
			lq = list_entry(ld->queue.prev, struct look_queue, queue);			
			if (lq->beg_pos > ld->head_pos)
			{
				ld->dir = FWD;
				lq = list_entry(ld->queue.prev, struct look_queue, queue);			
			}
		}

		printk( "[LOOK] DSP %c %u\n", get_dir(lq->rq), lq->beg_pos);
		
		list_del_init(&lq->queue);
		elv_dispatch_add_tail(q, lq->rq);
		ld->head_pos = lq->beg_pos + blk_rq_sectors(lq->rq) - 1;
		
		if (ld->dir == FWD)
		{
			struct look_queue *pos;
			list_for_each_entry(pos, &(ld->queue), queue)
			{
				if (pos->beg_pos >= ld->head_pos)
				{
					break;
				}
			}

			if (&ld->queue != pos->queue.prev)
			{
				list_move(&ld->queue, pos->queue.prev);
				printk( "MOVING SENTINEL.\n");
			}
		}

		look_print_queue(q);

		look_put_req_fn(q, lq->rq); 

		return 1;
	}
	return 0;
}

/**
* look_add_request - add request to IO Scheduler queue
* @q: the look queue
* @rq: the request to be added
*
*/
static void look_add_request(struct request_queue *q, struct request *rq)
{
    struct look_queue *pos, *next, *prev, *new;
    
    /* Set up look data structure */
    look_set_req_fn(q, rq); 
    new = rq->elevator_private;
    
    printk( "[LOOK] ADD %c %u\n", get_dir(new->rq), new->beg_pos);
    
    /* List is empty, add */
    if( list_empty(&new->look_metadata->queue) ) {
        list_add(&new->queue, &new->look_metadata->queue);
        
        printk( "INSERT EMPTY\n");
        look_print_queue(q);
        return;
    }
    
    /* List is singular, add special case */
    if( list_is_singular(&new->look_metadata->queue) ) {
        pos = list_entry( new->look_metadata->queue.next, struct look_queue, queue );

        /* head < new < element or head > element, insert front*/
        if( (new->look_metadata->head_pos < new->beg_pos) && (new->beg_pos < pos->beg_pos ) ||
            (new->look_metadata->head_pos > pos->look_metadata->head_pos) )
        {
            list_add( &new->queue, &new->look_metadata->queue );
        }
        else {
            list_add_tail( &new->queue, &new->look_metadata->queue );
        }
        
        printk( "INSERT SINGULAR\n");
        look_print_queue(q);
        return;

    }
   
    /* List contains at least two elements */ 
    if( new->beg_pos > new->look_metadata->head_pos ) {
        
        /* Search forward */
        printk( "FORWARD\n");
        
        list_for_each_entry(pos, &new->look_metadata->queue, queue)
	    {
            if( pos->queue.next != &new->look_metadata->queue ) {
                
                /* We have not reached the end of the list, set next and continue  */
                next = list_entry( new->queue.next, struct look_queue, queue );
                
                /* Insert at beginning of list */
                if( pos->beg_pos > new->beg_pos ) {
                    printk( "INSERT BEGINNING\n");
                    list_add( &new->queue, &new->look_metadata->queue );
                    break;
                }
                
                if( (pos->beg_pos < new->beg_pos) && (new->beg_pos < next->beg_pos) )
                {
                    /* pos < new < next, insert */
                    printk( "INSERT MIDDLE\n");
                    list_add( &new->queue, &pos->queue );
                    break;
                }


                if( next->beg_pos < pos->look_metadata->head_pos )
                {
                    /* next < pos: end of this side of the queue, insert */
                    printk( "INSERT EDGE\n");
                    list_add( &new->queue, &pos->queue );
                    break;
                }

            } else {
                /* End of the list, insert */
                printk( "INSERT END\n");
                list_add( &new->queue, &pos->queue );
                break;
            }
        }
    } else 
    {
        /* Search reverse */
        printk( "REVERSE\n");
        
        list_for_each_entry_reverse(pos, &new->look_metadata->queue, queue)
	    {
            if( pos->queue.prev != &new->look_metadata->queue ) {
                
                /* We have not reached the end of the list, set prev and continue  */
                prev = list_entry( new->queue.prev, struct look_queue, queue );

                /* Insert at beginning of list */
                if( pos->beg_pos < new->beg_pos ) {
                    printk( "INSERT BEGINNING\n");
                    list_add_tail( &new->queue, &new->look_metadata->queue );
                    break;
                }

                if( (prev->beg_pos < new->beg_pos) && (new->beg_pos < pos->beg_pos) )
                {
                    /* prev < new < pos, insert */
                    printk( "INSERT MIDDLE\n");
                    list_add_tail( &new->queue, &pos->queue );
                    break;
                }


                if( prev->beg_pos > pos->look_metadata->head_pos )
                {
                    /* prev > pos: end of this side of the queue, insert */
                    printk( "INSERT EDGE\n");
                    list_add_tail( &new->queue, &pos->queue );
                    break;
                }

            } else {
                /* End of the list, insert */
                printk( "INSERT END\n");
                list_add_tail( &new->queue, &pos->queue );
                break;
            }
        }
    
    }

    look_print_queue(q);	
}

/**
* look_queue_empty - Is the queue empty
* @q: the request_queue
*
* Documentation in biodoc.txt states that drivers should not use this function 
* but rather check if elv_next_request is NULL.  Returns 1 or 0
*/
static int look_queue_empty(struct request_queue *q)
{
	struct look_data *nd = q->elevator->elevator_data;

	return list_empty(&nd->queue);
}

/**
* look_former_req_fn - get the request before rq
* @rq: a request
* @q: the request queue that rq is in
* 
* Return a pointer to the request struct
*/
static struct request *
look_former_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

/**
* look_latter_req_fn - get the request after rq
* @rq: a request
* @q: the request queue that rq is in
* 
* Return a pointer to the request struct
*/
static struct request *
look_latter_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

/**
* look_init_fn - allocate memory for a request_queue
* @q: empty pointer to q
*/
static void *look_init_queue(struct request_queue *q)
{
	struct look_data *nd;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	INIT_LIST_HEAD(&nd->queue);
	return nd;
}

/**
* look_exit_fn -  deallocates memory allocated in look_init_fn
* @q: the request queue
*
* called when scheduler is relieved of its scheduling duties for a disk
*/
static void look_exit_queue(struct elevator_queue *e)
{
	struct look_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

/**
* get_dir -  get the direction of the disk
* @rq: a pointer to the specific request
*
* Returns a 'r' for read and 'w' for write
*/
static char get_dir(struct request * rq)
{
	int dir = rq_data_dir(rq);
	
	if (dir == 0)
		return 'R';
	else return 'W';
} 

/**
* look_print_queue -  print te specified look queue
* @q: request_queue in question
*
*/
void look_print_queue( struct request_queue *q ) {
    struct look_queue *pos;
    struct look_data *ld = q->elevator->elevator_data;

    printk( "QUEUE: head position: %u\n", ld->head_pos);
    list_for_each_entry(pos, &ld->queue, queue) {

        printk( "    %u\n", pos->beg_pos);

    }

}

/**
* struct look_data - the elevator qu
* @.ops: pointers to the functions of this struct.
* @.elevator_name: the name of this type of elevator
* @.elevator_owner: owner of elevator 
*
*/ 
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

/**Private functions**/
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

MODULE_AUTHOR("Alex Weeks, Kevin McIntosh, Tyler McClung, Josh Jordahl, Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Look Scheduler IO scheduler");
