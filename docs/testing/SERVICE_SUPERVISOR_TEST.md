# ChronoOS Service Supervisor Test

## Test

The ChronoOS service supervisor was tested with an intentionally
failing service.

## Verified behavior

- Service registry loading
- Service process creation
- Process state tracking
- Process termination detection using `waitpid()`
- Failure state transition
- `on-failure` restart policy
- Restart counter
- Maximum restart limit
- Final FAILED state after the restart limit

## Result

The test service intentionally exited with code `1`.

The supervisor detected the termination and restarted the service:

- Restart 1/3
- Restart 2/3
- Restart 3/3

After the third restart failed, the supervisor reported:

`Restart limit exceeded`

and transitioned the service to:

`FAILED`

## Status

PASS

This test demonstrates functional process supervision and restart
policy behavior in the ChronoOS development environment.
