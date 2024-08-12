from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import sys

class MagicServer(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        return super(MagicServer, self).end_headers()

with ThreadingHTTPServer(('0.0.0.0', 8000), MagicServer) as httpd:
    print("listening on 0.0.0.0:8000")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        sys.exit(0)

