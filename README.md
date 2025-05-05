# PCom HTTP Client Checker

This repository contains checker for the PCom HTTP client homework.

## Prerequisites

Dependencies:

- Python >= 3.7;
- [`pexpect`](https://pexpect.readthedocs.io/en/stable/) (third party Python package);

It is highly recommended to use a VirtualEnv, either by using the bundled
Makefile or by manually installing the dependencies:
```sh
# symply run:
make
# this will do the same as:
python3 -mvenv .venv
.venv/bin/python3 -mpip install -r requirements.txt
# Note: you need to source activate each time you start a new terminal
source .venv/bin/activate
```

### Usage

Invoke the checker on your client's compiled executable:

```sh
# !!! don't forget to source .venv/bin/activate !!!
# first, read the tool's internal help:
python3 checker.py --help 
# run the checker using default settings:
python3 checker.py ../path/to/client
# Hint: supply a custom username:password (make sure to use something unique)
python3 checker.py --admin 'myadminuser:hunter2' ../path-to/client
```

The default test script uses the admin user to create a random normal test user.
This will ensure a clean slate while doing all other tests (since the server 
persists all edits inside a database).

Alternately, you can use e.g., `--script delete_movies` if you have a functioning
implementation for `get_movies` and `delete_movie` to quickly cleanup your
associated database.

Also make sure to check out [the source code](./checker.py) for the
actual details about the script(s) being tested.

<span style="color: #A33">**Warning**: This script is just an instrument used
by our team to automate the homework verification process.
If any bugs affecting its effectiveness are found, we reserve the right to
correct them at any time (you will be notified when this is the case).
When in doubt, use the homework text as the rightful source of truth and use the
Moodle Forum to ask any questions.
</span>

