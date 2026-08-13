#!/usr/bin/env python3

from http.server import SimpleHTTPRequestHandler, HTTPServer
from pathlib import Path
import os

ROOT = Path(__file__).resolve().parent
API = ROOT.parent / "api"

class Handler(SimpleHTTPRequestHandler):

    def do_GET(self):

        if self.path.startswith("/api/"):

            endpoint = self.path[5:]

            if endpoint == "geojson":
                target = API / "server.py"

                # API standalone handled by a simple internal call.
                import json

                geojson = API.parent / "data" / "processed" / \
                    "colombia_infrastructure.geojson"

                if not geojson.exists():
                    self.send_error(404)
                    return

                data = geojson.read_bytes()

                self.send_response(200)
                self.send_header(
                    "Content-Type",
                    "application/json"
                )
                self.send_header(
                    "Content-Length",
                    str(len(data))
                )
                self.end_headers()
                self.wfile.write(data)
                return

            if endpoint == "stats":
                import sqlite3

                db = API.parent / "data" / "processed" / \
                    "chrononet_colombia.db"

                if not db.exists():
                    self.send_error(404)
                    return

                conn = sqlite3.connect(db)

                values = {
                    "total_records":
                        conn.execute(
                            "SELECT COUNT(*) FROM infrastructure"
                        ).fetchone()[0],

                    "municipalities":
                        conn.execute(
                            "SELECT COUNT(DISTINCT municipality) "
                            "FROM infrastructure"
                        ).fetchone()[0],

                    "operators":
                        conn.execute(
                            "SELECT COUNT(DISTINCT operator) "
                            "FROM infrastructure"
                        ).fetchone()[0],

                    "technologies":
                        conn.execute(
                            "SELECT COUNT(DISTINCT technology) "
                            "FROM infrastructure"
                        ).fetchone()[0]
                }

                conn.close()

                data = json.dumps(values).encode()

                self.send_response(200)
                self.send_header(
                    "Content-Type",
                    "application/json"
                )
                self.send_header(
                    "Content-Length",
                    str(len(data))
                )
                self.end_headers()
                self.wfile.write(data)
                return

        super().do_GET()

os.chdir(ROOT)

print("============================================================")
print(" CHRONONET GIS WEB")
print("============================================================")
print("http://127.0.0.1:8788")
print("")

HTTPServer(
    ("127.0.0.1", 8788),
    Handler
).serve_forever()
