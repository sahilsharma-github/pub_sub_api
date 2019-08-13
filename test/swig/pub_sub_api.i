%module pub_sub_api

%apply char* {char*};
%newobject get_all_logical_ids;
char *get_all_logical_ids();
%newobject get_all_tags;
char *get_all_tags();
%newobject get_all_contexts;
char *get_all_contexts();

%{
    
    #include "../../h/pub_sub_api.h"
    #include "../../h/pub_sub_types.h"
    #include "../../h/pub_sub_msg_defines.h"
    #include<net/if.h>
#include<netinet/in.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/ioctl.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<string.h>
#include<unistd.h>

    pub_sub_cb_map lidtag_map;
    pub_sub_cb_map tag_map;
    pub_sub_cb_map context_map;
    pthread_t conn_manager;
    pthread_mutex_t pub_sub_mutex;
    bool thread_spawned;
    pub_sub_queue msg_jobs;
    static int sockfd;
    static int client_fd;
    static struct sockaddr_in sendaddr;
    static struct sockaddr_in sub_sendaddr;
    static char interface_name[PUB_SUB_MAX_IF_LEN];
    static char interface_bcast_ip[PUB_SUB_MAX_IP_LEN];
    char* extract_ip(char * if_name);
    void convert_to_bcast_ip(char * ip);
    void send_broadcast_message(char * msg, int size, const char * bcast_ip);
    Result init_publish_api(const char * if_name);
    Result init_client();
    void process_notification_msg(generic_msg msg);
    void process_msg_from_filter_app(generic_msg msg);
    void update_map( pub_sub_cb_map * map, generic_msg msg, pub_sub_callback cb);
    void * tcp_conn_handler(void * data);
    static PyObject *py_callback = NULL;


    static void python_callback(char * lid, char * tag, char * val)
    {
        PyObject *func, *arglist;
        PyObject *result;
        func = py_callback;
        arglist = Py_BuildValue("sss", lid, tag, val);
        //--------------------------------------------
        PyGILState_STATE gstate;
        gstate = PyGILState_Ensure();
        result = PyEval_CallObject(func,arglist);
        PyGILState_Release(gstate);
        //-------------------------------------------------
        Py_DECREF(arglist);
        Py_XDECREF(result);
    }
    static void * py_subscribe_lid_tag(PyObject * pyfunc, char * lid, char * tag)
    {
        Py_XDECREF(py_callback); /* Dispose of previous callback */
        py_callback = pyfunc;
        Py_XINCREF(py_callback); /* Remember new callback */
        Py_INCREF(Py_None); 
        subscribe_lid_tag(python_callback, lid, tag);
        Py_INCREF(Py_None);
        return Py_None;
    }
    static void * py_subscribe_tag(PyObject * pyfunc, char * tag)
    {
        Py_XDECREF(py_callback); /* Dispose of previous callback */
        py_callback = pyfunc;
        Py_XINCREF(py_callback); /* Remember new callback */
        Py_INCREF(Py_None);
        subscribe_tag(python_callback, tag);
        Py_INCREF(Py_None);
        return Py_None;
    }
    static void * py_subscribe_context(PyObject * pyfunc, char * ctxt)
    {
        Py_XDECREF(py_callback); /* Dispose of previous callback */
        py_callback = pyfunc;
        Py_XINCREF(py_callback); /* Remember new callback */
        Py_INCREF(Py_None);
        subscribe_context(python_callback, ctxt);
        Py_INCREF(Py_None);
        return Py_None;

    }
%}
    %include "../../h/pub_sub_api.h"
    %include "../../h/pub_sub_types.h"
    %include "../../h/pub_sub_msg_defines.h"

    static PyObject * py_subscribe_lid_tag(PyObject * pyfunc, char * lid, char * tag);
    static void * py_subscribe_tag(PyObject * pyfunc, char * tag);
    static void * py_subscribe_context(PyObject * pyfunc, char * ctxt);
    pub_sub_cb_map lidtag_map;
    pub_sub_cb_map tag_map;
    pub_sub_cb_map context_map;
    pthread_t conn_manager;
    pthread_mutex_t pub_sub_mutex;
    bool thread_spawned;
    pub_sub_queue msg_jobs;
    static int sockfd;
    static int client_fd;
    static struct sockaddr_in sendaddr;
    static struct sockaddr_in sub_sendaddr;
    static char interface_name[PUB_SUB_MAX_IF_LEN];
    static char interface_bcast_ip[PUB_SUB_MAX_IP_LEN];
    char* extract_ip(char * if_name);
    void convert_to_bcast_ip(char * ip);
    void send_broadcast_message(char * msg, int size, const char * bcast_ip);
    Result init_publish_api(const char * if_name);
    Result init_client();
    void process_notification_msg(generic_msg msg);
    void process_msg_from_filter_app(generic_msg msg);
    void update_map( pub_sub_cb_map * map, generic_msg msg, pub_sub_callback cb);
    void get_all_logical_ids();
    void get_all_tags();
    void get_all_contexts();

