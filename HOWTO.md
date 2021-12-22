# HOWTO
How to do different tasks in HyperDbg

## Automatic testing for the script engine
There is an automatic test for the script engine's statements.

The statement generator is available at: https://github.com/HyperDbg/script-engine-test

This repo is added automatically to HyperDbg when you cloned it with the `--recursive` flag.

To run the tests, you must first run `tasks/test-environment.py` to make the environment for testing the script engine.

After that, run the following command in your HyperDbg command-line:

```
? test
```
