#!/usr/bin/env python

import sys
import time
import _pub_sub_api

_pub_sub_api.init_publish_api("br0")
count = 0

for i in range(1000, 1024):
    time.sleep(0.001)
    #_pub_sub_api.register_lid(str(i))
    _pub_sub_api.register_lid_tag_context(str(i), 'Sahil_'+str(i), 'Sector33_'+str(i))


_pub_sub_api.register_lid_tag("1011", "Sahil")
_pub_sub_api.register_lid_tag("1021", "Rohit")
_pub_sub_api.register_lid_tag("1001", "ABC")
_pub_sub_api.register_lid_tag("1015", "DEF")
_pub_sub_api.register_lid_tag("1013", "Rohit")
_pub_sub_api.register_lid_tag("1031", "Sahil")
_pub_sub_api.register_lid_tag("1071", "HSC")

_pub_sub_api.register_lid_tag_context("102344", "Sahil", "Sec 33")
_pub_sub_api.register_lid_tag_context("10234", "Sahil", "Sec 34")
_pub_sub_api.register_lid_tag_context("1023", "EEE", "Sec 33")
_pub_sub_api.register_lid_tag_context("1024", "FFF", "Sec 35")
_pub_sub_api.register_lid_tag_context("1014", "DEF", "Sec 38")
_pub_sub_api.register_lid_tag_context("10", "Sahil", "HSC")

time.sleep(1)
l1 = _pub_sub_api.get_all_logical_ids()
l2 = _pub_sub_api.get_all_tags()
l3 = _pub_sub_api.get_all_contexts()
print 'Printing from Python'
print 'l1 is: ', l1
print 'l2 is: ', l2
