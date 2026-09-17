"""
Chrono 5G Core architecture definition.

This module describes the integration boundary between Chrono
control services and a standards-based 5G Core implementation.

Chrono does not reimplement the 3GPP 5G Core protocols here.
The production target is integration with Open5GS or another
standards-compliant 5G Core implementation.
"""

COMPONENTS = {
    "AMF": "Access and Mobility Management",
    "SMF": "Session Management",
    "UPF": "User Plane",
    "UDM": "Unified Data Management",
    "AUSF": "Authentication Server",
    "NRF": "Network Repository",
    "PCF": "Policy Control",
    "NSSF": "Network Slice Selection",
}

CHRONO_PATH = [
    "UE",
    "5G NR",
    "gNB",
    "5G Core",
    "UPF",
    "Chrono Edge",
    "Chrono Backbone",
]


def architecture():
    return {
        "control_plane": "Chrono 5G Core API",
        "standards_core": "Open5GS integration target",
        "components": COMPONENTS,
        "data_path": CHRONO_PATH,
    }
