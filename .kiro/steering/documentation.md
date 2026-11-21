# Specs and Documentation

Whenever a change is made, the specs and documentation are updated in the same branch and pull request as the code. A change is not finished until they match what was built.

## Where things are documented

| Subject | Location |
|---------|----------|
| Firmware: sensor, averaging, NTP, reporting, status bar, display power | `.kiro/specs/hygrometer-reporting/` |
| Firmware: WiFi configuration, AP mode, credentials | `.kiro/specs/wifi-config-app/` |
| Server (tempmon2): ingest and query API | `.kiro/specs/json-http-ingestion/` in the tempmon2 repository |
| Working rules for Kiro | `.kiro/steering/` |

Device behavior belongs in the firmware specs. The tempmon2 spec records only what the server needs from the device, and points to the firmware spec for the rest.

## What to update

- `requirements.md`: new or changed behavior, as acceptance criteria. Add glossary terms for new concepts.
- `design.md`: components, interfaces and signatures, state machines, error handling, correctness properties and the test strategy. Names and signatures must match the shipped code, not an earlier plan.
- `tasks.md`: add tasks for the new work and mark them done. Correct tasks that describe code that was removed or renamed.
- Steering files, when a working rule or a hardware fact changes (for example the COM port behavior).

## When

- **Features:** update requirements, design and tasks together with the code.
- **Bug fixes:** update the spec when the fix changes documented behavior, an interface, or an error-handling entry, or when the spec was wrong. If the spec is unaffected, say so in the pull request description.
- **Design before code is fine, but** correct the design after implementation if names, signatures or behavior changed.
- **Releases:** the release notes must agree with the specs.

## Before reporting a change as done

1. Search the specs for the names, constants and behaviors you changed, and fix stale references.
2. Check that each new acceptance criterion has a task and, where testable, a test that names it.
3. If you find a gap between the specs and the code that this change does not address, file a GitHub issue (see the SDLC steering for the current mode). Do not leave it undocumented.
4. Commit the documentation with the code, as its own commit if that keeps the history clearer.
