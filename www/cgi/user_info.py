#!/usr/bin/env python3
import os
import sys
from urllib.parse import parse_qs


def send_response(status: str, body: str, content_type: str = "text/html; charset=utf-8") -> None:
    body_bytes = body.encode("utf-8")
    headers = [
        f"Status: {status}",
        f"Content-Type: {content_type}",
        f"Content-Length: {len(body_bytes)}",
    ]
    for h in headers:
        sys.stdout.write(h + "\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.flush()
    sys.stdout.buffer.write(body_bytes)


def main() -> None:
    query = os.environ.get("QUERY_STRING", "")
    params = parse_qs(query, keep_blank_values=False)

    # Validate required parameters explicitly (no silent fallbacks)
    if "name" not in params or "age" not in params:
        body = (
            "<html><body>"
            "<h1>400 Bad Request</h1>"
            "<p>Missing required query parameters 'name' and/or 'age'.</p>"
            "</body></html>"
        )
        send_response("400 Bad Request", body)
        return

    name = params["name"][0]
    age_str = params["age"][0]

    # Strict age validation – no fallback on invalid values
    if not age_str.isdigit():
        body = (
            "<html><body>"
            "<h1>400 Bad Request</h1>"
            "<p>Parameter 'age' must be a positive integer.</p>"
            "</body></html>"
        )
        send_response("400 Bad Request", body)
        return

    age = int(age_str)

    body = (
        "<html><body>"
        "<h1>User info from CGI</h1>"
        f"<p>Name: {name}</p>"
        f"<p>Age: {age}</p>"
        "</body></html>"
    )

    send_response("200 OK", body)


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        # Explicit error reporting instead of silent fallback
        err_body = (
            "<html><body>"
            "<h1>500 Internal Server Error</h1>"
            f"<p>Unhandled error in CGI script: {e}</p>"
            "</body></html>"
        )
        send_response("500 Internal Server Error", err_body)
