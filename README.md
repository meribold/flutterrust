# flutterrust

Just a university assignment.

## Building

You need GNU Make, GCC (`g++`), Boost, [ICU](http://site.icu-project.org), and wxWidgets.

For a release build, run

    make

and for a debugging build

    make DEBUG=1

All targets are created in subdirectories of `build`.  The main executable is called
`flutterrust`.

## Usage

*   Click and drag to scroll the map.
*   Right-click to place plants or animals using the context menu.
*   Hold `shift` and click and drag to test the pathfinding.
*   Hit `space` to unpause or pause the simulation (TODO).
*   Hit `F` to advance the simulation by a single step (TODO).

<!-- vim: set tw=90 sts=-1 sw=4 et spell: -->
