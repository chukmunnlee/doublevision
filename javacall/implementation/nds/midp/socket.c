/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */


#ifdef __cplusplus
extern "C" {
#endif

#include <nds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <dswifi9.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "javacall_socket.h" 
#include "javacall_time.h" 
#include "defs.h"

extern int errno;

#define MAX_SOCK_Q_LEN (128)

typedef struct blockedSocksQ {
    int buf[MAX_SOCK_Q_LEN]; 
    int num;
} blockedSocksQ;

static blockedSocksQ readSocks;
static blockedSocksQ writeSocks;
static int maxsockfd;
static javacall_handle timer;

static javacall_handle open_timer = NULL;

void socks_Q_init() {
    readSocks.num = 0;
    writeSocks.num = 0;
    maxsockfd = 0;
}

inline int addBlockedSock(int sockfd, blockedSocksQ* sbuf) {

    if (sbuf->num == MAX_SOCK_Q_LEN) {
        return -1;
    } else {
        sbuf->buf[sbuf->num] = sockfd;
        sbuf->num++;
        maxsockfd = (sockfd < maxsockfd) ? maxsockfd : sockfd;
        return 1;
    }
}

inline int delBlockedSock(int sockfd, blockedSocksQ* sbuf) {

    int i = 0;
    while(sbuf->buf[i] != sockfd) {
        if (i == sbuf->num) {
            return -1;
        }
        i++;
    }
    memmove(&sbuf->buf[i], &sbuf->buf[i+1], (sbuf->num-i)*sizeof(int));
    sbuf->num--;
    return 1;
}

inline void makeFDSet(fd_set* set, blockedSocksQ* sbuf) {
    FD_ZERO(set);
    int i;
    for (i = 0; i < sbuf->num; i++){
        FD_SET(sbuf->buf[i], set);
    }
}

void timer_socket_callback() {

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0; 

    fd_set readfds;
    fd_set writefds;
    makeFDSet(&readfds,  &readSocks);
    makeFDSet(&writefds, &writeSocks);

    int status = select(maxsockfd+1, &readfds, &writefds, NULL, &tv);
    if (status == 0) {
        return;
    }
    int i;    
    for (i = 0; i < readSocks.num; i++) {
        if (FD_ISSET(readSocks.buf[i], &readfds)) {
            javanotify_socket_event(JAVACALL_EVENT_SOCKET_RECEIVE, 
                            (javacall_handle)readSocks.buf[i], JAVACALL_OK);
        }
    }
    for (i = 0; i < writeSocks.num; i++) {
        if (FD_ISSET(writeSocks.buf[i], &writefds)) {
            javanotify_socket_event(JAVACALL_EVENT_SOCKET_SEND, 
                            (javacall_handle)writeSocks.buf[i], JAVACALL_OK);
        }
    }
 
}

void startTimerIfNeeded() {
    if (timer == 0) {
        javacall_time_initialize_timer(100 /*msec*/, JAVACALL_TRUE /*cyclic*/,
                                       timer_socket_callback, &timer);
    }
} 

void stopTimerIfNeeded() {
    if (readSocks.num == 0 &&  writeSocks.num == 0) {
        javacall_time_finalize_timer(timer);
        timer = 0;
    }
}

#if 0
//Copied from PSPKVM
typedef struct op {
	struct sockaddr_in *addr;
	int fd;
	int retry;
	int status;
	struct op *next;
} open_param;

typedef struct rp {
	int fd;
	unsigned char *buff;
	int len;
	int bytes_read;
	int status;
	struct rp *next;
} read_param;

static open_param *open_chain_root = NULL;
static javacall_handle open_timer = NULL;

void add_to_open_chain(open_param *node) {

	open_param *curr;

	node->next = NULL;

	if (!open_chain_root) {
		open_chain_root = node;
		return;
	}

	curr = node;
	while (!curr) {
		if (!curr->next) {
			curr->next = node;
			return;
		}
		curr = curr->next;
	}
}
//Does not free the node, caller have to do it
void remove_from_open_chain(open_param *node) {

	open_param *prev;
	open_param *curr;

	if (open_chain_root == node) {
		open_chain_root = node->next;
		return;
	}

	prev = open_chain_root;
	curr = open_chain_root->next;
	while (!curr) {
		if (curr == node) {
			prev->next = curr->next;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

void open_connection(javacall_handle handle) {

	int status;
	int sockfd;
	int optval = 1;

       /*
	open_param *curr = open_chain_root;
	if (!curr)
		return;

	sockfd = curr->fd;

	curr->retry++;
       */
	status = connect(sockfd, (struct sockaddr *)curr->addr, sizeof(struct sockaddr_in));

	switch (status) {
		case 0:
			ioctl(sockfd, FIONBIO, &optval);
			status = JAVACALL_OK;
			break;

		case -1:
			//Flag an error for everything that is not a timeout
			if (errno != ETIMEDOUT) {
				status = JAVACALL_FAIL;
				break;
			}

		default:
			//Its a time out
			if (curr->retry < RETRY) {
				remove_from_open_chain(curr);
				add_to_open_chain(curr);
				return;
			}
			status = JAVACALL_FAIL;
	}

	remove_from_open_chain(curr);
	if (!open_chain_root) {
		javacall_time_finalize_timer(open_timer);
		open_timer = NULL;
	}

	javanotify_socket_event(JAVACALL_EVENT_SOCKET_OPEN_COMPLETED
			, (javacall_handle)curr->fd, status);
}

static read_param *read_chain_root = NULL;
static javacall_handle read_timer = NULL;

void add_to_read_chain(read_param *node) {

	read_param *curr;

	//Since we always add to the tail of the chain, next is NULL
	node->next = NULL;

	if (!read_chain_root) {
		read_chain_root = node;
		return;
	}
	curr = read_chain_root;
	while (!curr) {
		//Add it at the end
		if (!curr->next) {
			curr->next = node;
			return;
		}
		curr = curr->next;
	}
}
//Does not free the node, the caller have to do it
void remove_from_read_chain(read_param *node) {

	read_param *prev;
	read_param *curr;

	if (read_chain_root == node) {
		read_chain_root = node->next;
		return;
	}
	prev = read_chain_root;
	curr = read_chain_root->next;
	while (!curr) {
		if (curr == node) {
			prev->next = curr->next;
			return;
		}
		prev = curr;
		curr = curr->next;
	}
}

void read_bytes(javacall_handle handle) {
	int status;
	int count = 0;
	int sockfd;

	//Read the head
	read_param *curr = read_chain_root;

	//Should not need to check if we are absolutely correct
	if (!curr)
		return;

	/*
		Keep reading until
		1. we have read exactly the amount (len)
		2. exceeded the amount (len)
		3. got an error, status == -1, includes EAGAIN/EWOULDBLOCK
	*/
	sockfd = curr->fd;
	while (1) {
		status = recv(sockfd, (curr->buff + curr->bytes_read), curr->len, 0);
		switch (status) {
			case -1:
				if (errno != EWOULDBLOCK) {
					curr->status = JAVACALL_FAIL;
                                   javacall_print("is not block.\n");
					javanotify_socket_event(JAVACALL_EVENT_SOCKET_RECEIVE
							, (javacall_handle)curr->fd, JAVACALL_FAIL);
				} else {
					//Move curr to the end of the queue
                                   javacall_print("move to end!\n");
					remove_from_read_chain(curr);
					add_to_read_chain(curr);
				}
				return;

			case 0: 
				//Treat peer shutdown as OK
				goto out;

			default:
				curr->bytes_read += status;
				curr->len -= status;
				if (curr->len <= 0)
					goto out;
		}
	}

out:
	curr->status = JAVACALL_OK;
	remove_from_read_chain(curr);
	if (!read_chain_root) {
		javacall_time_finalize_timer(read_timer);
		read_timer = NULL;
	}

	javanotify_socket_event(JAVACALL_EVENT_SOCKET_RECEIVE
			, (javacall_handle)curr->fd, JAVACALL_OK);
}
#endif

/**
 * Initiates the connection of a client socket.
 *
 * @param ipBytes IP address of the local device in the form of byte array
 * @param port number of the port to open
 * @param pHandle address of variable to receive the handle; this is set
 *        only when this function returns JAVACALL_WOULD_BLOCK or JAVACALL_OK.
 * @param pContext address of a pointer variable to receive the context;
 *        this is set only when the function returns JAVACALL_WOULDBLOCK.
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an IO error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function to complete the operation
 * @retval JAVACALL_CONNECTION_NOT_FOUND when there was some other error (Connection not found exception case)
 */

int create_socket(int *sockfd) {

    int optval = 1;

    if ((*sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        switch (errno) {
            case EADDRINUSE:
            case ECONNREFUSED:
                return (JAVACALL_CONNECTION_NOT_FOUND);

            case ENOBUFS:
            case ENOMEM:
                return (JAVACALL_OUT_OF_MEMORY);

            default:
                break;
        }
        return (JAVACALL_FAIL);
    }

    //Set to non blocking
    ioctl(*sockfd, FIONBIO, &optval);

    return (JAVACALL_OK);
}

inline void close_socket(int sockfd, struct sockaddr_in *addr) {

    if (addr)
        free(addr);
    shutdown(sockfd, 0);
    closesocket(sockfd);
}

javacall_result javacall_socket_open_start(unsigned char *ipBytes,int port,
                                           void **pHandle, void **pContext) {
    //open_param *param;
    struct sockaddr_in *addr;
    int optval = 1;
    int status;
    int sockfd;
    //int retry = 0;

    if ((status = create_socket(&sockfd)) != JAVACALL_OK)
        return (status);

       javacall_printf("create socket, fd is %d.\n", sockfd);
    //setsockopt does not do anything 
    //int status = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    if (!addr) {
        close_socket(sockfd, NULL);
        return (JAVACALL_OUT_OF_MEMORY);
    }

    bzero(addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_port = htons((unsigned short)port);
    memcpy(&(addr->sin_addr.s_addr), ipBytes, sizeof(addr->sin_addr.s_addr));

    status = connect(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
    switch (status) {
        case 0:
            *pHandle = (void *)sockfd;
            *pContext = NULL;
            free(addr);
                     javacall_printf("connected, return ok.\n");
            return (JAVACALL_OK);

        case -1:
            //Flag an error for everything that is not a timeout
            if (errno != ETIMEDOUT) {
                close_socket(sockfd, addr);
                return (JAVACALL_FAIL);
            }

        default:
            break;
            //Its a time out
    }

    //param = (open_param *)malloc(sizeof(open_param));
    //if (!param) {
    //  close_socket(sockfd, addr);
    //  return (JAVACALL_OUT_OF_MEMORY);
    //}

    //param->fd = sockfd;
    //param->retry = 0;
    //param->addr = addr;

    *pHandle = (void *)sockfd;
    //*pContext = param;
    *pContext = NULL;

    //add_to_open_chain(param);
    //if (!open_timer)
    //  javacall_time_initialize_timer(3000, JAVACALL_TRUE, open_connection, open_timer);
    addBlockedSock(sockfd, &readSocks);
    startTimerIfNeeded();

    return (JAVACALL_WOULD_BLOCK);
}
    
/**
 * Finishes a pending open operation.
 *
 * @param handle the handle returned by the open_start function
 * @param context the context returned by the open_start function
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if an error occurred  
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result javacall_socket_open_finish(void *handle, void *context) {
    /*
    int sockfd = (int)handle;
    
    int err = 0;
    socklen_t err_size = sizeof(err);

    int status = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &err, &err_size);

    if (err == 0 && status == 0) {
        return JAVACALL_OK;
    }
    
    close_socket(sockfd);
    return JAVACALL_FAIL;
    */
    return JAVACALL_OK;
}

//originally from pcsl_network_bsd (pcsl_socket.c)
javacall_result socket_read_common(void *handle, unsigned char *pData,
                                   int len, int *pBytesRead) {
    int sockfd = (int) handle;
    int status = recv(sockfd, pData, len, 0);

    if (SOCKET_ERROR == status) {
        if (EWOULDBLOCK == errno || EINPROGRESS == errno) {
            return JAVACALL_WOULD_BLOCK;
        } else if (EINTR == errno) {
            return JAVACALL_INTERRUPTED;
         } else {
            return JAVACALL_FAIL;
        }
    }

    *pBytesRead = status;
    return JAVACALL_OK;
}

/**
 * Initiates a read from a platform-specific TCP socket.
 *  
 * @param handle handle of an open connection
 * @param pData base of buffer to receive read data
 * @param len number of bytes to attempt to read
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns JAVACALL_OK
 * @param pContext address of pointer variable to receive the context;
 *        it is set only when this function returns JAVACALL_WOULDBLOCK
 * 
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the operation would block
 * @retval JAVACALL_INTERRUPTED for an Interrupted IO Exception
 */
javacall_result javacall_socket_read_start(void *handle,unsigned char *pData,int len, 
                                           int *pBytesRead, void **pContext) {
    javacall_result status;
    status = socket_read_common(handle, pData, len, pBytesRead);

    if (status == JAVACALL_WOULD_BLOCK) {
        addBlockedSock((int)handle, &readSocks);
        startTimerIfNeeded();
        *pContext = NULL;
    }

    return status;

    /*
    read_param *param;
    param = (read_param *)malloc(sizeof(read_param));
    if (!param) {
        return (JAVACALL_OUT_OF_MEMORY);
    }

    param->fd = sockfd;
    param->len = len;
    param->buff = malloc(len);
    if (!param->buff) {
        free(param);
        return (JAVACALL_OUT_OF_MEMORY);
    }
    param->bytes_read = 0;
    *pContext = param;

    //Must add to the read chain first before we check if we need to start the timer!!!
    add_to_read_chain(param);

    if (!read_timer)
        javacall_time_initialize_timer(250, JAVACALL_TRUE, read_bytes, &read_timer);
    
    return (JAVACALL_WOULD_BLOCK);
    */
}
    
/**
 * Finishes a pending read operation.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer to receive read data
 * @param len number of bytes to attempt to read
 * @param pBytesRead returns the number of bytes actually read; it is
 *        set only when this function returns JAVACALL_OK
 * @param context the context returned by read_start
 * 
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 * @retval JAVACALL_INTERRUPTED for an Interrupted IO Exception
 */
javacall_result javacall_socket_read_finish(void *handle,unsigned char *pData,int len,int *pBytesRead,void *context) {
    /*
    read_param *param = (read_param *)context;
    int status = param->status;

    if (status == JAVACALL_OK) {
        memcpy(pData, param->buff, param->bytes_read);
        *pBytesRead = param->bytes_read;
    }

    free(param->buff);
    free(param);

    return (status);                                        
    */
    javacall_result status = socket_read_common(handle, pData, len, pBytesRead);
    
    if (status != JAVACALL_WOULD_BLOCK) {
        delBlockedSock((int)handle, &readSocks);
        stopTimerIfNeeded();
    }

    return status;
}
    
/**
 * Initiates a write to a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer containing data to be written
 * @param len number of bytes to attempt to write
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        JAVACALL_OK
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns JAVACALL_WOULDBLOCK
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the operation would block
 * @retval JAVACALL_INTERRUPTED for an Interrupted IO Exception
 */
javacall_result javacall_socket_write_start(void *handle,char *pData,int len,int *pBytesWritten,void **pContext) {
    int sockfd = (int) handle;
    int status = send(sockfd, pData, len, 0);

    if (SOCKET_ERROR == status) {
        if (EWOULDBLOCK == errno || EINPROGRESS == errno) {
            //return JAVACALL_WOULD_BLOCK;
            return JAVACALL_FAIL; // assume send will never block
        } else if (EINTR == errno) {
            return JAVACALL_INTERRUPTED;
        } else {
            return JAVACALL_FAIL;
        }
    }

    *pBytesWritten = status;
    return JAVACALL_OK;
}
    
/**
 * Finishes a pending write operation.
 *
 * @param handle handle of an open connection
 * @param pData base of buffer containing data to be written
 * @param len number of bytes to attempt to write
 * @param pBytesWritten returns the number of bytes written after
 *        successful write operation; only set if this function returns
 *        JAVACALL_OK
 * @param context the context returned by write_start
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 * @retval JAVACALL_INTERRUPTED for an Interrupted IO Exception
 */
javacall_result javacall_socket_write_finish(void *handle,char *pData,int len,int *pBytesWritten,void *context) {
    return JAVACALL_FAIL; //Since write_start never blocking, write_finish should never be invoked
}
    
/**
 * Initiates the closing of a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param pContext address of a pointer variable to receive the context;
 *	  it is set only when this function returns JAVACALL_WOULDBLOCK
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error
 * @retval JAVACALL_WOULD_BLOCK  if the operation would block 
 */
javacall_result javacall_socket_close_start(void *handle,void **pContext) {
    int sockfd = (int)handle;
    shutdown(sockfd, 0);
    closesocket(sockfd);
    return (JAVACALL_OK);
}
    
/**
 * Initiates the closing of a platform-specific TCP socket.
 *
 * @param handle handle of an open connection
 * @param context the context returned by close_start
 *
 * @retval JAVACALL_OK          success
 * @retval JAVACALL_FAIL        if there was an error   
 * @retval JAVACALL_WOULD_BLOCK  if the caller must call the finish function again to complete the operation
 */
javacall_result javacall_socket_close_finish(void *handle,void *context) {
    return (JAVACALL_OK);
}

/******************************************************************************
 ******************************************************************************
 ******************************************************************************
    OPTIONAL FUNCTIONS
 ******************************************************************************
 ******************************************************************************
 ******************************************************************************/
    
/**
 * @defgroup OptionalTcpSocket Optional client socket API
 * @ingroup Network
 * @{
 */

/**
 * Gets the number of bytes available to be read from the platform-specific
 * socket without causing the system to block.
 *
 * @param handle handle of an open connection
 * @param pBytesAvailable returns the number of available bytes
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error 
 */
//Copied wholesale from PSPKVM
javacall_result /* OPTIONAL*/ javacall_socket_available(javacall_handle handle,int *pBytesAvailable) {
    int sockfd = (int)handle;
    fd_set fdR;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fdR);
    FD_SET(sockfd, &fdR);

    switch (select(sockfd + 1, &fdR, NULL, NULL, &tv)) {
        case -1:
            return (JAVACALL_FAIL);

        case 0:
            *pBytesAvailable = 0;
            break;

        default:
            if (FD_ISSET(sockfd, &fdR))
                *pBytesAvailable = 1;
            break;
    }

    return (JAVACALL_OK);
}
    
/**
 * Shuts down the output side of a platform-specific TCP socket.
 * Further writes to this socket are disallowed.
 *
 * Note: a function to shut down the input side of a socket is
 * explicitly not provided.
 *
 * @param handle handle of an open connection
 *
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    if there was an error
 */
javacall_result /*OPTIONAL*/ javacall_socket_shutdown_output(javacall_handle handle) {
    return JAVACALL_FAIL;
}

javacall_result /*OPTIONAL*/ javacall_server_socket_open_start(
    int port, void **pHandle, void **pContext) {

    int status;
    int sockfd;
    struct sockaddr_in *addr;
    //open_param *param;

    if ((status = create_socket(&sockfd)) != JAVACALL_OK)
        return (status);

    addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    if (!addr) {
        close_socket(sockfd, NULL);
        return (JAVACALL_OUT_OF_MEMORY);
    }

    bzero(addr, sizeof(struct sockaddr_in));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) == INVALID_SOCKET) {
        switch (errno) {
            case EADDRNOTAVAIL:
            case EADDRINUSE:
            case EINVAL:
                status = JAVACALL_CONNECTION_NOT_FOUND;
                break;

            case ENOMEM:
                status = JAVACALL_OUT_OF_MEMORY;
                break;

            default:
                status = JAVACALL_FAIL;
                break;
        }

        close_socket(sockfd, addr);

        return (status);
    }

    if (listen(sockfd, 3) == INVALID_SOCKET) {
        close_socket(sockfd, addr);
        return (JAVACALL_FAIL);
    }

    /*
    param = (open_param *)malloc(sizeof(open_param));
    if (param == NULL) {
        close_socket(sockfd, addr);
        return (JAVACALL_OUT_OF_MEMORY);
    }

    param->fd = sockfd;
    param->addr = addr;

    *pHandle = (void *)sockfd;
    *pContext = param;
    */
    return (JAVACALL_OK);
}

/**
 * See pcsl_network.h for definition.
 */
javacall_result javacall_server_socket_open_finish(void *handle,void *context){

    //Never called
    return JAVACALL_FAIL;
}


/**
 * See javacall_socket.h for definition.
 */
javacall_result /*OPTIONAL*/ javacall_server_socket_accept_start(
    javacall_handle handle, 
    javacall_handle *pNewhandle) {

    //accept_param *serv_param = (accept_param *)handle;
   
    return JAVACALL_FAIL;
}

/**
 * See javacall_socket.h for definition.
 */
javacall_result /*OPTIONAL*/ javacall_server_socket_accept_finish(
    javacall_handle handle, 
    javacall_handle *pNewhandle) {

    return JAVACALL_FAIL;
}

#ifdef __cplusplus
}
#endif


