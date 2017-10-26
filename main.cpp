#include <event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

#include <string.h>

static void echo_read_cb(struct bufferevent *bev, void *ctx){
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    evbuffer_add_buffer(output, input);
}

static void echo_event_cb(struct bufferevent *bev, short events, void *ctx){
    if(events & BEV_EVENT_ERROR){
        perror("Error from bufferevent");
    }
    if(events & (BEV_EVENT_ERROR | BEV_EVENT_EOF)){
        bufferevent_free(bev);
    }
}

static void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, sockaddr *address, int sock_len, void *ctx){
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);

    bufferevent_enable(bev, EV_READ|EV_WRITE);
}

static void accept_error_cb(struct evconnlistener *listener, void *ctx){
    int error = EVUTIL_SOCKET_ERROR();

    fprintf(stderr, "Got an error %d (%s) on the listener\n", error, evutil_socket_error_to_string(error));

    struct event_base *base = evconnlistener_get_base(listener);
    event_base_loopexit(base, NULL);
}

int main(){
    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;


    base = event_base_new();
    if(!base){
        puts("Couldn't open event base");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET; sin.sin_port = htons(8888); sin.sin_addr.s_addr = htonl(INADDR_ANY);


    listener = evconnlistener_new_bind(base, accept_conn_cb, NULL, LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, 1, (sockaddr*)&sin, sizeof(sin));
    if(!listener){
        perror("Couldn't create listener");
        return 1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

    event_base_dispatch(base);
    return 0;
}