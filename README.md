# oh-extract
Small utility for datamining the (bindict) client data from Once Human.

## Usage
### Python REPL
1. Build the DLL with Visual Studio 2022, or download the release DLL.
2. Start the game
3. Inject the DLL your injector of choice
4. A windows console will be spawned into a python REPL within the game's python interpreter.

### bindict extractor
1. Under the python REPL, run the following:
```python
exec(open(r'C:\dev\oh-extract\scripts\dump.py').read())
```
(Replacing the path to your own cloned repository path)

2. The script will find any modules containing `bindict` variables and dump them as JSON to `<GAME_ROOT>/json_output/*`.