#!/usr/bin/env python

import sys
import time
import threading
import _pub_sub_api

count = 0

def func(lid, tag, value):
    global count
    if lid != 'thread 1':
        count = count + 1
    if count > 0:
        print count
        print 'call back invoked with ', lid, ' : ', tag, ' : ', value

def func1( stri):
    while True:
        time.sleep(10)
        func('thread 1', 'testing', '09890')
for i in range(0,1):
    #_pub_sub_api.py_subscribe_lid_tag(func, str(i).encode('utf-8'), "Sahil")
    _pub_sub_api.py_subscribe_lid_tag(func, "LID1", "Sahil");
    #_pub_sub_api.py_subscribe_tag(func, "Sahil")
    #_pub_sub_api.py_subscribe_context(func, "Sector33")

t1 = threading.Thread(target=func1, args=("Thread1",))
t1.start()

t1.join
while True:
    a = 10
