#!/usr/bin/env python

import sys
import time
import _pub_sub_api

_pub_sub_api.init_publish_api("br0")
count = 0


while True:
    for i in range(0,10):
        for j in range(0,10):
            time.sleep(0.000001)
            _pub_sub_api.publish_info(str(i), "Sahil", "1111")
            count = count + 1
            if count == 200000:
                print 'Sent ', count, ' Messages'
                sys.exit()

    """_pub_sub_api.publish_info("LID1", "Sahil", "456")
    _pub_sub_api.publish_cinfo("asa", "Sahil", "context1", "889")
    _pub_sub_api.publish_cinfo("EQWE", "WQSQW", "Sector33", "767")"""
