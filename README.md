# Autolog

The only thing that I am more consistent in doing than playing pickleball is forgetting to log my progress for my computer science coursework. At Yale, nearly all coding-based CS coursework utilizes the log format created by Professor Stan Eisenstat, and as such, streamlining the logging process would make it harder to forget.

Autolog is a CLI tool to accomplish that task, following Professor Eisenstat's log format. In order to minimize resource overhead and due to the infrequent calls of autolog in its correct use case, I elected to design this as a finite-state machine rather than a daemon. To streamline its use, after the log is created within the project root directory, all subsequent commands can be called from the project root directory or subdirectory of the project root, regardless of the depth of the subdirectory hierarchy.

### Set-up

- Install: `git clone https://github.com/sebaraj/Autolog.git`
- In project directory, build: `g++ -o autolog autolog.cpp`
- If you have superuser permissions, move executable to path: `sudo mv autolog /usr/log/bin/`
- Otherwise (such as on the Zoo), create a local bin path: `mkdir -p ~/.local/bin` and move the executable: `mv autolog ~/.local/bin/`

### Use

- Create log in the project root directory: `autolog create <logfile_name>`
  - Common logfile names include `time.log`, `LOG.md`
- Add collaborators to log: `autolog collab <collaborator_name>`
- When starting your coding session: `autolog start`
- When you are done with your coding session: `autolog stop`
