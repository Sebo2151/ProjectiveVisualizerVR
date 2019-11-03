# ProjectiveVisualizerVR
Displays real algebraic surfaces in projective space in virtual reality.

Runs on Windows using an HTC Vive. Possibly compatible with Oculus, but untested.

To run, navigate to ProjectiveVisualizerVR/bin, then double-click ProjectiveViewerVR.exe.

The program should display a randomly selected cubic surface at startup.

The program uses one Vive controller for input:
- Press and hold the trigger for affine transformations of the surface.
- Press and hold the trackpad button for projective transformations of the surface. The surface will turn purple while this is happening.
  (Purple = Projective)
- Combining the two controls above often eventually distorts the coordinate system enough that the mesh starts to look bad - Press the
  grip button to return to the default coordinate system.
- Press the menu button to open an equation input screen. The surface will disappear. Type any algebraic equation in x,y,z,w with integer
  coefficients using your keyboard, then press Enter or press the menu button again to view the corresponding surface. The display will
  turn red-ish if there is a syntax error. Any inputted equation will be automatically homogenized by inserting multiples of w if needed.
  
  The source code should be editable and compilable from Visual Studio. I make no claims of elegance or readability.
