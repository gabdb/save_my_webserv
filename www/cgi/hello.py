#!/usr/bin/env python3
import os
import sys

body = "<html><body>"\
       "<h1>Hello from Python CGI</h1>"\
       "<p>Method: %s</p>" % os.environ.get("REQUEST_METHOD", "") + \
       "<p>Path: %s</p>" % os.environ.get("PATH_INFO", "") + \
       "</body></html>"

headers = [
    "Status: 200 OK",
    "Content-Type: text/html",
    "Content-Length: %d" % len(body),
]

for h in headers:
    sys.stdout.write(h + "\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
