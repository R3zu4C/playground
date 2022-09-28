#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>

#include "rlib.h"

typedef struct Buffer
{
    char allocated;         /* Says if the Buffer is currently used. If it's not allocated the Buffer segment can be overwritten */
    char segment[500];      /* Payload which was/will be in the packet. Might be written continously by rel_read over several calls */
    uint16_t len;           /* Gives us the number of written bytes in the segment. Might get changed by rel_read if the Buffer is in the send-buffer */
} Buffer;

struct reliable_state
{
    rel_t *next;			/* Linked list for traversing all connections */
    rel_t **prev;

    conn_t *c;			    /* This is the connection object */

    /* Add your own data fields below this */

    Buffer *recv_buffer;
    Buffer *send_buffer;

    /* Gives us the lower bound of our window. */
    size_t recv_seqno;      /* Up until this seqno everything has been recieved */
    size_t send_seqno;      /* Up until this seqno everything has been sent */

    size_t eof_seqno;

    size_t window_size;

    size_t already_written; /* How many bytes of a Buffer are already written into the console. Buffer-Segments might be written only partly in the function rel_output() by lib-call conn_ouput() */
    
    char eof_recv_flag;
    char eof_read_flag;
    char all_sent_ack_flag;
    char all_written_flag;
    char last_alloc_sent_flag; /* Needed so that we know, when to create a new packet instead of fill up an old packet */
    char small_pkt_onl_flag;   /* Tells us if we have an unacknwolged partly full packet in our send-buffer. A packet with a payload smaller than 500 Bytes is called small */
};

rel_t *rel_list;

void 
send_ack (rel_t *r)
{
    struct ack_packet pkt;

    // Host to Network Byte Order (Endianness)
    pkt.len = htons(8);
    pkt.ackno = htonl(r->recv_seqno);

    pkt.cksum = 0;
    pkt.cksum = cksum(&pkt, 8);
    
    print_pkt((packet_t *)&pkt, "send ack", 8);

    conn_sendpkt(r->c, (packet_t *)&pkt, 8);
}

void 
send_packet (rel_t *r, uint32_t seq_no)
{
    packet_t pkt;
    Buffer *s = &(r->send_buffer[seq_no % r->window_size]);

    // Host to Network Byte Order (Endianness)
    pkt.len = htons(s->len + 12);
    pkt.seqno = htonl(seq_no);
    pkt.ackno = htonl(r->recv_seqno);

    pkt.cksum = 0;
    memcpy(pkt.data, s->segment, s->len);
    pkt.cksum = cksum(&pkt, s->len + 12);

    if (s->len) print_pkt(&pkt, "send data", s->len + 12);
    else print_pkt(&pkt, "send EOF", 12);

    conn_sendpkt(r->c, &pkt, s->len + 12);
}

/* Creates a new reliable protocol session, returns NULL on failure.
 * Exactly one of c and ss should be NULL.  (ss is NULL when called
 * from rlib.c, while c is NULL when this function is called from
 * rel_demux.) */
rel_t *
rel_create (conn_t *c, const struct sockaddr_storage *ss,
	    const struct config_common *cc)
{
    fprintf(stderr, "rel_create is called\n");

    rel_t *r;
    r = xmalloc(sizeof(*r));
    memset(r, 0, sizeof(*r));

    if (!c)
    {
        c = conn_create(r, ss);
        if (!c)
        {
            free(r);
            return NULL;
        }
    }

    r->c = c;
    r->next = rel_list;
    r->prev = &rel_list;
    if (rel_list) rel_list->prev = &r->next;
    rel_list = r;

    r->window_size = cc->window;

    r->recv_buffer = calloc(r->window_size, sizeof(Buffer));
    r->send_buffer = calloc(r->window_size, sizeof(Buffer));

    r->recv_seqno = 1;
    r->send_seqno = 1;

    r->already_written = 0;
    r->eof_seqno = 0;

    r->all_sent_ack_flag = 0;
    r->all_written_flag = 0;
    r->eof_read_flag = 0;
    r->eof_recv_flag = 0;
    r->small_pkt_onl_flag = 0;
    r->last_alloc_sent_flag = 1;

    return r;
}

void 
rel_destroy (rel_t *r)
{
    fprintf(stderr, "rel_destroy is called\n");
    if (r->next)
        r->next->prev = r->prev;
    *r->prev = r->next;
    conn_destroy(r->c);

    /* Free any other allocated memory here */
    free(r->recv_buffer);
    free(r->send_buffer);
    free(r);
}

/* This function only gets called when the process is running as a
 * server and must handle connections from multiple clients.  You have
 * to look up the rel_t structure based on the address in the
 * sockaddr_storage passed in.  If this is a new connection (sequence
 * number 1), you will need to allocate a new conn_t using rel_create
 * ().  (Pass rel_create NULL for the conn_t, so it will know to
 * allocate a new connection.)
 */
void 
rel_demux (const struct config_common *cc,
               const struct sockaddr_storage *ss,
               packet_t *pkt, size_t len)
{
}

