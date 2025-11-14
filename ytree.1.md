---
title: YTREE
section: 1
header: "LOCAL COMMANDS"
date: "21 March 2004"
---

# NAME

ytree - File Manager -

# SYNOPSYS

`ytree` [*archive file*|*directory*]

# DESCRIPTION

If there is no command line argument, the current directory will be used.

Following commands are available:

### 1.) DIR-Modus:

**-Attribute**
:   Change directory permissions (like chmod)

**-Delete**
:   Delete selected directory

**-Filespec**
:   Set file specification (reg. expression)
    e.g. *.[ch] for all *.c and *.h - Files

**-Group**
:   Change directory group ownership

**-Log**
:   Restart ytree with new
    root directory/archiv file

**-Makedir**
:   Create new directory

**-Owner**
:   Change user ownership of selected directory

**-Rename**
:   Rename selected directory

**-Showall**
:   Show all files in all directories

**-Tag**
:   Tag all files in selected directory

**-Untag**
:   Untag all files in selected directory

**-eXecute**
:   Execute a shell command. The `{}` placeholder is replaced by the current directory path (`.`).

**-^Dirmode**
:   Change viewmodus for files:

    -   **filenames only**
    -   **name, attributes, links, size,**
        **modification time, symb. link**
    -   **name, attribute, inode, owner, group, symb. link**
    -   **change status-, access time, symb. link**

**-Return**
:   Switch to file modus

**-^L (redraw)**
:   Re-read the contents of the current directory from disk and refresh the view. This is useful after running an external shell command that modifies files.

**-^Quit**
:   QuitTo: If you exit ytree with ^Q, the last selected directory becomes your
    current working directory. This feature only works if you start ytree
    with this bash-function (copy this to your ~/.bashrc):
    ```
    function yt
    {
      echo cd $PWD >~/.ytree-$$.chdir
      /usr/bin/ytree $1 $2 $3 $4 $5
      source ~/.ytree-$$.chdir
      rm ~/.ytree-$$.chdir
    }
    ```

### 2.) FILE-Modus

**-Attribute**
:   Change file permissions (like chmod)

**-^Attribute**
:   Change permissions of all tagged files.
    **?** stands for: do not change attribute

**-Copy/(^K)**
:   (C)opy: Copy the selected file.
:   (^K) Copy: Copy all tagged files.

**-Delete**
:   Delete selected file

**-^Delete**
:   Delete all tagged files

**-Edit**
:   Edit selected file with EDITOR (see ~/.ytree)
    or - if not defined - vi

**-Filespec**
:   Set file specification (reg. expression)
    e.g. *.[ch] for all *.c and *.h - Files

**-Group**
:   Change group ownership of selected file

**-^Group**
:   Change group ownership of all tagged files

**-Hex**
:   View selected file with HEXDUMP (see ~/.ytree),
    or - if not defined - hd / od -h

**-Log**
:   Restart ytree with new root directory/archive file

**-Move/(^N)**
:   (M)ove: Move the selected file.
:   (^N) Move: Move all tagged files. The `^N` (Next) shortcut is used because `^M` is the terminal control code for the Enter key.

**-Owner**
:   Change user ownership of selected file

**-^Owner**
:   Change user ownership of all tagged files

**-Pipe**
:   Pipe content of file to a command. This sends the file's *content* to the command's standard input (e.g., `cat file | wc`). It does not use the `{}` placeholder.

**-^Pipe**
:   Pipe content of all tagged files to a command.

**-Rename/(^R)**
:   (R)ename: Rename the selected file.
:   (^R)ename: Rename all tagged files.

**-untag ^Search**
:   Untag files by using an external program (e.g. grep)

**-Sort**
:   Sort filelist by

    -   **access time**
    -   **change time**
    -   **extension**
    -   **group**
    -   **modification time**
    -   **name**
    -   **owner**
    -   **size**

**-Tag**
:   Tag selected file

