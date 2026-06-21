## Removing submodules
To remove a submodule you need to:

- Delete the relevant section from the `.gitmodules` file.
- Stage the `.gitmodules` changes git add `.gitmodules`.
- Delete the relevant section from `.git/config`.
- Run `git rm --cached path_to_submodule` (no trailing slash).
- Run `rm -rf .git/modules/path_to_submodule` (no trailing slash).
- Commit `git commit -m "Removed submodule <name>"`.
- Delete the now untracked submodule files `rm -rf path_to_submodule`.

To add a submodule:
```
git submodule add https://github.com/HyperDbg/evaluation
```

Discard changes on a special file:
```
git checkout hyperdbg/hprdbgctrl/print.cpp
```
  
To release (deploy):
  
 Imagine Tag is **v0.1.0** (tags should start with the 'v*', and only on the master branch). After that, we will use the following command.
 ```
git checkout master
git tag -a v0.1.0 -m "your descriptive comment"
git push origin master v0.1.0
 ```

To update submodules:

```
git submodule update --init --recursive --remote
```

To remove the effects of 'git add .' without modifying (removing) the changes in the source code (remove the source codes ready for staging):
```
git reset
```

To remove the effects of `git add .` and `git commit -m "test"` or just `git add .` without modifying (removing) the changes in the source code:
```
git reset --soft HEAD~1
```

To remove **all uncommitted and untracked changes** in Git and reset your working tree to the last commit:

```bash
git reset --hard
git clean -fd
```

To modify the last pushed commit:

```bash
# Undo the last commit but keep the changes in your working tree
git reset --soft HEAD~1

# Make your additional modifications
# edit files...

# Stage everything
git add .

# Create a new commit
git commit -m "Updated commit message"

# Force-push to replace the remote commit
git push --force-with-lease
```

---------------

## Releasing instructions
```
git checkout master 
git pull
git tag -a v0.1.0 -m "HyperDbg releases" 
git push origin master v0.1.0
git checkout dev
git rebase master
git push
```
