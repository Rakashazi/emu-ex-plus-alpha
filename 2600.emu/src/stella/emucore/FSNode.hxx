//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2015 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: FSNode.hxx 3131 2015-01-01 03:49:32Z stephena $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef FS_NODE_HXX
#define FS_NODE_HXX

#include <algorithm>

/*
 * The API described in this header is meant to allow for file system browsing in a
 * portable fashions. To this ends, multiple or single roots have to be supported
 * (compare Unix with a single root, Windows with multiple roots C:, D:, ...).
 *
 * To this end, we abstract away from paths; implementations can be based on
 * paths (and it's left to them whether / or \ or : is the path separator :-);
 * but it is also possible to use inodes or vrefs (MacOS 9) or anything else.
 *
 * You may ask now: "isn't this cheating? Why do we go through all this when we use
 * a path in the end anyway?!?".
 * Well, for once as long as we don't provide our own file open/read/write API, we
 * still have to use fopen(). Since all our targets already support fopen(), it should
 * be possible to get a fopen() compatible string for any file system node.
 *
 * Secondly, with this abstraction layer, we still avoid a lot of complications based on
 * differences in FS roots, different path separators, or even systems with no real
 * paths (MacOS 9 doesn't even have the notion of a "current directory").
 * And if we ever want to support devices with no FS in the classical sense (Palm...),
 * we can build upon this.
 */

#include "bspf.hxx"

class FilesystemNode;
class AbstractFSNode;

/**
 * List of multiple file system nodes. E.g. the contents of a given directory.
 * This is subclass instead of just a typedef so that we can use forward
 * declarations of it in other places.
 */
class FSList : public vector<FilesystemNode> { };

/**
 * This class acts as a wrapper around the AbstractFSNode class defined
 * in backends/fs.
 */
class FilesystemNode
{
  public:
    /**
     * Flag to tell listDir() which kind of files to list.
     */
    enum ListMode {
      kListFilesOnly = 1,
      kListDirectoriesOnly = 2,
      kListAll = 3
    };

    /**
     * Create a new pathless FilesystemNode. Since there's no path associated
     * with this node, path-related operations (i.e. exists(), isDirectory(),
     * getPath()) will always return false or raise an assertion.
     */
    FilesystemNode();

    /**
     * Create a new FilesystemNode referring to the specified path. This is
     * the counterpart to the path() method.
     *
     * If path is empty or equals '~', then a node representing the
     * "home directory" will be created. If that is not possible (since e.g. the
     * operating system doesn't support the concept), some other directory is
     * used (usually the root directory).
     */
    explicit FilesystemNode(const string& path);

    virtual ~FilesystemNode() { }

    /**
     * Compare the name of this node to the name of another. Directories
     * go before normal files.
     */
    inline bool operator<(const FilesystemNode& node) const
    {
      if (isDirectory() != node.isDirectory())
        return isDirectory();

      return BSPF_compareIgnoreCase(getName(), node.getName()) < 0;
    }

    /**
     * Compare the name of this node to the name of another, testing for
     * equality,
     */
    inline bool operator==(const FilesystemNode& node) const
    {
      return BSPF_compareIgnoreCase(getName(), node.getName()) == 0;
    }

    /**
     * Indicates whether the object referred by this path exists in the
     * filesystem or not.
     *
     * @return bool true if the path exists, false otherwise.
     */
    virtual bool exists() const;

    /**
     * Return a list of child nodes of this directory node. If called on a node
     * that does not represent a directory, false is returned.
     *
     * @return true if successful, false otherwise (e.g. when the directory
     *         does not exist).
     */
    virtual bool getChildren(FSList &fslist, ListMode mode = kListDirectoriesOnly,
                             bool hidden = false) const;

    /**
     * Return a string representation of the name of the file. This is can be
     * used e.g. by detection code that relies on matching the name of a given
     * file. But it is *not* suitable for use with fopen / File::open, nor
     * should it be archived.
     *
     * @return the file name
     */
    virtual const string& getName() const;

    /**
     * Return a string representation of the file which can be passed to fopen().
     * This will usually be a 'path' (hence the name of the method), but can
     * be anything that fulfills the above criterions.
     *
     * @return the 'path' represented by this filesystem node
     */
    virtual const string& getPath() const;

    /**
     * Return a string representation of the file which contains the '~'
     * symbol (if applicable), and is suitable for archiving (i.e. writing
     * to the config file).
     *
     * @return the 'path' represented by this filesystem node
     */
    virtual string getShortPath() const;

    /**
     * Determine whether this node has a parent.
     */
    bool hasParent() const;

    /**
     * Get the parent node of this node. If this node has no parent node,
     * then it returns a duplicate of this node.
     */
    FilesystemNode getParent() const;

    /**
     * Indicates whether the path refers to a directory or not.
     */
    virtual bool isDirectory() const;

    /**
     * Indicates whether the path refers to a real file or not.
     *
     * Currently, a symlink or pipe is not considered a file.
     */
    virtual bool isFile() const;

    /**
     * Indicates whether the object referred by this path can be read from or not.
     *
     * If the path refers to a directory, readability implies being able to read
     * and list the directory entries.
     *
     * If the path refers to a file, readability implies being able to read the
     * contents of the file.
     *
     * @return bool true if the object can be read, false otherwise.
     */
    virtual bool isReadable() const;

