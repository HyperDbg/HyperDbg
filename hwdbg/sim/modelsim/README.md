# Automated ModelSim Viewer

First of all, make sure to edit the address of the "ModelSim" directory in **modelsim.py**.

After that, only modify the "modelsim.config" file.

In the "modelsim.config" file, the first line that starts with "module:" is the name of the target module's **Tester** class. The rest of the lines are signals to be shown.

For example:
```
module:DebuggerModuleTest
clock
inputPin 
```

If you don't specify the signals to be filtered, then **ALL** signals will be shown.

For example:
```
module:DebuggerModuleTest
```

At last, run it with the following command:
```
python3 modelsim.py
```

or,
```
python3 sim/modelsim.py
```

