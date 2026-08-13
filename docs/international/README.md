# ChronoOS International Infrastructure Profiles

ChronoOS now includes country-specific profiles for 5G infrastructure
and licensed media/IPTV management.

## 5G

- Estonia
- Perú
- El Salvador
- Chile
- Brasil

## Media / IPTV

- Estonia
- Perú
- El Salvador
- Chile

The profiles provide country-specific engineering and regulatory
metadata.

They do not themselves grant spectrum rights, telecommunications
concessions or content redistribution rights.

---

# 5G Regulatory References

## Estonia

Regulator: TTJA

5G reference bands:

- 694–790 MHz
- 3400–3800 MHz
- 24.7–27.1 GHz

https://www.ttja.ee/

## Perú

Authority: MTC

Primary 5G reference:

- 3300–3800 MHz
- 3500 MHz focus

https://www.gob.pe/mtc

## El Salvador

Regulator: SIGET

Spectrum is subject to the applicable national allocation table and
SIGET authorization.

https://www.siget.gob.sv/

## Chile

Regulator: SUBTEL

5G reference bands include:

- 700 MHz
- AWS
- 3300–3400 MHz
- 3600–3650 MHz
- 25.90–27.50 GHz

https://www.subtel.gob.cl/

---

# Media / IPTV

ChronoOS Media requires:

- Content origin
- Provider
- Territory
- Authorization
- License validity
- Traceability

Unauthorized retransmission is not part of the ChronoOS Media design.

---

# CLI

5G:

    chrono-5g-estonia-check
    chrono-5g-peru-check
    chrono-5g-el-salvador-check
    chrono-5g-chile-check

Media:

    chrono-media-estonia
    chrono-media-peru
    chrono-media-el-salvador
    chrono-media-chile