void 
rel_recvpkt (rel_t *r, packet_t *pkt, size_t n)
{
    // Network Byte Order to Host (Endianness)
    uint16_t pkt_len = ntohs(pkt->len);
    uint32_t pkt_ackno = ntohl(pkt->ackno);
    uint16_t pkt_cksum = pkt->cksum;
    uint32_t pkt_seqno = ntohl(pkt->seqno);

    if (n < 8 || n != pkt_len) return; // If invalid packet size

    pkt->cksum = 0;
    // If packet corrupted
    if (cksum(pkt, n) != pkt_cksum)
    {
        send_ack(r);
        return;
    }
    pkt->cksum = pkt_cksum;

    if (n == 8) print_pkt(pkt, "recieve ack", n);
    if (n == 12) print_pkt(pkt, "recieve EOF", n);
    if (n > 12) print_pkt(pkt, "recieve data", n);

    // Cummulative acknowledgment
    if (r->send_seqno < pkt_ackno)
    {
        for (uint16_t i = r->send_seqno; i < pkt_ackno; i++)
        {
            Buffer *s = &(r->send_buffer[i % r->window_size]);
            if (s->len < 500)
            {
                r->small_pkt_onl_flag = 0;
            }
            s->allocated = 0;
            s->len = 0;
        }
        r->send_seqno = pkt_ackno;
    }

    if (r->eof_recv_flag && pkt_seqno >= r->eof_seqno) return; // Data packets after EOF not allowed

    // if (pkt_seqno < r->recv_seqno || pkt_seqno >= r->recv_seqno + r->window_size) return; // If sequence number outside window
    if(n >= 12)
    {
        size_t idx = pkt_seqno % r->window_size; // Index inside window

        // If duplicate packet recieved
        if (r->recv_buffer[idx].allocated || r->recv_seqno > pkt_seqno)
        {
            send_ack(r);
            return;
        } 
        // If EOF recieved
        if (pkt_len == 12)
        {
            r->eof_recv_flag = 1;
            r->eof_seqno = pkt_seqno;
        }

        memcpy(r->recv_buffer[idx].segment, pkt->data, pkt_len - 12);

        r->recv_buffer[idx].len = pkt_len - 12;
        r->recv_buffer[idx].allocated = 1;

        if (pkt_seqno == r->recv_seqno) rel_output(r);
    }

}

void 
rel_read (rel_t *r)
{
    Buffer *temp_buf;

    size_t new_seqno;
    size_t start_seq = r->send_seqno; // First free index inside window

    // Calculate start_seq
    while (start_seq < (size_t)(r->send_seqno + r->window_size))
    {
        if (r->send_buffer[start_seq % r->window_size].allocated) start_seq++;
        else break;
    }

    if (r->send_buffer[start_seq % r->window_size].allocated) return; // If buffer is full

    if (r->last_alloc_sent_flag) new_seqno = start_seq; // Create a new packet
    else new_seqno = start_seq - 1; // Fill up previous packet

    temp_buf = &(r->send_buffer[new_seqno % r->window_size]);

    char *write_start = (char *)&(temp_buf->segment) + r->already_written; // Index from where to start writing in buffer
    int16_t recieved = conn_input(r->c, (void *)write_start, (uint16_t)(500 - temp_buf->len)); // Read from STDIN

    if (recieved == 0) return; // No input

    if (recieved == -1) // EOF recieved
    {
        r->eof_read_flag = 1;
        r->send_buffer[start_seq % r->window_size].allocated = 1;
        return;
    }

    fprintf(stderr, "%5d, %s = %d\n", getpid(), "READ Len", recieved);

    temp_buf->allocated = 1;
    temp_buf->len += recieved;

    if (temp_buf->len == 500 || (!r->small_pkt_onl_flag && temp_buf->len != 0))
    {
        r->last_alloc_sent_flag = 1;
        send_packet(r, new_seqno);
    }
    else r->last_alloc_sent_flag = 0; // Store packet to fill later
}

void 
rel_output (rel_t *r)
{
    char ack_flag = 0; // Flag to check if an ACK has to be sent

    // Traverse recv-buffer to send allocated data
    while (r->recv_buffer[r->recv_seqno % r->window_size].allocated)
    {
        Buffer *s = &(r->recv_buffer[r->recv_seqno % r->window_size]);

        size_t written = conn_output(r->c, &(s->segment) + r->already_written, s->len - r->already_written); // Write to STDOUT

        if (written == s->len - r->already_written) // Packet fully filled
        {
            r->already_written = 0;
            s->allocated = 0;
            r->recv_seqno++;
            ack_flag = 1; // Send ACK
        }
        else // Packet partially filled
        {
            r->already_written += written; 
            break;
        }
    }

    if (ack_flag) send_ack(r);

    // If EOF and recv-buffer is empty, all data has been outputted
    if (r->eof_recv_flag)
    {
        char recv_buf_empty = 1;
        for (size_t i = 0; i < r->window_size; i++)
        {
            if (r->recv_buffer[i].allocated)
            {
                recv_buf_empty = 0;
                break;
            }
        }
        if (recv_buf_empty) r->all_written_flag = 1;
    }
}

void
rel_timer ()
{
    if (!rel_list->eof_read_flag) rel_read(rel_list);

    /* Retransmit any packets that need to be retransmitted */
    Buffer *temp_buf;

    char all_acked = 1; // Flag to check if all packets have been ACKed

    size_t ub = rel_list->send_seqno + rel_list->window_size; // Upper bound of window

    // Traverse the window
    for (size_t i = rel_list->send_seqno; i < ub; i++)
    {
        temp_buf = &rel_list->send_buffer[i % rel_list->window_size];

        // If packet is unacknowledged
        if (temp_buf->allocated)
        {
            send_packet(rel_list, i);
            all_acked = 0;
        }
    }

    // If all packets correctly recieved
    if (rel_list->eof_read_flag && all_acked) rel_list->all_sent_ack_flag = 1;

    // If session end
    if (rel_list->eof_recv_flag && rel_list->eof_read_flag && rel_list->all_sent_ack_flag && rel_list->all_written_flag) rel_destroy(rel_list);
}
