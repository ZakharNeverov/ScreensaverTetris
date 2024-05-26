**WINAPI Tetris-style screensaver for windows 10/11**
Steps to Install and Test the Screensaver:
Compile the Screensaver:

Compile your project in Release mode to generate the .exe file.
Rename the compiled ``.exe`` file to have a ``.scr`` extension. For example, if your executable is ``ScreensaverTetris.exe``, rename it to ``ScreensaverTetris.scr``.
Move the .scr File:

Copy the renamed .scr file to the Windows system directory. The typical location is:
For 32-bit Windows: ``C:\Windows\System32``
For 64-bit Windows: ``C:\Windows\SysWOW64``
Register the Screensaver:

Right-click on the ``.scr`` file in the system directory and select Install. This will open the screensaver settings dialog where you can preview and configure the screensaver.
Test the Screensaver:

Open the Windows Settings or Control Panel.
Go to Personalization > Lock screen > Screen saver settings.
Select your screensaver from the list (it should appear with the name you gave the .scr file).
Click Preview to test the screensaver, or set the wait time and click OK to enable it.
Additional Notes:
Exit Conditions: Ensure the screensaver can exit properly on user input (mouse movement, key press). Your code already handles this with the PostQuitMessage(0); calls in the WM_MOUSEMOVE, WM_KEYDOWN, WM_LBUTTONDOWN, and WM_RBUTTONDOWN cases.
Configuration Dialog: If you want to provide a configuration dialog, ensure the ShowConfigDialog function properly handles this case. Your current implementation shows a message box indicating no configuration is available, which is fine if no configuration is needed.
Preview Mode: If you want to support the preview mode (when the screensaver is shown in a small window in the settings), you'll need to implement this. The current placeholder returns 0 for this case.
