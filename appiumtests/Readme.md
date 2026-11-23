# GUI tests

These tests drive Dolphin through the accessibility bus with
[selenium-webdriver-at-spi](https://invent.kde.org/sdk/selenium-webdriver-at-spi).
Each test file starts its own nested KWin and one Dolphin for all its tests.

- `dolphinappium.py` holds the shared setup and helpers.
- `locationbartest.py`, `filterbartest.py`, `searchbartest.py` and `columnsviewtest.py` hold the tests.

## Running

Build and install selenium-webdriver-at-spi, then configure Dolphin with its prefix in
`CMAKE_PREFIX_PATH` so that the tests are added. The Python client needs a virtual
environment that sees the system `pyatspi` and `gi`:

```bash
python3 -m venv --system-site-packages ~/.venvs/selenium-atspi
source ~/.venvs/selenium-atspi/bin/activate
pip install Appium-Python-Client selenium
```

Run the tests through ctest, so that they test the Dolphin of the build:

```bash
ctest --test-dir build -R 'locationbartest|filterbartest|searchbartest|columnsviewtest' --output-on-failure
```

A single file also runs directly, but then tests the installed Dolphin:

```bash
selenium-webdriver-at-spi-run ./filterbartest.py
```