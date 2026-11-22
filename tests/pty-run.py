"""Run a command on a pseudo-terminal, typing keys into it.

    python3 pty-run.py KEYS COMMAND [ARG]...

KEYS is typed once the command has had a moment to start. It takes
Python string escapes (\\x05 is Ctrl-E); \\x00 separates groups of
keys, with a pause between each. The command's output is written to
stdout, and its exit status is this script's. A command still running
after 15 seconds is killed.
"""

import os
import pty
import select
import sys
import time


def main():
    # unicode_escape yields one code point per byte; latin-1 turns them
    # back into the UTF-8 bytes the test wrote.
    keys = sys.argv[1].encode("utf-8").decode("unicode_escape").encode("latin-1")
    argv = sys.argv[2:]

    pid, fd = pty.fork()
    if pid == 0:
        os.execvp(argv[0], argv)

    output = bytearray()
    deadline = time.time() + 15

    def drain(seconds):
        """Collect output for SECONDS; False once the terminal closes."""
        end = time.time() + seconds
        while time.time() < end:
            ready, _, _ = select.select([fd], [], [], 0.05)
            if ready:
                try:
                    data = os.read(fd, 4096)
                except OSError:
                    return False
                if not data:
                    return False
                output.extend(data)
        return True

    alive = drain(0.5)
    for group in keys.split(b"\x00"):
        if not alive:
            break
        os.write(fd, group)
        alive = drain(0.4)

    status = None
    while time.time() < deadline:
        finished, status = os.waitpid(pid, os.WNOHANG)
        if finished:
            break
        status = None
        if alive:
            alive = drain(0.1)
        else:
            time.sleep(0.05)
    if status is None:
        os.kill(pid, 9)
        _, status = os.waitpid(pid, 0)
        output.extend(b"\npty-run: timed out\n")
    drain(0.1)

    sys.stdout.write(output.decode("utf-8", "replace"))
    if os.WIFEXITED(status):
        sys.exit(os.WEXITSTATUS(status))
    sys.exit(128 + os.WTERMSIG(status))


main()
