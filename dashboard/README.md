# ChronoOS plant dashboard (demo)

A control-room style dashboard for an industrial plant: line status, ChronoOS device fleet, alarms, trends and one-click security incidents (tampered event, lost device, offline gateway, signed update). Two fictional sites: a bottling line and an aluminum rolling mill.

**All data is simulated.** It illustrates real ChronoOS 1.3 features (signed events, tamper rejection, revocation, offline queue, signed updates). PLC/OPC UA integration and real plant data are not part of this demo. The page has no external dependencies and works offline.

## Try it

**In a browser (Termux or any computer), from the repository root:**

    python3 -m http.server 8088 --bind 127.0.0.1 --directory dashboard/web
    # open http://127.0.0.1:8088

**On Android (APK):** built by GitHub Actions (`android-demo` workflow). Run it from the Actions tab, or push a tag named `demo-v*`; the APK and its SHA-256 are attached to the build (and to the release for tags). Check `sha256sum -c chronoos-demo.apk.sha256` before installing. It is a debug-signed demo build with no internet permission; Android will ask you to allow installing from this source.

## License

Evaluation use only, see `chronoos-1.3/DEMO-LICENSE.md`. Commercial licensing: contactochronoshield@gmail.com
