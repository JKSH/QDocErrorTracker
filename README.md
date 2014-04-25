QDoc Error Tracker
==================
This tool is designed to help developers keep track of issues in the [Qt
documentation](http://qt-project.org/doc/), as identified by the [QDoc tool]
(http://qt-project.org/doc/qt-5/qdoc-index.html).


Intended Users
--------------
Users who want to contribute to the Qt documentation, and know how to build it
from source.


Features
--------
- Parses the STDERR output of the build process, to capture issues in the
  documentation source code. Captured issues are stored on disk.
- Displays captured issues from a build in a spreadsheet. Spreadsheet is
  sortable by source repository, file path, error message, etc.
- Easy diff between any two builds.
- Captured issues can be annotated. Annotations automatically apply to all
  builds which contain this particular issue.


Use Cases
---------
- Check that a patch fixes existing errors without introducing new ones.
- Monitor the state of the documentation as the Qt Project evolves. Identify
  when new issues emerge.
- Sort issues


Building the Program
--------------------
Simply build QDocErrorTracker.pro. The only requirements are:
- Qt 5.0.0 or later
- A C++11 compliant compiler


Usage
-----
Step 1: Build the Qt documentation and redirect STDERR to a log file. E.g.

    make html_docs 2> docerrors.log


Step 2: Launch QDoc Error Tracker and load the log file.
