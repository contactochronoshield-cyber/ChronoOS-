#!/usr/bin/env python3

import json
import sqlite3
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
from urllib.parse import urlparse, parse_qs

ROOT = Path(__file__).resolve().parents[1]
DB = ROOT / "data" / "processed" / "chrononet_colombia.db"
GEOJSON = ROOT / "data" / "processed" / "colombia_infrastructure.geojson"

HOST = "127.0.0.1"
PORT = 8787

class Handler(BaseHTTPRequestHandler):

    def send_json(self, payload, status=200):
        data = json.dumps(
            payload,
            ensure_ascii=False
        ).encode("utf-8")

        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):

        parsed = urlparse(self.path)

        if parsed.path == "/health":
            self.send_json({
                "status": "online",
                "service": "ChronoNet GIS",
                "country": "CO"
            })
            return

        if parsed.path == "/geojson":

            if not GEOJSON.exists():
                self.send_json({
                    "error": "GeoJSON dataset not available"
                }, 404)
                return

            self.send_json(
                json.loads(
                    GEOJSON.read_text(encoding="utf-8")
                )
            )
            return

        if parsed.path == "/stats":

            if not DB.exists():
                self.send_json({
                    "error": "Database not available"
                }, 404)
                return

            conn = sqlite3.connect(DB)

            total = conn.execute(
                "SELECT COUNT(*) FROM infrastructure"
            ).fetchone()[0]

            municipalities = conn.execute(
                "SELECT COUNT(DISTINCT municipality) FROM infrastructure"
            ).fetchone()[0]

            operators = conn.execute(
                "SELECT COUNT(DISTINCT operator) FROM infrastructure"
            ).fetchone()[0]

            technologies = conn.execute(
                "SELECT COUNT(DISTINCT technology) FROM infrastructure"
            ).fetchone()[0]

            conn.close()

            self.send_json({
                "country": "Colombia",
                "total_records": total,
                "municipalities": municipalities,
                "operators": operators,
                "technologies": technologies
            })
            return

        self.send_json({
            "service": "ChronoNet GIS",
            "endpoints": [
                "/health",
                "/geojson",
                "/stats"
            ]
        })

def main():
    print("============================================================")
    print(" CHRONONET GIS API")
    print("============================================================")
    print(f"Listening on http://{HOST}:{PORT}")
    print("")

    server = HTTPServer((HOST, PORT), Handler)
    server.serve_forever()

if __name__ == "__main__":
    main()
