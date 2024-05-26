# WINAPI Tetris-style screensaver for windows 10/11

## Steps to Install and Test the Screensaver:

### Compile the Screensaver:

Compile your project in Release mode to generate the .exe file.
Rename the compiled ``.exe`` file to have a ``.scr`` extension. For example, if your executable is ``ScreensaverTetris.exe``, rename it to ``ScreensaverTetris.scr``.

### 1. Move the .scr File:

- Copy the renamed .scr file to the Windows system directory. The typical location is:
  - For 32-bit Windows: ``C:\Windows\System32``
  - For 64-bit Windows: ``C:\Windows\SysWOW64``

### 2. Register the Screensaver:

- Right-click on the ``.scr`` file in the system directory and select Install. This will open the screensaver settings dialog where you can preview and configure the screensaver.

### 3. Test the Screensaver:

- Open the Windows Settings or Control Panel.
- Go to Personalization > Lock screen > Screen saver settings.
- Select your screensaver from the list (it should appear with the name you gave the .scr file).
- Click Preview to test the screensaver, or set the wait time and click OK to enable it.

