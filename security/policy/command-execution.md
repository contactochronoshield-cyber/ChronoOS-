# ChronoOS Command Execution Security Policy

External input MUST NOT be concatenated into shell commands.

Forbidden patterns include:

- system(user_input)
- os.system(user_input)
- shell=True with untrusted data
- popen(user_input)
- eval(user_input)
- exec(user_input)

Preferred approach:

1. Use direct APIs instead of a shell.
2. Use subprocess argument arrays.
3. Set shell=False.
4. Validate every externally supplied argument.
5. Apply allowlists where commands are unavoidable.
6. Never construct privileged commands from network input.
7. Log security-sensitive command execution.
8. Never expose raw command construction through an API.

Example:

BAD:

    subprocess.run("ping " + host, shell=True)

GOOD:

    subprocess.run(["ping", "-c", "1", validated_host], shell=False)

ChronoOS security-sensitive components MUST treat command execution
as a privileged boundary.
