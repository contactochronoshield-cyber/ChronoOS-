# ChronoNet GIS

## Telecom Infrastructure Intelligence

ChronoNet GIS is the geospatial infrastructure intelligence module of
ChronoOS.

Its purpose is to visualize and analyze publicly available,
authorized and Chrono Shield-owned telecommunications infrastructure
data.

## Colombia

The first deployment uses public Colombian mobile infrastructure and
coverage information.

Source:

Datos Abiertos Colombia / MinTIC / Postdata / CRC

Dataset:

Infraestructura y cobertura de redes de acceso móvil

## Capabilities

- Geographic visualization
- Mobile infrastructure mapping
- Operator filtering
- Technology filtering
- Municipality analysis
- Infrastructure statistics
- GeoJSON export
- SQLite storage
- Local API
- Web map
- Future private infrastructure layers
- Future Chrono Shield measurement layers

## Data classes

ChronoNet separates:

1. Public data
2. Authorized operational data
3. Chrono Shield-owned infrastructure
4. Chrono Shield measurements

Operational control of third-party infrastructure is outside the scope
of this module.

## CLI

    chrono-netmap status

    chrono-netmap import

    chrono-netmap map

    chrono-netmap api

## Architecture

    ChronoOS
        |
        +-- ChronoNet GIS
                |
                +-- Public telecom data
                |
                +-- SQLite
                |
                +-- GeoJSON
                |
                +-- Local API
                |
                +-- GIS Web Map
                |
                +-- Chrono Shield infrastructure
