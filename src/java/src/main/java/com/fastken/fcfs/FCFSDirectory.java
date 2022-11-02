package com.fastken.fcfs;
  
import java.nio.file.FileSystemException;

public class FCFSDirectory {
    public static class Entry {
        private long inode;
        private String name;

        private static native void doInit();
        static {
            doInit();
        }

        public Entry(long inode, String name) {
            this.inode = inode;
            this.name = name;
        }

        public long getInode() {
            return this.inode;
        }

        public String getName() {
            return this.name;
        }
    }

    private static native void doInit();

    static {
        doInit();
    }

    public native Entry next();
    public native void seek(long loc);
    public native long tell();
    public native void rewind();
    public native void close();

    private long handler;

    public FCFSDirectory(long handler) {
        this.handler = handler;
    }

    public void setHandler(long handler) {
        this.handler = handler;
    }

    public long getHandler() {
        return this.handler;
    }
}
