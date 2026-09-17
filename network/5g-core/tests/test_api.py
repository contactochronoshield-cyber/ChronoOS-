from fastapi.testclient import TestClient

from api.app import app


client = TestClient(app)


def test_health():
    response = client.get("/api/health")

    assert response.status_code == 200
    assert response.json()["status"] == "online"


def test_register_node():
    response = client.post(
        "/api/nodes",
        json={
            "node_id": "CR5G-001",
            "name": "Chrono 5G Core Node",
            "node_type": "core",
        },
    )

    assert response.status_code == 200
    assert response.json()["node_id"] == "CR5G-001"


def test_create_network():
    response = client.post(
        "/api/networks",
        json={
            "network_id": "CHRONO-PRIVATE-01",
            "name": "Chrono Private 5G",
        },
    )

    assert response.status_code == 200


def test_register_subscriber():
    response = client.post(
        "/api/subscribers",
        json={
            "subscriber_id": "SUB-001",
            "imsi": "001010000000001",
        },
    )

    assert response.status_code == 200
