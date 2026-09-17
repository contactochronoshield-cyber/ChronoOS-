from datetime import datetime, timezone
from typing import Any

from fastapi import FastAPI
from pydantic import BaseModel, Field


app = FastAPI(
    title="Chrono 5G Core",
    description="Chrono Shield 5G Core Control API",
    version="0.1.0",
)


nodes: list[dict[str, Any]] = []
networks: list[dict[str, Any]] = []
subscribers: list[dict[str, Any]] = []
telemetry: list[dict[str, Any]] = []


def now() -> str:
    return datetime.now(timezone.utc).isoformat()


class Node(BaseModel):
    node_id: str = Field(min_length=3)
    name: str
    node_type: str = "edge"
    status: str = "online"


class Network(BaseModel):
    network_id: str = Field(min_length=3)
    name: str
    network_type: str = "private-5g"


class Subscriber(BaseModel):
    subscriber_id: str = Field(min_length=3)
    imsi: str
    status: str = "active"


class Telemetry(BaseModel):
    node_id: str
    metric: str
    value: float
    unit: str = ""


@app.get("/api/health")
def health():
    return {
        "status": "online",
        "service": "chrono-5g-core",
        "version": app.version,
        "timestamp": now(),
    }


@app.get("/api")
def api_info():
    return {
        "name": "Chrono 5G Core",
        "version": app.version,
        "status": "development-core",
        "architecture": {
            "control": "Chrono 5G Core API",
            "5g_core": "Open5GS integration target",
            "edge": "Chrono Edge",
            "backbone": "Chrono Backbone",
        },
    }


@app.get("/api/nodes")
def get_nodes():
    return {
        "count": len(nodes),
        "nodes": nodes,
    }


@app.post("/api/nodes")
def register_node(node: Node):
    record = node.model_dump()
    record["registered_at"] = now()
    nodes.append(record)
    return record


@app.get("/api/networks")
def get_networks():
    return {
        "count": len(networks),
        "networks": networks,
    }


@app.post("/api/networks")
def create_network(network: Network):
    record = network.model_dump()
    record["created_at"] = now()
    networks.append(record)
    return record


@app.get("/api/subscribers")
def get_subscribers():
    return {
        "count": len(subscribers),
        "subscribers": subscribers,
    }


@app.post("/api/subscribers")
def register_subscriber(subscriber: Subscriber):
    record = subscriber.model_dump()
    record["registered_at"] = now()
    subscribers.append(record)
    return record


@app.get("/api/telemetry")
def get_telemetry():
    return {
        "count": len(telemetry),
        "telemetry": telemetry,
    }


@app.post("/api/telemetry")
def receive_telemetry(data: Telemetry):
    record = data.model_dump()
    record["received_at"] = now()
    telemetry.append(record)
    return record