**-^Tag**
:   Tag all currently shown files

**-Untag**
:   Untag selected file

**-^Untag**
:   Untag all currently shown files

**-View**
:   View file with the pager defined in ~/.ytree
    or - if not defined - with pg -cen

**-eXecute**
:   Execute a shell command. The `{}` placeholder is replaced by the selected filename.

**-e^Xecute**
:   Execute shell command for all tagged files.
    The string {} is replaced by each file's full path.

**-pathcopY/^Y**
:   (Y) PathCopy: Copy selected file inclusive path.
:   (^Y) PathCopy: Copy all tagged files inclusive path.

**-^Filemode**
:   Switch view-modus for files:

    -   **filenames only**
    -   **name, attribute, links, size, modification time,**
        **symb. link**
    -   **name, attribute, inode, owner, group, symb. link**
    -   **changestatus-, access time, symb. link**

**-^L (redraw)**
:   Re-read the contents of the current directory from disk and redraw the screen.

**-Space**
:   Suppress screen-output while working

**-Return**
:   Switch to expand modus

### 3.) ARCHIV-DIR-Modus

**-Filespec**
:   Set file specification (reg. expression)
    e.g. *.[ch] for all *.c and *.h - Files

**-Log**
:   Restart ytree with new root directory/archive file

**-Showall**
:   Show all files in all directories

**-Tag**
:   Tag all files in selected directory

**-Untag**
:   Untag all files in selected directory

**-^Dirmode**
:   Change viewmodus for files:

    -   **filenames only**
    -   **name, attribute, links, size, modification time**
    -   **name, attribute, owner, group**

**-^L (redraw)**
:   Redraw the screen.

### 4.) ARCHIV-FILE-Modus:

**-Copy**
:   Copy selected file

**-^K Copy**
:   Copy all tagged files

**-Filespec**
:   Set file specification (reg. expression)
    e.g. *.[ch] for all *.c and *.h - Files

**-Hex**
:   View selected file with HEXDUMP (see ~/.ytree),
    or - if not defined - hd / od -h

**-Pipe**
:   Pipe content of all tagged to a command

**-Sort**
:   Sort file list by

    -   **access time**
    -   **change time**
    -   **extension**
    -   **group**
    -   **modification time**
    -   **name**
    -   **owner**
    -   **size**

**-Tag**
:   Tag selected file

**-^Tag**
:   Tag all files in selected directory

**-Untag**
:   Untag all files in selected directory

**-View**
:   View file with the pager defined in ~/.ytree
    or - if not defined - with pg -cen

**-^Filemode**
:   Switch view-modus for files:

    -   **filenames only**
    -   **name, attribute, links, size**

**-^L (redraw)**
:   Redraw the screen.

**-Return**
:   Switch to Expand-Modus

ytree switches to archive-modus automatically by choosing an archive file with
the **Log** command or by calling ytree from the command line with an archive
file given as a command line argument. It uses the libarchive library to read
a wide variety of archive and compression formats.

The View command is customizeable in the [VIEWER] section of ~/.ytree:

Example:

```
[VIEWER]
.jpg,.gif,.bmp,.tif,.ppm,.xpm=xv
.1,.2,.3,.4,.5,.6,.7,.8,.n=nroff -man | less
.ps=ghostview
.mid,.MID=playmidi -e
.wav,.WAV=splay
.au=auplay
.avi,.mpg,.mov=xanim
.htm,.html=lynx
.pdf,.PDF=acroread
.mp3=mpg123
```

A command-line history is supported: Use cursor up/down.
Use "F2" on the command-line to select directories.

# FILES

$HOME/.ytree
:   ytree configuration file

# BUGS

To avoid problems with escape sequences on RS/6000 machines
(telnet/rlogin) please set the environment variable ESCDELAY:

```
ESCDELAY=1000
export ESCDELAY
```

# AUTOR

W. Bregulla (werner@frolix.han.de)