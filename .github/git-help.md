To remove a submodule you need to:

- Delete the relevant section from the .gitmodules file.
- Stage the .gitmodules changes git add .gitmodules
- Delete the relevant section from .git/config.
- Run "git rm --cached path_to_submodule" (no trailing slash).
- Run rm -rf .git/modules/path_to_submodule (no trailing slash).
- Commit git commit -m "Removed submodule <name>"
- Delete the now untracked submodule files rm -rf path_to_submodule

To add a submodule :
- git submodule add https://github.com/HyperDbg/evaluation

Discard Changes on a special file :
- git checkout hyperdbg/hprdbgctrl/print.cpp