    /**
     * Indicates whether the object referred by this path can be written to or not.
     *
     * If the path refers to a directory, writability implies being able to modify
     * the directory entry (i.e. rename the directory, remove it or write files
     * inside of it).
     *
     * If the path refers to a file, writability implies being able to write data
     * to the file.
     *
     * @return bool true if the object can be written to, false otherwise.
     */
    virtual bool isWritable() const;

    /**
     * Create a directory from the current node path.
     *
     * @return bool true if the directory was created, false otherwise.
     */
    virtual bool makeDir();

    /**
     * Rename the current node path with the new given name.
     *
     * @return bool true if the node was renamed, false otherwise.
     */
    virtual bool rename(const string& newfile);

    /**
     * Read data (binary format) into the given buffer.
     *
     * @param buffer  The buffer to containing the data
     *                This will be allocated by the method, and must be
     *                freed by the caller.
     * @return  The number of bytes read (0 in the case of failure)
     *          This method can throw exceptions, and should be used inside
     *          a try-catch block.
     */
    virtual uInt32 read(uInt8*& buffer) const;

    /**
     * The following methods are almost exactly the same as the various
     * getXXXX() methods above.  Internally, they call the respective methods
     * and replace the extension (if present) with the given one.  If no
     * extension is present, the given one is appended instead.
     */
    string getNameWithExt(const string& ext) const;
    string getPathWithExt(const string& ext) const;
    string getShortPathWithExt(const string& ext) const; // FIXME - dead code

  private:
    shared_ptr<AbstractFSNode> _realNode;
    FilesystemNode(AbstractFSNode* realNode);
};


/**
 * Abstract file system node.  Private subclasses implement the actual
 * functionality.
 *
 * Most of the methods correspond directly to methods in class FSNode,
 * so if they are not documented here, look there for more information about
 * the semantics.
 */

using AbstractFSList = vector<AbstractFSNode*>;

class AbstractFSNode
{
  protected:
    friend class FilesystemNode;
    using ListMode = FilesystemNode::ListMode;

  public:
    /**
     * Destructor.
     */
    virtual ~AbstractFSNode() { }

    /*
     * Indicates whether the object referred by this path exists in the
     * filesystem or not.
     */
    virtual bool exists() const = 0;

    /**
     * Return a list of child nodes of this directory node. If called on a node
     * that does not represent a directory, false is returned.
     *
     * @param list List to put the contents of the directory in.
     * @param mode Mode to use while listing the directory.
     * @param hidden Whether to include hidden files or not in the results.
     *
     * @return true if successful, false otherwise (e.g. when the directory
     *         does not exist).
     */
    virtual bool getChildren(AbstractFSList& list, ListMode mode, bool hidden) const = 0;

    /**
     * Returns the last component of the path pointed by this FilesystemNode.
     *
     * Examples (POSIX):
     *			/foo/bar.txt would return /bar.txt
     *			/foo/bar/    would return /bar/
     *
     * @note This method is very architecture dependent, please check the concrete
     *       implementation for more information.
     */
    virtual const string& getName() const = 0;

    /**
     * Returns the 'path' of the current node, usable in fopen().
     */
    virtual const string& getPath() const = 0;

    /**
     * Returns the 'path' of the current node, containing '~' and for archiving.
     */

    virtual string getShortPath() const = 0;

    /**
     * Indicates whether this path refers to a directory or not.
     */
    virtual bool isDirectory() const = 0;

    /**
     * Indicates whether this path refers to a real file or not.
     */
    virtual bool isFile() const = 0;

    /**
     * Indicates whether the object referred by this path can be read from or not.
     *
     * If the path refers to a directory, readability implies being able to read
     * and list the directory entries.
     *
     * If the path refers to a file, readability implies being able to read the
     * contents of the file.
     *
     * @return bool true if the object can be read, false otherwise.
     */
    virtual bool isReadable() const = 0;

    /**
     * Indicates whether the object referred by this path can be written to or not.
     *
     * If the path refers to a directory, writability implies being able to modify
     * the directory entry (i.e. rename the directory, remove it or write files
     * inside of it).
     *
     * If the path refers to a file, writability implies being able to write data
     * to the file.
     *
     * @return bool true if the object can be written to, false otherwise.
     */
    virtual bool isWritable() const = 0;

    /**
     * Create a directory from the current node path.
     *
     * @return bool true if the directory was created, false otherwise.
     */
    virtual bool makeDir() = 0;

    /**
     * Rename the current node path with the new given name.
     *
     * @return bool true if the node was renamed, false otherwise.
     */
    virtual bool rename(const string& newfile) = 0;

    /**
     * Read data (binary format) into the given buffer.
     *
     * @param buffer  The buffer to containing the data
     *                This will be allocated by the method, and must be
     *                freed by the caller.
     * @return  The number of bytes read (0 in the case of failure)
     *          This method can throw exceptions, and should be used inside
     *          a try-catch block.
     */
    virtual uInt32 read(uInt8*& buffer) const { return 0; }

    /**
     * The parent node of this directory.
     * The parent of the root is the root itself.
     */
    virtual AbstractFSNode* getParent() const = 0;
};

#endif
